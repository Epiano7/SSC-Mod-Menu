import sys,unittest,tempfile,json,hashlib,os
from pathlib import Path
from unittest import mock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'linux'))
import ssc_installer as core

class RecoveryTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);self.game=self.root/'game';self.game.mkdir();self.package=self.root/'package';self.package.mkdir()
  game=b'non-executable game fixture';(self.game/'SkillshotCity.exe').write_bytes(game)
  files={}
  for name in core.OWNED:
   p=self.package/'payload'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(('new '+name).encode());files[name]=core.digest(p)
  (self.package/'package.json').write_text(json.dumps({'schema':1,'product':'SSCMods','version':'0.1.33','files':files,'game_size':len(game),'game_sha256':hashlib.sha256(game).hexdigest()}))
  self.env=mock.patch.dict(os.environ,{'XDG_STATE_HOME':str(self.root/'state')});self.env.start()
  self.closed=mock.patch.object(core,'require_closed');self.closed.start()
 def tearDown(self):self.closed.stop();self.env.stop();self.temp.cleanup()
 def orphan(self):
  (self.game/'opengl32.dll').write_bytes(b'previous unknown loader');m=self.game/'SSCMods';m.mkdir();(m/'runtime.dll').write_bytes(b'previous mod')
  (m/'menu.ini').write_bytes(b'user settings');(m/'unrelated.dat').write_bytes(b'keep this')
 def test_orphan_refused_without_repair(self):
  self.orphan()
  with self.assertRaisesRegex(ValueError,'Use Repair'):core.install(self.package,self.game)
  self.assertEqual((self.game/'opengl32.dll').read_bytes(),b'previous unknown loader');self.assertIsNone(core.installed(self.game))
 def test_repair_backups_update_and_uninstall(self):
  self.orphan();result=core.install(self.package,self.game,repair=True);self.assertIn('Backup:',result)
  backups=list((self.root/'state/ssc-mod-menu/backups').iterdir());self.assertEqual(len(backups),1)
  self.assertEqual((backups[0]/'opengl32.dll').read_bytes(),b'previous unknown loader');self.assertEqual((backups[0]/'SSCMods/runtime.dll').read_bytes(),b'previous mod')
  self.assertIsNotNone(core.installed(self.game));core.install(self.package,self.game);core.uninstall(self.game)
  self.assertFalse((self.game/'opengl32.dll').exists());self.assertEqual((self.game/'SSCMods/menu.ini').read_bytes(),b'user settings');self.assertEqual((self.game/'SSCMods/unrelated.dat').read_bytes(),b'keep this')
 def test_repair_rollback(self):
  self.orphan();write=core.atomic_write;calls=0
  def fail(path,body):
   nonlocal calls
   calls+=1
   if calls==2:raise OSError('simulated disk failure')
   return write(path,body)
  with mock.patch.object(core,'atomic_write',side_effect=fail):
   with self.assertRaises(OSError):core.install(self.package,self.game,repair=True)
  self.assertEqual((self.game/'opengl32.dll').read_bytes(),b'previous unknown loader');self.assertEqual((self.game/'SSCMods/runtime.dll').read_bytes(),b'previous mod');self.assertIsNone(core.installed(self.game))
 def test_wrong_game_and_bad_manifest_not_bypassed(self):
  self.orphan();(self.game/'SkillshotCity.exe').write_bytes(b'changed')
  with self.assertRaisesRegex(ValueError,'newer mod package'):core.install(self.package,self.game,repair=True)
  self.assertEqual((self.game/'opengl32.dll').read_bytes(),b'previous unknown loader')
 def test_invalid_ownership_not_bypassed(self):
  self.orphan();(self.game/core.MANIFEST).write_text('{}')
  with self.assertRaisesRegex(ValueError,'Invalid ownership'):core.install(self.package,self.game,repair=True)
 @unittest.skipUnless(os.name=='posix','native Linux symlinks')
 def test_linked_directory_refused(self):
  outside=self.root/'outside';outside.mkdir();(outside/'runtime.dll').write_bytes(b'keep');(self.game/'SSCMods').symlink_to(outside,target_is_directory=True)
  with self.assertRaisesRegex(ValueError,'linked mod'):core.install(self.package,self.game,repair=True)
  self.assertEqual((outside/'runtime.dll').read_bytes(),b'keep')
if __name__=='__main__':unittest.main()
