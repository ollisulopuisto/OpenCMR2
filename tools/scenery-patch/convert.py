# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
from PIL import Image
import numpy as np,math,json
from scipy.ndimage import distance_transform_edt


# Preserve silhouette rows at ~16-source-pixel cadence for 256px assets, 8px for 128px.
def mesh(a):
 h,w=a.shape[:2];rs=[];step=max(4,h//32)
 for iy in range(0,h,step):
  ix=np.where(a[iy,:,3]>128)[0]
  if len(ix):rs.append((2*(1-iy/h),(ix.max()+ix.min()+1)/h-w/h,max((ix.max()-ix.min()+1)/h,.018)))
 maxr=max(r[2] for r in rs);ts=np.linspace(0,math.tau,17);arcs=np.r_[0,np.cumsum(np.hypot(np.diff(maxr*np.cos(ts)),np.diff(.65*maxr*np.sin(ts))))];verts=[]
 for yy,cx,r in rs:
  for k in range(17):
   t=k/16*math.tau;verts.append((cx+r*math.cos(t),yy,r*.65*math.sin(t),arcs[k]/2,(2-yy)/2))
 faces=[]
 for j in range(len(rs)-1):
  for k in range(16):
   aa=j*17+k;bb=aa+17;faces.extend([(aa,aa+1,bb),(aa+1,bb+1,bb)])
 return np.array(verts),faces,arcs[-1]/2

root=None # Set to caller-selected empty output directory in main().
def tree(a,rel):
 p=root/rel;p.mkdir(parents=True,exist_ok=True);src=Image.open(a['path']).convert('RGBA');ar=np.array(src);h,w=ar.shape[:2]
 crop=src.crop((int(w*.18),int(h*.15),int(w*.82),int(h*.67)));crop.save(p/'crown.tga',compression=None)
 v,fs,_=mesh(ar);fs=[f for f in fs if all(v[list(f),1]>=.62)]
 v[:,3]*=w/crop.width;v[:,4]=(v[:,4]-.15)*h/crop.height
 # Offline rasterizer compensates texture aspect; bake that into OBJ UVs.
 v[:,3]*=crop.height/crop.width
 v[:,:3]/=2
 trunk=np.array(src.crop((int(.43*w),int(.72*h),int(.57*w)+1,h)));valid=trunk[:,:,3]>128
 if valid.any():
  _,near=distance_transform_edt(~valid,return_indices=True);trunk[~valid]=trunk[near[0][~valid],near[1][~valid]]
 trunk[:,:,3]=255;Image.fromarray(trunk).save(p/'trunk.tga',compression=None)
 lines=['mtllib tree.mtl'];verts=[];tris=[]
 for f in fs:
  pts=v[list(f)].copy();xyz=pts[:,:3];normal=np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0]);normal/=max(np.linalg.norm(normal),1e-10)
  tris.append(('crown',len(verts)+1));verts.extend([(q[:3],(q[3],1-q[4]),normal) for q in pts])
 for j in range(8):
  t=j*math.tau/8;u=(j+1)*math.tau/8;r=.0225*w/h
  q=[(r*math.cos(t),0,r*math.sin(t)),(r*math.cos(t),.32,r*math.sin(t)),(r*math.cos(u),.32,r*math.sin(u)),(r*math.cos(u),0,r*math.sin(u))]
  for f in [(0,1,2),(0,2,3)]:
   tris.append(('trunk',len(verts)+1));nm=(math.cos((t+u)/2),0,math.sin((t+u)/2));uv=[(0,0),(0,1),(1,1),(1,0)];verts.extend([(q[i],uv[i],nm) for i in f])
 for xyz,uv,nm in verts:lines.append('v '+' '.join(f'{x:.8f}' for x in xyz))
 for xyz,uv,nm in verts:lines.append('vt '+' '.join(f'{x:.8f}' for x in uv))
 for xyz,uv,nm in verts:lines.append('vn '+' '.join(f'{x:.8f}' for x in nm))
 last=None
 for mat,i in tris:
  if last!=mat:lines.append('usemtl '+mat);last=mat
  lines.append('f '+' '.join(f'{k}/{k}/{k}' for k in range(i,i+3)))
 (p/'tree.obj').write_text('\n'.join(lines)+'\n');(p/'tree.mtl').write_text('newmtl crown\nKd 1 1 1\nd 1\nmap_Kd crown.tga\nnewmtl trunk\nKd 1 1 1\nd 1\nmap_Kd trunk.tga\n')
 return str(rel/'tree.obj')
def spectator(a,rel):
 p=root/rel;p.mkdir(parents=True,exist_ok=True);src=Image.open(a['path']).convert('RGBA');src=src.resize((src.width*2,src.height*2),Image.Resampling.NEAREST);ar=np.array(src);h,w=ar.shape[:2];frames=[]
 # Subtle upper-body sway: original cutout remapped, lower 30% and feet fixed. No invented limbs.
 yy,xx=np.mgrid[:h,:w]
 for i in range(12):
  weight=np.clip((.70-yy/h)/.70,0,1)**2;dx=np.rint(math.sin(i*math.tau/12)*max(1,w*.009)*weight).astype(int);sx=xx-dx;ok=(sx>=0)&(sx<w);out=np.zeros_like(ar);out[ok]=ar[yy[ok],sx[ok]];path=p/f'frame-{i:02}.tga';Image.fromarray(out).save(path,compression=None);frames.append(str(rel/path.name))
 return frames

