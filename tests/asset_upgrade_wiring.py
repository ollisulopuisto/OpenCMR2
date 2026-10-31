"""Source contract: Finland visual upgrade must never swap contact or physics."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
 def test_finland_never_allocates_contact_or_enables_exact_edges(self):
  text=(ROOT/'game/CurvedRoad.cpp').read_text()
  self.assertIn('auto*hp=fin?nullptr:',text)
  self.assertIn('Track_ResearchExactEdges(!fin)',text)
  self.assertIn('Original(a,"fin01.hpc"',text)
  self.assertIn('Original(a,"fin01fin.dat"',text)
  helper=(ROOT/'tools/scenery-patch/finland1_roads.py').read_text()
  self.assertIn("payloads[variant+'-curved.c3d']=r",helper)
  self.assertNotIn("payloads[variant+'-curved.hpc']",helper)
 def test_package_contains_every_helper_dependency(self):
  text=(ROOT/'tools/package_release.py').read_text()
  for leaf in ['finland1_roads.py','finland1-roads.json','uprez_prepare.py','uprez-textures.json','strip_sources.py','road_periodic.py','export-evidence.py']:self.assertIn(leaf,text)
if __name__=='__main__':unittest.main()
