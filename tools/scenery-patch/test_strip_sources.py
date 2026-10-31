import unittest,struct,hashlib,gzip,io
import strip_sources as s
def c3d(flags=0x800,tex=0,stem='bsh_sh1a',tris=((0,1,2),(0,2,3))):
 b=bytearray(2048)
 b[0:4]=b'PP_F'
 struct.pack_into('<i',b,0xc,1024)          # texture names offset
 struct.pack_into('<3H',b,0x1a,1,0,0)       # 1 mesh, 0 objects, 0 sectors
 struct.pack_into('<I',b,0x20,len(tris))    # triangles
 struct.pack_into('<H',b,0x24,1)            # textures
 m=48
 struct.pack_into('<I',b,m+12,0)            # vertex offset
 struct.pack_into('<II',b,m+36,0,len(tris)) # triangle offset/count
 struct.pack_into('<I',b,m+48,flags)
 ts=48+288
 for i,t in enumerate(tris):
  off=ts+i*76
  struct.pack_into('<i',b,off+4,tex)
  struct.pack_into('<3H',b,off+64,*t)
 vs=ts+len(tris)*76
 for j in range(4):
  base=vs+j*48
  struct.pack_into('<3f',b,base,float(j),float(j*j),1.0)
  struct.pack_into('<2f',b,base+32,j*0.25,0.5)
 b[1024:1024+len(stem)]=stem.encode()
 return bytes(b),vs
def archive(payload,member='temp.sht'):
 body=bytearray(payload)
 directory=len(body)
 entry=struct.pack('<III',len(payload),0,len(member))+member.encode()+b'\x00'*((4-len(member)%4)%4)
 blob=b'CMPR'+bytes(4)+body+entry+struct.pack('<I',directory)
 out=io.BytesIO()
 with gzip.GzipFile(fileobj=out,mode='wb')as f:f.write(blob)
 return out.getvalue()
class Tests(unittest.TestCase):
 def test_quad_hash_matches_reference(self):
  blob,vs=c3d()
  rows=list(s.strip_hashes(archive(blob),'temp.sht',['bsh_sh1']))
  self.assertEqual(len(rows),1)
  stem,digest=rows[0]
  self.assertEqual(stem,'bsh_sh1a')
  h=hashlib.sha256()
  raw=blob
  for j in (0,1,2,0,2,3):
   base=vs+j*48
   h.update(raw[base:base+12]);h.update(raw[base+32:base+40])
  self.assertEqual(digest,h.hexdigest())
 def test_gate_and_stem_exclusions(self):
  blob,_=c3d(flags=0)                 # no 0x800: not a strip mesh
  self.assertEqual(list(s.strip_hashes(archive(blob),'temp.sht',['bsh_sh1'])),[])
  blob,_=c3d(stem='roc_01a')          # stem not requested
  self.assertEqual(list(s.strip_hashes(archive(blob),'temp.sht',['bsh_sh1'])),[])
  blob,_=c3d(stem='roc_01a')
  self.assertEqual(len(list(s.strip_hashes(archive(blob),'temp.sht',['roc_01']))),1)
if __name__=='__main__':unittest.main()
