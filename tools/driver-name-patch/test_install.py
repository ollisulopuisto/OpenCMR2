import unittest,tempfile,hashlib
from pathlib import Path
from install import install
class Safety(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
  self.root=Path(self.tmp.name);self.game=self.root/'original';self.game.mkdir()
  for name in ['Game','FrontEnd','Sounds','CountrySpecific','SetupRep']:(self.game/name).mkdir()
  (self.game/'Game'/'original.bin').write_bytes(b'original data')
 def hashes(self):return {str(p.relative_to(self.game)):hashlib.sha256(p.read_bytes()).hexdigest() for p in self.game.rglob('*') if p.is_file()}
 def test_separate_and_repeat(self):
  before=self.hashes();target=install(self.game,self.root/'private')
  self.assertEqual(len(target.read_text().splitlines()),16);self.assertEqual(before,self.hashes())
  with self.assertRaises(FileExistsError):install(self.game,self.root/'private')
  self.assertEqual(before,self.hashes())
 def test_inside(self):
  for output in [self.game,self.game/'pack']:
   with self.assertRaises(ValueError):install(self.game,output)
 def test_symlink(self):
  private=self.root/'private';private.mkdir();target=self.game/'Game'/'original.bin'
  (private/'driver-names.tsv').symlink_to(target)
  with self.assertRaises(FileExistsError):install(self.game,private)
  self.assertEqual(target.read_bytes(),b'original data')
if __name__=='__main__':unittest.main()