def read_archive(path, name, index):
 """Read and verify supported DDS entries without writing any output."""
 import gzip, struct, hashlib
 # Bound decompression before parsing untrusted local archives.
 with gzip.open(path, 'rb') as stream:
  blob = stream.read(256 * 1024 * 1024 + 1)
 if len(blob) > 256 * 1024 * 1024 or len(blob) < 12 or blob[:4] != b'CMPR':
  raise ValueError('Unsupported BFL header or size: ' + name)
 b = blob[8:]
 directory = struct.unpack_from('<I', b, len(b)-4)[0]
 if directory > len(b)-4:
  raise ValueError('BFL directory outside file: ' + name)
 pos = directory
 data = {}
 seen = set()
 while pos < len(b)-4:
  if pos+12 > len(b)-4:
   raise ValueError('Truncated BFL directory')
  size, offset, n = struct.unpack_from('<III', b, pos)
  pos += 12
  padded = (n+3)//4*4
  if n == 0 or n > 255 or pos+padded > len(b)-4:
   raise ValueError('Invalid BFL entry name')
  leaf = b[pos:pos+n].decode('ascii')
  pos += padded
  if '/' in leaf or chr(92) in leaf or '..' in leaf or offset+size > directory or leaf in seen:
   raise ValueError('Unsafe or duplicate BFL entry')
  seen.add(leaf)
  key = name+'/'+leaf
  if key in index:
   raw = b[offset:offset+size]
   if hashlib.sha256(raw).hexdigest() != index[key]['sha256']:
    raise ValueError('Unsupported source texture hash: '+key+'; no output written')
   with Image.open(__import__('io').BytesIO(raw)) as image:
    if image.size != (index[key]['width'], index[key]['height']):
     raise ValueError('Unsupported source dimensions: '+key)
   data[key] = raw
 return data

def main():
 import argparse,tempfile,os
 ap=argparse.ArgumentParser(description='Local procedural asset patcher: requires verified original game files; contains no artwork. Never alters inputs.')
 ap.add_argument('--input-dir',type=Path,required=True)
 ap.add_argument('--output-dir',type=Path,required=True)
 ap.add_argument('--check-only',action='store_true',help='Verify originals without creating output')
 args=ap.parse_args()
 global root
 root=args.output_dir.resolve();inp=args.input_dir.resolve()
 if not inp.is_dir():ap.error('Input must be an existing directory.')
 if root==inp or inp in root.parents:ap.error('Choose output outside original game input directory.')
 if root.exists() and (not root.is_dir() or any(root.iterdir())):ap.error('Output must be new or empty; will not replace existing files.')
 index=json.loads(Path(__file__).with_name('source-index.json').read_text(encoding='utf-8'))
 archives=sorted({key.split('/')[0] for key in index});paths={}
 # Do not traverse symlink directories or read linked files outside the owned input.
 for directory,dirs,files in os.walk(inp,followlinks=False):
  dirs[:]=[d for d in dirs if not (Path(directory)/d).is_symlink()]
  for leaf in files:
   p=Path(directory)/leaf
   if leaf.lower() in archives and not p.is_symlink():paths.setdefault(leaf.lower(),[]).append(p)
 for name in archives:
  if len(paths.get(name,[]))!=1:ap.error(f'Need exactly one {name}; found {len(paths.get(name,[]))}')
 data={}
 for name in archives:data.update(read_archive(paths[name][0],name,index))
 if set(data)!=set(index):raise ValueError('Missing required DDS entries; no output written')
 if args.check_only:
  print(f'Verified {len(data)} source textures. No output written.');return
 # Publish the complete local output only after successful generation.
 root.parent.mkdir(parents=True,exist_ok=True)
 destination=root
 with tempfile.TemporaryDirectory(prefix='.cmr2-patch-',dir=root.parent) as staging:
  root=Path(staging)/'pack';root.mkdir()
  rows=['kind\tsource_archive\tsource_texture\tasset\tframes\tticks'];coverage=[]
  with tempfile.TemporaryDirectory(prefix='cmr2-source-') as tmp:
   for key,rec in index.items():
    archive,leaf=key.split('/');a={**rec,'archive':archive,'texture':leaf};rel=Path(Path(archive).stem)/Path(leaf).stem
    source=Path(tmp)/archive/leaf;source.parent.mkdir(exist_ok=True);source.write_bytes(data[key]);a['path']=str(source)
    if rec.get('category')=='individual-tree':
     asset=tree(a,Path('trees')/rel);rows.append('\t'.join(['tree',archive,leaf,asset,'','1']));status='converted'
    elif rec.get('category')=='spectator':
     frames=spectator(a,Path('spectators')/rel);rows.append('\t'.join(['spectator',archive,leaf,frames[0],','.join(frames),'5']));status='converted'
    else:status='retained-original'
    coverage.append({k:v for k,v in a.items() if k!='path'}|{'status':status})
  (root/'manifest.tsv').write_text('\n'.join(rows)+'\n',encoding='utf-8')
  (root/'coverage.json').write_text(json.dumps(coverage,indent=2)+'\n',encoding='utf-8')
  if destination.exists():destination.rmdir() # fails if another process populated it
  root.rename(destination)
 root=destination
 print(f'Created {len(rows)-1} sidecar entries in {root}. Originals unchanged.')
if __name__=='__main__':main()
