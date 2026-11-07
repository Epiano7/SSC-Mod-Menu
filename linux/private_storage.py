"""Per-launch loopback bridge to per-user private-access device storage.
No activation codes or grants are handled here. Keyring import is optional.
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

class DeviceStore:
    """Small, account-separated local files; no desktop keyring dependency."""
    def __init__(self, folder=None):
        base=Path(os.environ.get('XDG_DATA_HOME',str(Path.home()/'.local/share')))
        self.folder=Path(folder) if folder is not None else base/'ssc-mod-menu/private-access'
        if not self.folder.is_absolute(): raise StorageUnavailable()
    @contextlib.contextmanager
    def locked(self):
        import fcntl
        try: self.folder.mkdir(parents=True,mode=0o700)
        except FileExistsError: pass
        self.check(self.folder.lstat(),directory=True)
        fd=os.open(self.folder/'lock',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW|os.O_NONBLOCK,0o600)
        try:
            self.check(os.fstat(fd));fcntl.flock(fd,fcntl.LOCK_EX)
            yield
        finally: os.close(fd)
    @staticmethod
    def check(st,directory=False):
        valid=stat.S_ISDIR(st.st_mode) if directory else stat.S_ISREG(st.st_mode) and st.st_nlink==1
        if not valid or st.st_uid!=os.getuid() or st.st_mode&0o077: raise StorageUnavailable()
    @staticmethod
    def account(value):
        if not isinstance(value,str) or not re.fullmatch('7656119[0-9]{10}',value): raise StorageUnavailable()
        return value
    def read(self,account):
        path=self.folder/(account+'.json')
        try: fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
        except FileNotFoundError: return None
        try:
            st=os.fstat(fd);self.check(st)
            if st.st_size>512: raise StorageUnavailable()
            data=json.loads(os.read(fd,513))
            if not isinstance(data,dict) or set(data)!={'schema','account','seed'} or type(data['schema']) is not int or data['schema']!=1 or data['account']!=account or not isinstance(data['seed'],str) or not re.fullmatch('[0-9a-f]{64}',data['seed']): raise StorageUnavailable()
            return data['seed']
        finally: os.close(fd)
    def write(self,account,seed):
        fd,tmp=tempfile.mkstemp(prefix='.device-',dir=self.folder)
        try:
            with os.fdopen(fd,'w') as f:
                json.dump({'schema':1,'account':account,'seed':seed},f);f.flush();os.fsync(f.fileno())
            os.replace(tmp,self.folder/(account+'.json'))
        finally:
            if os.path.exists(tmp): os.unlink(tmp)
    def seed(self,account,create):
        account=self.account(account)
        with self.locked():
            result=self.read(account)
            if result is not None or not create: return result
            result=secrets.token_hex(32);self.write(account,result)
            if self.read(account)!=result: raise StorageUnavailable()
            return result
    def import_keyring(self,account,keyring=None):
        account=self.account(account)
        # Called only by the explicit migration action, never on game startup.
        result=(keyring or Keyring()).seed(account,False)
        if result is None: raise StorageUnavailable('No previous keyring approval was found for the selected Steam account')
        if not re.fullmatch('[0-9a-f]{64}',result): raise StorageUnavailable()
        with self.locked():
            existing=self.read(account)
            if existing is not None and existing!=result: raise StorageUnavailable('A different local approval already exists; it has not been overwritten')
            if existing is None: self.write(account,result)
        return True

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
    with Server(token,DeviceStore()) as server:
        fd=os.open(folder/'endpoint.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
        with os.fdopen(fd,'w') as f: json.dump({'port':server.server_port,'token':token},f)
        server.serve_forever(poll_interval=.25)

if __name__=='__main__':
    try: serve(sys.argv[1])
    except Exception: sys.exit(1)
