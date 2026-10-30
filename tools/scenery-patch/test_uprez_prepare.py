import unittest,tempfile,hashlib,io
from pathlib import Path
from unittest.mock import patch
from PIL import Image
import uprez_prepare as u
class Tests(unittest.TestCase):
 def test_runtime_metadata_and_stage_coverage(self):
  import json,re
  root=Path(__file__).resolve().parents[2]
  table=(root/'game/UprezSources.inc').read_text();actual=re.findall(r'\{"([0-9a-f]{64})","([^"]+)","([0-9a-f]{64})"\}',table)
  self.assertEqual(actual,[(r['sha256'],r['output'],r['output_sha256'])for r in u.INDEX])
  self.assertEqual(len({r['output']for r in u.INDEX}),len(u.INDEX))
  for archive in ['frahit.bfl','fralot.bfl']:
   selected=[r for r in u.INDEX if r['archive']==archive and 'temp.c3d' in r.get('usage','')]
   self.assertEqual(len(selected),51)
   self.assertTrue(all(r['repeat']==[False,False]and r['stage']=='fra01'for r in selected))
 def test_fixed_trackside_coverage(self):
  counts={}
  for r in u.INDEX:
   if 'temp.sht' in r.get('usage',''):counts[r['archive']]=counts.get(r['archive'],0)+1
  self.assertEqual(counts,{'finhit.bfl':5,'finlot.bfl':5,'frahit.bfl':20,'fralot.bfl':20})
 def test_source_refusal(self):
  with self.assertRaises(ValueError):u.encode(b'wrong',u.INDEX[0])
 def test_cutout_refusal(self):
  im=Image.new('RGBA',(8,8),(22,33,44,0));buf=io.BytesIO();im.save(buf,format='DDS');raw=buf.getvalue();row=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),width=8,height=8,repeat=[True,True])
  with self.assertRaises(ValueError):u.encode(raw,row)
 def test_cutout_nonsquare_and_mips(self):
  im=Image.new('RGBA',(8,4),(18,90,25,0));im.paste((50,120,30,255),(2,1,6,4));buf=io.BytesIO();im.save(buf,format='DDS');raw=buf.getvalue();row=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),width=8,height=4,repeat=[False,False],kind='full-canvas-cutout')
  import struct
  payload=u.encode(raw,row);self.assertEqual(struct.unpack_from('<III',payload,8),(16,8,5))
  self.assertEqual(len(payload),20+4*(16*8+8*4+4*2+2+1))
  row['repeat']=[True,True]
  with self.assertRaises(ValueError):u.encode(raw,row)
 def test_missing_archive_no_write(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);(root/'game').mkdir()
   with self.assertRaises(ValueError):u.prepare(root/'game',root/'cache')
   self.assertFalse((root/'cache').exists())
 def test_atomic_partial_countries_reuse_tamper(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);data=root/'game';data.mkdir();(data/'finhit.bfl').write_bytes(b'fixture')
   row=dict(archive='finhit.bfl',member='ground.dds',output='ground.rgba',sha256='a'*64,output_sha256=hashlib.sha256(b'rgba').hexdigest())
   with patch.object(u,'INDEX',[row]),patch.object(u._export,'members',return_value=[('ground.dds',b'input')]),patch.object(u,'encode',return_value=b'rgba'):
    pack=u.prepare(data,root/'cache');self.assertEqual(pack,u.prepare(data,root/'cache'));(pack/'ground.rgba').write_bytes(b'tamper')
    with self.assertRaises(ValueError):u.prepare(data,root/'cache')
    self.assertEqual((data/'finhit.bfl').read_bytes(),b'fixture')
if __name__=='__main__':unittest.main()
