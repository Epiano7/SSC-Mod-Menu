"""Per-launch loopback bridge to the native Secret Service keyring.
No activation codes or grants are handled here. Device seeds never go to disk.
"""
import ctypes as C
import hmac
import json
import os
from pathlib import Path
import re
import secrets
import stat
import sys
import tempfile
import contextlib
from http.server import BaseHTTPRequestHandler, HTTPServer

class StorageUnavailable(Exception):
    pass

class Attribute(C.Structure):
    _fields_ = [('name', C.c_char_p), ('type', C.c_int)]
class Schema(C.Structure):
    _fields_ = [('name', C.c_char_p), ('flags', C.c_int), ('attributes', Attribute * 32),
                ('reserved', C.c_int)] + [('reserved'+str(i), C.c_void_p) for i in range(1,8)]

class Keyring:
    def __init__(self):
        try:
            self.lib=C.CDLL('libsecret-1.so.0'); self.g=C.CDLL('libglib-2.0.so.0')
        except OSError as e: raise StorageUnavailable() from e
        self.schema=Schema(); self.schema.name=b'org.epiano7.SSCMod.Device.v1'
        self.schema.attributes[0]=Attribute(b'steam_id',0)
        self.g.g_hash_table_new.argtypes=[C.c_void_p,C.c_void_p]; self.g.g_hash_table_new.restype=C.c_void_p
        self.g.g_hash_table_insert.argtypes=[C.c_void_p,C.c_void_p,C.c_void_p]
        self.g.g_hash_table_unref.argtypes=[C.c_void_p]
        self.g.g_error_free.argtypes=[C.c_void_p]
        self.lib.secret_password_lookupv_sync.argtypes=[C.POINTER(Schema),C.c_void_p,C.c_void_p,C.POINTER(C.c_void_p)]
        self.lib.secret_password_lookupv_sync.restype=C.c_void_p
        self.lib.secret_password_storev_sync.argtypes=[C.POINTER(Schema),C.c_void_p,C.c_char_p,C.c_char_p,C.c_char_p,C.c_void_p,C.POINTER(C.c_void_p)]
        self.lib.secret_password_storev_sync.restype=C.c_int
        self.lib.secret_password_free.argtypes=[C.c_void_p]
    def seed(self, account, create):
        # Serialize key creation across simultaneous Steam launches/prefixes.
        import fcntl
        folder=Path(tempfile.gettempdir()) / ('ssc-keyring-lock-'+str(os.getuid()))
        try: folder.mkdir(mode=0o700)
        except FileExistsError: pass
        st=folder.lstat()
        if not stat.S_ISDIR(st.st_mode) or st.st_uid!=os.getuid() or st.st_mode&0o077: raise StorageUnavailable()
        fd=os.open(folder/'lock',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600)
        try:
            st=os.fstat(fd)
            if not stat.S_ISREG(st.st_mode) or st.st_uid!=os.getuid() or st.st_nlink!=1 or st.st_mode&0o077: raise StorageUnavailable()
            fcntl.flock(fd,fcntl.LOCK_EX)
            return self._seed(account,create)
        finally: os.close(fd)
    def _seed(self, account, create):
        attrs=self.g.g_hash_table_new(C.cast(self.g.g_str_hash,C.c_void_p),C.cast(self.g.g_str_equal,C.c_void_p))
        if not attrs: raise StorageUnavailable()
        key=C.create_string_buffer(b'steam_id'); value=C.create_string_buffer(account.encode('ascii'))
        self.g.g_hash_table_insert(attrs,key,value)
        try:
            def lookup():
                err=C.c_void_p(); ptr=self.lib.secret_password_lookupv_sync(C.byref(self.schema),attrs,None,C.byref(err))
                try:
                    if err.value: raise StorageUnavailable()
                    result=C.string_at(ptr).decode('ascii') if ptr else None
                    if result is not None and not re.fullmatch('[0-9a-f]{64}',result): raise StorageUnavailable()
                    return result
                finally:
                    if ptr: self.lib.secret_password_free(ptr)
                    if err.value: self.g.g_error_free(err)
            result=lookup()
            if result is not None or not create: return result
            result=secrets.token_hex(32); err=C.c_void_p()
            ok=self.lib.secret_password_storev_sync(C.byref(self.schema),attrs,b'default',b'SSC Mod Menu private device',result.encode('ascii'),None,C.byref(err))
            try:
                if not ok or err.value or lookup()!=result: raise StorageUnavailable()
            finally:
                if err.value: self.g.g_error_free(err)
            return result
        finally: self.g.g_hash_table_unref(attrs)

def validate_request(data):
    if not isinstance(data,dict) or set(data)!={'account','create','nonce'}: raise ValueError()
    account=data['account']
    if not isinstance(account,str) or not re.fullmatch('7656119[0-9]{10}',account): raise ValueError()
    if type(data['create']) is not bool or not isinstance(data['nonce'],str) or not re.fullmatch('[0-9a-f]{32}',data['nonce']): raise ValueError()
    return account,data['create'],data['nonce']

class Server(HTTPServer):
    # Single-request serial handling prevents a get/create race within a launch.
    def __init__(self,token,keyring):
        self.token=token; self.keyring=keyring
        super().__init__(('127.0.0.1',0),Handler)

class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args): pass
    def setup(self):
        super().setup(); self.connection.settimeout(3)
    def do_POST(self):
        code=503; result={'status':'unavailable'}
        try:
            if self.path!='/device' or not hmac.compare_digest(self.headers.get('Authorization',''),'Bearer '+self.server.token):
                code=403
            else:
                length=int(self.headers.get('Content-Length','0'))
                if not 0<length<=256 or self.headers.get('Transfer-Encoding'): raise ValueError()
                account,create,nonce=validate_request(json.loads(self.rfile.read(length)))
                seed=self.server.keyring.seed(account,create)
                result={'status':'ok' if seed else 'missing','nonce':nonce}
                if seed: result['seed']=seed
                code=200
        except (ValueError,UnicodeError): code=400; result={'status':'invalid'}
        except Exception: pass # Never log secret values or native keyring error text.
        body=json.dumps(result).encode('ascii')
        try:
            self.send_response(code); self.send_header('Content-Type','application/json'); self.send_header('Cache-Control','no-store'); self.send_header('Content-Length',str(len(body))); self.end_headers(); self.wfile.write(body)
        except (OSError,TimeoutError): pass

def serve(folder):
    folder=Path(folder); st=folder.lstat()
    if not stat.S_ISDIR(st.st_mode) or st.st_uid!=os.getuid() or st.st_mode&0o077: raise StorageUnavailable()
    # Only a random launch capability is written here, never a device seed.
    token=secrets.token_hex(32)
    class LazyKeyring:
        instance=None
        def seed(self,account,create):
            if self.instance is None: self.instance=Keyring()
            return self.instance.seed(account,create)
    with Server(token,LazyKeyring()) as server:
        fd=os.open(folder/'endpoint.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
        with os.fdopen(fd,'w') as f: json.dump({'port':server.server_port,'token':token},f)
        server.serve_forever(poll_interval=.25)

if __name__=='__main__':
    try: serve(sys.argv[1])
    except Exception: sys.exit(1)
