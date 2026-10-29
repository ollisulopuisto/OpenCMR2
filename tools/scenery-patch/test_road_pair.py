import struct
import unittest
import road_pair as r

def artifacts():
    points=[(0.,0.,0.),(1.,0.,0.),(0.,0.,1.)]
    h=struct.pack('<ii5hh',0,0,1,0,0,0,0,0)+struct.pack('<hhi',1,0,0)+struct.pack('<HH',3,0)+b''.join(struct.pack('<3i',*(round(x*65536)for x in p))for p in points)+struct.pack('<HH4H',1,0,0,1,2,21)+struct.pack('<ihhh',1,0,1,1)
    c=bytearray(48+288+76+3*52);c[:4]=b'PP_F';struct.pack_into('<II',c,8,len(c),len(c));struct.pack_into('<I',c,20,3);struct.pack_into('<H',c,26,1);struct.pack_into('<I',c,32,1);struct.pack_into('<II',c,60,0,3);struct.pack_into('<II',c,84,0,1);struct.pack_into('<3H',c,48+288+64,0,1,2)
    for i,p in enumerate(points):struct.pack_into('<3f',c,48+288+76+i*48,*p)
    return h,bytes(c)

class PairTests(unittest.TestCase):
 def setUp(self):self.h,self.c=artifacts();self.oh=b'unchanged original HPC';self.oc=b'unchanged original C3D'
 def choose(self,h=None,c=None,verified=True):return r.select(self.oh,self.oc,h,c,identity_verified=verified,visual_only=0)
 def assertOriginal(self,s):self.assertIs(s.contact,self.oh);self.assertIs(s.render,self.oc);self.assertFalse(s.research)
 def test_valid_pair(self):
  self.assertEqual(r.validate_pair(self.h,self.c,0),(1,1));s=self.choose(self.h,self.c);self.assertTrue(s.research)
 def test_partial_never_half_activates(self):
  for h,c in [(None,None),(self.h,None),(None,self.c)]:self.assertOriginal(self.choose(h,c))
 def test_unverified_source_and_invalid_pair(self):
  self.assertOriginal(self.choose(self.h,self.c,False));self.assertOriginal(self.choose(self.h,b'invalid'));self.assertOriginal(self.choose(b'invalid',self.c))
 def test_mismatch_keeps_both_original(self):
  c=bytearray(self.c);struct.pack_into('<f',c,48+288+76,2.);self.assertOriginal(self.choose(self.h,c))
 def test_contact_reference_tail_and_grid_refused(self):
  self.assertOriginal(self.choose(self.h[:-1],self.c))
  self.assertOriginal(self.choose(self.h+b'x',self.c))
  for offset,fmt,value in [(20,'<h',-1),(24,'<i',5),(len(self.h)-6,'<h',1),(len(self.h)-4,'<h',2)]:
   h=bytearray(self.h);struct.pack_into(fmt,h,offset,value);self.assertOriginal(self.choose(h,self.c))
 def test_reversed_winding_refused(self):
  c=bytearray(self.c);struct.pack_into('<3H',c,48+288+64,0,2,1);self.assertOriginal(self.choose(self.h,c))
 def test_nan_and_index_overflow(self):
  for offset,fmt,value in [(48+288+76,'<f',float('nan')),(48+288+64,'<H',3),(60,'<I',0xffffffff)]:
   c=bytearray(self.c);struct.pack_into(fmt,c,offset,value);self.assertOriginal(self.choose(self.h,c))
 def test_truncated_offset_and_unsupported_layout(self):
  for offset,fmt,value in [(8,'<I',0),(24,'<H',1),(16,'<I',1),(26,'<H',175)]:
   c=bytearray(self.c);struct.pack_into(fmt,c,offset,value);self.assertOriginal(self.choose(self.h,c))
  self.assertOriginal(self.choose(self.h,self.c[:-1]))
 def test_prior_success_never_leaks_into_failure(self):
  self.assertTrue(self.choose(self.h,self.c).research);self.assertOriginal(self.choose(self.h,None));self.assertOriginal(self.choose(self.h,self.c,False))
 def test_frozen_snapshot_and_original_unchanged(self):
  c=bytearray(self.c);s=self.choose(self.h,c);c[0]=0;self.assertEqual(s.render,self.c);self.assertEqual(self.oh,b'unchanged original HPC')
if __name__=='__main__':unittest.main()
