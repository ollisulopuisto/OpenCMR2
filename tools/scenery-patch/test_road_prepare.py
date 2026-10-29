import unittest,tempfile,gzip,struct,hashlib,json
from pathlib import Path
from unittest.mock import patch
import road_prepare as r
class PrepareTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name);self.data=self.root/'owned';self.data.mkdir();self.cache=self.root/'cache'
 def tearDown(self):self.tmp.cleanup()
 def archive(self):
  raw=b'original';name=b'temp.c3d';blob=raw;offset=len(blob);blob+=struct.pack('<III',len(raw),0,len(name))+name+bytes((-len(name))%4);blob+=struct.pack('<I',offset)
  with gzip.open(self.data/'fra01hi.bfl','wb')as f:f.write(b'CMPR'+bytes(4)+blob)
  return raw
 def test_missing_source_refused_no_cache(self):
  with self.assertRaises(ValueError):r.prepare(self.data,self.cache)
  self.assertFalse(self.cache.exists())
 def test_wrong_member_hash_no_cache(self):
  self.archive()
  with self.assertRaises(ValueError):r.prepare(self.data,self.cache)
  self.assertFalse(self.cache.exists())
 def test_source_cache_overlap(self):
  with self.assertRaises(ValueError):r.prepare(self.data,self.data/'cache')
 def test_atomic_reuse_and_tamper_no_original_changes(self):
  raw=self.archive();before=(self.data/'fra01hi.bfl').read_bytes();hashes={'temp.c3d':hashlib.sha256(raw).hexdigest()};outputs={'curved.hpc':hashlib.sha256(b'hpc').hexdigest(),'curved.c3d':hashlib.sha256(b'c3d').hexdigest()}
  with patch.object(r,'SOURCE_HASHES',hashes),patch.object(r,'OUTPUT_HASHES',outputs),patch.object(r,'build',return_value=(b'hpc',b'c3d')),patch.object(r,'validate_pair'):
   p=r.prepare(self.data,self.cache);self.assertEqual(p,r.prepare(self.data,self.cache));(p/'curved.hpc').write_bytes(b'changed')
   # Even a forged cache marker cannot override compiled output identity.
   marker=json.loads((p/'prepared.json').read_text());marker['curved.hpc']=hashlib.sha256(b'changed').hexdigest();(p/'prepared.json').write_text(json.dumps(marker))
   with self.assertRaises(ValueError):r.prepare(self.data,self.cache)
  self.assertEqual(before,(self.data/'fra01hi.bfl').read_bytes())
 def test_failed_generation_no_partial_pair(self):
  raw=self.archive()
  with patch.object(r,'SOURCE_HASHES',{'temp.c3d':hashlib.sha256(raw).hexdigest()}),patch.object(r,'build',side_effect=ValueError('failed')):
   with self.assertRaises(ValueError):r.prepare(self.data,self.cache)
  self.assertEqual(list(self.cache.iterdir()),[])
if __name__=='__main__':unittest.main()
