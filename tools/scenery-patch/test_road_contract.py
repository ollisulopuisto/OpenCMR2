import copy
import hashlib
import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path
import road_contract as r

class ContractTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name).resolve();self.source=self.root/'original';self.art=self.root/'artifacts';self.source.mkdir();self.art.mkdir()
  names=['fra01.hpc','fra01fin.dat','fra01.srf','temp.c3d']
  for name in names:(self.source/name).write_bytes(name.encode())
  (self.art/'curved.hpc').write_bytes(struct.pack('<ii5hh',0,0,1,0,0,0,0,0)+struct.pack('<hhi',1,0,0)+struct.pack('<HH',3,0)+bytes(36)+struct.pack('<HH4H',1,0,0,1,2,21)+struct.pack('<ihhh',1,0,1,1))
  self.mock=patch.object(r,'SOURCE_HASHES',{n:r.digest(self.source/n)for n in names});self.mock.start()
  self.d=dict(schema=r.SCHEMA,runtime_enable=False,stage='fra01',source_archive='fra01hi (1).bfl',maximum_offset_m=.35,collision_triangles=1,collision_vertices=3,fin_policy='original-unverified-alignment',sources=[dict(member=n,sha256=r.digest(self.source/n))for n in names],outputs=[dict(member='curved.hpc',sha256=r.digest(self.art/'curved.hpc'))])
 def tearDown(self):self.mock.stop();self.tmp.cleanup()
 def test_prepare_reuse_no_source_writes(self):
  before={p.name:p.read_bytes()for p in self.source.iterdir()};p=r.prepare(self.d,self.source,self.art,self.root/'cache');self.assertEqual(p,r.prepare(self.d,self.source,self.art,self.root/'cache'));self.assertEqual(before,{p.name:p.read_bytes()for p in self.source.iterdir()});self.assertFalse((p/'manifest.tsv').exists())
 def test_tamper_blocks_reuse(self):
  p=r.prepare(self.d,self.source,self.art,self.root/'cache');(p/'curved.hpc').write_bytes(b'changed')
  with self.assertRaises(ValueError):r.prepare(self.d,self.source,self.art,self.root/'cache')
 def test_source_mismatch_before_cache_write(self):
  (self.source/'fra01.hpc').write_bytes(b'wrong')
  with self.assertRaises(ValueError):r.prepare(self.d,self.source,self.art,self.root/'cache')
  self.assertFalse((self.root/'cache').exists())
 def test_self_asserted_source_hash_not_trusted(self):
  (self.source/'fra01.hpc').write_bytes(b'other stage');self.d['sources'][0]['sha256']=r.digest(self.source/'fra01.hpc')
  with self.assertRaises(ValueError):r.validate(self.d,self.source,self.art)
 def test_malformed_quadtree_not_trusted(self):
  for count,start in [(-1,0),(1,3),(-2,0)]:
   raw=bytearray((self.art/'curved.hpc').read_bytes());struct.pack_into('<hhi',raw,20,count,0,start);(self.art/'curved.hpc').write_bytes(raw);self.d['outputs'][0]['sha256']=r.digest(self.art/'curved.hpc')
   with self.assertRaises(ValueError):r.validate(self.d,self.source,self.art)
 def test_runtime_never_enabled(self):
  with self.assertRaises(ValueError):r.require_runtime(self.d)
  self.d['runtime_enable']=True
  with self.assertRaises(ValueError):r.validate(self.d,self.source,self.art)
 def test_limits_paths_and_fin(self):
  for key,value in [('collision_triangles',32768),('collision_vertices',65536),('maximum_offset_m',.36),('fin_policy','warp-by-proximity'),('stage','fin10')]:
   d=copy.deepcopy(self.d);d[key]=value
   with self.assertRaises(ValueError):r.validate(d,self.source,self.art)
  for name in ['../escape','/absolute','x\\y']:
   d=copy.deepcopy(self.d);d['outputs'][0]['member']=name
   with self.assertRaises(ValueError):r.validate(d,self.source,self.art)
 def test_symlink_and_source_cache_refused(self):
  (self.root/'link').symlink_to(self.root/'other',target_is_directory=True)
  with self.assertRaises(ValueError):r.prepare(self.d,self.source,self.art,self.root/'link')
  with self.assertRaises(ValueError):r.prepare(self.d,self.source,self.art,self.source/'cache')
 def test_symlink_cache_ancestor_refused(self):
  other=self.root/"other";other.mkdir();(self.root/"alias").symlink_to(other,target_is_directory=True)
  with self.assertRaisesRegex(ValueError,"Symlink"):r.prepare(self.d,self.source,self.art,self.root/"alias"/"cache")
 def test_key_covers_recipe_sources_and_outputs(self):
  key=r.cache_key(self.d)
  for kind in ['sources','outputs']:
   d=copy.deepcopy(self.d);d[kind][0]['sha256']='0'*64;self.assertNotEqual(key,r.cache_key(d))
if __name__=='__main__':unittest.main()
