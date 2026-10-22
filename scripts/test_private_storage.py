import sys,unittest,tempfile,threading,json,http.client,os
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'linux'))
from private_storage import Keyring,Server,validate_request,StorageUnavailable
A='76561198025298853';B='76561198029717676'
class Tests(unittest.TestCase):
 def test_request_rejections(self):
  valid={'account':A,'create':True,'nonce':'a'*32}
  self.assertEqual(validate_request(valid),(A,True,'a'*32))
  for change in [{'account':'../bad'},{'create':1},{'nonce':'bad'},{'extra':True}]:
   with self.assertRaises(ValueError):validate_request(dict(valid,**change))
 def test_loopback_requires_capability(self):
  class Fake:
   calls=0
   def seed(self,a,c):self.calls+=1;return 'a'*64
  fake=Fake()
  with Server('b'*64,fake) as server:
   thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
   def send(token,payload):
    c=http.client.HTTPConnection('127.0.0.1',server.server_port,timeout=3);c.request('POST','/device',json.dumps(payload),{'Authorization':'Bearer '+token});r=c.getresponse();status=r.status;body=json.loads(r.read());c.close();return status,body
   valid={'account':A,'create':True,'nonce':'a'*32}
   self.assertEqual(send('wrong',valid)[0],403);self.assertEqual(fake.calls,0)
   self.assertEqual(send('b'*64,dict(valid,create=1))[0],400);self.assertEqual(fake.calls,0)
   status,body=send('b'*64,valid);self.assertEqual(status,200);self.assertEqual(body,{'status':'ok','nonce':'a'*32,'seed':'a'*64});self.assertEqual(fake.calls,1)
   server.shutdown();thread.join()
 def test_keyring_fail_closed(self):
  class Failed:
   def seed(self,a,c):raise StorageUnavailable('secret error must never leak')
  with Server('b'*64,Failed()) as server:
   t=threading.Thread(target=server.serve_forever,daemon=True);t.start()
   c=http.client.HTTPConnection('127.0.0.1',server.server_port,timeout=3);c.request('POST','/device',json.dumps({'account':A,'create':True,'nonce':'a'*32}),{'Authorization':'Bearer '+'b'*64});r=c.getresponse();self.assertEqual(r.status,503);self.assertEqual(json.loads(r.read()),{'status':'unavailable'});c.close();server.shutdown();t.join()
 @unittest.skipUnless(os.environ.get('SSC_TEST_REAL_KEYRING')=='1','native keyring integration requested separately')
 def test_native_keyring_persistence_and_account_separation(self):
  ring=Keyring();a=ring.seed(A,True);self.assertRegex(a,'^[0-9a-f]{64}$');self.assertEqual(Keyring().seed(A,False),a)
  b=ring.seed(B,True);self.assertNotEqual(a,b);self.assertEqual(Keyring().seed(B,False),b)
if __name__=='__main__':unittest.main()
