import unittest, math, tempfile, hashlib
from pathlib import Path
from test_preview_trees import fake_tree
import tree_rows as r
class RowTests(unittest.TestCase):
 def mesh(self):
  return [('trunk',(-.2,0,-.1),(0,0),(0,0,1)),('crown',(.2,1,.1),(1,1),(0,0,1)),('crown',(-.2,1,.1),(0,1),(0,0,1))]
 def test_five_separate_preserved_material_uv_normal_trees(self):
  a=r.compose([self.mesh()]*3);self.assertEqual(len(a),15)
  self.assertEqual(a,r.compose([self.mesh()]*3))
  self.assertEqual({m for m,p,uv,n in a},{f'{i}_{m}' for i in range(5) for m in ('trunk','crown')})
  self.assertEqual([uv for m,p,uv,n in a],[(0,0),(1,1),(0,1)]*5)
  for m,p,uv,n in a:
   self.assertTrue(all(math.isfinite(v) for v in p));self.assertLessEqual(abs(p[0]),.5);self.assertGreaterEqual(p[1],0);self.assertLessEqual(p[1],1);self.assertLessEqual(abs(p[2]),.5)
   self.assertAlmostEqual(sum(v*v for v in n),1)
  self.assertEqual(sum(p[1]==0 for m,p,uv,n in a),5)
 def test_generated_pack_materials_identities_and_loader_bounds(self):
  raw=fake_tree();keys=["finhit.bfl/"+n+".dds" for names in r.ROWS.values() for n in names]+[a+"/"+n+".dds" for a in ("finhit.bfl","finlot.bfl") for n in r.ROWS]
  data={k:raw for k in keys};index={k:{"sha256":hashlib.sha256(raw).hexdigest()} for k in keys}
  with tempfile.TemporaryDirectory() as td:
   out=Path(td);r.generate(data,index,out)
   self.assertEqual(len((out/"manifest.tsv").read_text().splitlines()),5)
   self.assertEqual(len((out/"source-identities.tsv").read_text().splitlines()),4)
   for strip in r.ROWS:
    folder=out/"rows"/strip;obj=(folder/"row.obj").read_text();mtl=(folder/"row.mtl").read_text()
    for i in range(5):self.assertIn(f"usemtl {i}_trunk",obj)
    for line in mtl.splitlines():
     if line.startswith("map_Kd "):self.assertTrue((folder/line.split()[1]).is_file())
    self.assertLess(sum(s.startswith("v ") for s in obj.splitlines()),65536)
 def test_two_explicit_source_rows_not_fabricated_segmentation(self):
  self.assertEqual(set(r.ROWS),{'tresh1ba','tresh2fa'});self.assertEqual(len(r.PLACEMENTS),5)
if __name__=='__main__':unittest.main()
