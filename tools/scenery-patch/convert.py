# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
from PIL import Image
import numpy as np,math,json


# Full-canvas source-fit contract. No restored artwork is bundled or fetched.
TREE_CANDIDATES={('frahit.bfl','tre_md1'),('frahit.bfl','tre_md4'),('finhit.bfl','tre_fir1')}
root=None
def tree(a,rel):
 name=Path(a['texture']).stem;src=Image.open(a['path']).convert('RGBA');a=np.array(src);h,w=a.shape[:2];p=root/rel;p.mkdir(parents=True,exist_ok=True)
 # Source projection, not circumference repetition. Source alpha is retained.
 crown_bottom=.74 if name.startswith('tre_md') else .88
 cut=round(h*crown_bottom)
 # Opaque stem-only texels; no nearest-fill transparency or ground colors.
 bark=[]
 for y in range(round(h*.78),round(h*.94)):
  ix=np.where(a[y,:,3]>=220)[0];ix=ix[(ix>w*.3)&(ix<w*.7)]
  if len(ix)<2:continue
  center=int(np.median(ix));half=max(1,min(3,len(ix)//4));row=a[y,max(0,center-half):center+half+1];row=row[row[:,3]>=220]
  if len(row):bark.append(row[np.linspace(0,len(row)-1,16).astype(int)])
 if not bark:raise ValueError('No opaque stem data '+name)
 ba=np.array(bark);ba[:,:,3]=255;Image.fromarray(ba).resize((64,len(ba)*4),Image.Resampling.LANCZOS).save(p/'trunk.tga')
 # Alpha outline shapes the shell. Model horizontal extent exactly supplied source W/H.
 rows=[]
 for y in range(0,cut,max(2,h//64)):
  ix=np.where(a[y,:,3]>=128)[0]
  if len(ix):rows.append((y,ix.min(),ix.max()+1))
 if len(rows)<2:raise ValueError("Insufficient crown silhouette "+name)
 extent=w
 verts=[]
 for y,lo,hi in rows:
  cx=((lo+hi)/2-w/2)/h;r=(hi-lo)/extent*(w/h)/2
  for k in range(17):
   t=k*math.tau/16;verts.append((cx+r*math.cos(t),1-y/h,r*.8*math.sin(t),y,lo,hi))
 triangles=[]
 for j in range(len(rows)-1):
  for k in range(16):
   aa=j*17+k;bb=aa+17;triangles.extend([(aa,aa+1,bb),(aa+1,bb+1,bb)])
 out=[];faces=[]
 for f in triangles:
  q=np.array([verts[i][:3] for i in f]);nm=np.cross(q[1]-q[0],q[2]-q[0]);nm/=max(np.linalg.norm(nm),1e-12)
  faces.append(('crown',len(out)+1))
  for idx in f:
   x,y,z,sy,lo,hi=verts[idx];theta=(idx%17)/16
   out.append(((x,y,z),(theta,min(.999,max(.001,1-sy/cut))),nm))
 # Bake a continuous cross-projection atlas, not repeated image stripes.
 # Adjacent shell faces share exactly the same texture coordinates at seams.
 color=np.array(src.convert('RGBa').resize((w*4,h*4),Image.Resampling.LANCZOS).convert('RGBA')).astype(float)/255
 hh,ww=color.shape[:2];aw=1024;ah=cut*4;atlas=np.zeros((ah,aw,4),np.uint8)
 ts=np.arange(aw)/aw*math.tau;co=np.cos(ts);si=np.sin(ts)
 # Smooth, symmetric source image projection for front/back and side.
 wx=np.abs(si)**4;wz=np.abs(co)**4;den=np.maximum(wx+wz,1e-8)
 for iy in range(ah):
  sy=iy/4;ri=min(range(len(rows)),key=lambda k:abs(rows[k][0]-sy));_,lo,hi=rows[ri];cx=((lo+hi)/2-w/2)/h;r=(hi-lo)/h/2
  xx=cx+r*co;zz=r*si
  u0=np.clip((.5+xx*h/w)*ww,0,ww-1).astype(int);u1=np.clip((.5+zz*h/w)*ww,0,ww-1).astype(int);yy=min(hh-1,round(sy*hh/h));ca=color[yy,u0];cb=color[yy,u1]
  alpha=(ca[:,3]*wx+cb[:,3]*wz)/den;prem=(ca[:,:3]*ca[:,3,None]*wx[:,None]+cb[:,:3]*cb[:,3,None]*wz[:,None])/den[:,None];rgb=prem/np.maximum(alpha[:,None],1e-6)
  atlas[iy,:,:3]=np.clip(rgb*255,0,255).astype('uint8');atlas[iy,:,3]=np.clip(alpha*255,0,255).astype('uint8')
 Image.fromarray(atlas).save(p/'crown.tga')
 # Stem diameter measured from source opaque pixels, capped at 6% source width.
 stemwidth=min(w*.06,np.median([np.sum(a[y,:,3]>=220) for y in range(round(h*.82),round(h*.90))]))
 radius=max(.005,stemwidth/extent*(w/h)/2);stemtop=1-crown_bottom+.035
 for j in range(8):
  t=j*math.tau/8;v=(j+1)*math.tau/8;q=[(radius*math.cos(t),0,radius*math.sin(t)),(radius*math.cos(t)*.7,stemtop,radius*.7*math.sin(t)),(radius*math.cos(v)*.7,stemtop,radius*.7*math.sin(v)),(radius*math.cos(v),0,radius*math.sin(v))];uv=[(0,0),(0,1),(1,1),(1,0)];nm=(math.cos((t+v)/2),0,math.sin((t+v)/2))
  for f in [(0,1,2),(0,2,3)]:faces.append(('trunk',len(out)+1));out.extend([(q[i],uv[i],nm) for i in f])
 lines=[f'# opencmr2-source-canvas-1 {w} {h}','mtllib tree.mtl']
 for kind,j in [('v',0),('vt',1),('vn',2)]:lines += [kind+' '+' '.join(f'{v:.8f}' for v in q[j]) for q in out]
 last=None
 for mat,i in faces:
  if mat!=last:lines.append('usemtl '+mat);last=mat
  lines.append('f '+' '.join(f'{k}/{k}/{k}' for k in range(i,i+3)))
 (p/'source-vertices.json').write_text(json.dumps({'schema':'source-uv-per-obj-v-1','source_canvas_wh':[w,h],'vertices':[{'obj_v':i+1,'source_uv':[.5+float(q[0][0])*h/w,1-float(q[0][1])]} for i,q in enumerate(out)]},indent=2))
 (p/'recipe.json').write_text(json.dumps({'schema':'source-canvas-tree-1','source_canvas_wh':[w,h],'source_mapping':str(rel),'method':'4x premultiplied interpolation, no new detail; continuous cross-projection atlas','triangles':len(faces),'crown_pixels':[aw,ah],'stem_radius':float(radius)},indent=2)+'\n')
 (p/'tree.obj').write_text('\n'.join(lines)+'\n');(p/'tree.mtl').write_text('newmtl crown\nKd 1 1 1\nd 1\nmap_Kd crown.tga\nnewmtl trunk\nKd 1 1 1\nd 1\nmap_Kd trunk.tga\n')
 return str(rel/'tree.obj')

def crossed_tree(a,rel):
 # Explicit opt-in cutout candidate, not a full3Dtree or private restored art.
 asset=tree(a,rel);p=root/rel;src=Image.open(a['path']).convert('RGBA');ar=np.array(src);h,w=ar.shape[:2]
 name=Path(a['texture']).stem;broad=name.startswith('tre_md')
 if broad:
  yy,xx=np.mgrid[:h,:w];ar[(yy>h*.72)&(abs(xx-w/2)<w*.08),3]=0
  ar[round(h*.94):,:,3]=0 # discard source ground grass, not tree foliage
 Image.fromarray(ar).convert('RGBa').resize((w*4,h*4),Image.Resampling.LANCZOS).convert('RGBA').save(p/'crossed.tga')
 old=(p/'tree.obj').read_text().splitlines();op=[];ot=[];on=[];stem=[];material=''
 for line in old:
  a=line.split()
  if not a:continue
  if a[0]=='v':op.append(a[1:])
  elif a[0]=='vt':ot.append(a[1:])
  elif a[0]=='vn':on.append(a[1:])
  elif a[0]=='usemtl':material=a[1]
  elif a[0]=='f' and material=='trunk':stem.append([list(map(int,x.split('/'))) for x in a[1:]])
 points=[];uv=[];normals=[[0,0,1]];faces=[]
 for k in range(4):
  theta=k*math.pi/4;co,si=math.cos(theta),math.sin(theta);start=len(points)
  for j in range(9):
   y=j/8;depth=.025*math.sin(j*1.7+k)*min(1,y*5)
   for x in [-w/h/2,w/h/2]:points.append([x*co-depth*si,y,x*si+depth*co]);uv.append([0 if x<0 else 1,y])
  for j in range(8):
   v=start+j*2;faces.extend([(v,v+1,v+2),(v+1,v+3,v+2)])
 lines=[f'# opencmr2-source-canvas-1 {w} {h}','mtllib tree.mtl'];ff=['usemtl crown']
 for face in faces:ff.append('f '+' '.join(f'{v+1}/{v+1}/1' for v in face))
 if broad:
  ff.append('usemtl trunk')
  for face in stem:
   ids=[]
   for v,t,n in face:
    points.append(list(map(float,op[v-1])));uv.append(list(map(float,ot[t-1])));normals.append(list(map(float,on[n-1])));ids.append(f'{len(points)}/{len(uv)}/{len(normals)}')
   ff.append('f '+' '.join(ids))
 lines+=['v '+' '.join(map(str,v)) for v in points]+['vt '+' '.join(map(str,v)) for v in uv]+['vn '+' '.join(map(str,v)) for v in normals]+ff
 (p/'tree.obj').write_text('\n'.join(lines)+'\n')
 (p/'tree.mtl').write_text('newmtl crown\nKd 1 1 1\nd 1\nmap_Kd crossed.tga\nnewmtl trunk\nKd 1 1 1\nd 1\nmap_Kd trunk.tga\n')
 (p/'source-vertices.json').write_text(json.dumps({'schema':'source-uv-per-obj-v-1','source_canvas_wh':[w,h],'vertices':[{'source_uv':[.5+v[0]*h/w,1-v[1]]}for v in points]},indent=2))
 metadata=json.loads((p/'recipe.json').read_text());metadata.update(method='4x premultiplied interpolation, no new detail; fixed source-projection cutouts',crown_pixels=[w*4,h*4],geometry='four fixed intersecting cutout planes, not full3D',triangles=len(faces)+(len(stem) if broad else 0),alpha_changes='broadleaf lower central stem and bottom6% removed from canopy; separate original stem' if broad else 'original source alpha interpolated')
 (p/'recipe.json').write_text(json.dumps(metadata,indent=2)+'\n');(p/'crown.tga').unlink();return asset

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
 ap.add_argument('--tree-geometry',choices=['shell','crossed'],default='shell',help='Experimental fixed cutout alternative; neither mode has native visual acceptance')
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
    if rec.get('category')=='individual-tree' and (archive,Path(leaf).stem) in TREE_CANDIDATES:
     asset=(crossed_tree if args.tree_geometry=='crossed' else tree)(a,Path('trees')/rel);rows.append('\t'.join(['tree',archive,leaf,asset,'','1']));status='source-fitted-interpolated'
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
