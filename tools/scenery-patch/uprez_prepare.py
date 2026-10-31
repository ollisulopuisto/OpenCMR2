#!/usr/bin/env python3
"""Optional exact-source Finland1/France1/Greece1 opaque ground2x filter, no artwork shipped."""
import hashlib,json,os,struct,tempfile
from pathlib import Path
try:
 from PIL import Image
 import io
 from integer_filter import levels as integer_levels
except ImportError:raise SystemExit('Stage textures need existing Python with Pillow; nothing installed.')
import importlib.util
_spec=importlib.util.spec_from_file_location('export_evidence',Path(__file__).with_name('export-evidence.py'))
_export=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_export)
HERE=Path(__file__).parent
INDEX=json.loads((HERE/'uprez-textures.json').read_text())['sources']
LIMIT=32*1024*1024

def encode(raw,row):
 if len(raw)!=row['bytes'] or hashlib.sha256(raw).hexdigest()!=row['sha256']:raise ValueError('Original texture differs')
 image=Image.open(io.BytesIO(raw)).convert('RGBA')
 if image.size!=(row['width'],row['height']):raise ValueError('Source dimensions differ')
 if row.get('kind')=='full-canvas-cutout'and row['repeat']!=[False,False]:raise ValueError('Cutout requires clamp policy')
 if row.get('kind')!='full-canvas-cutout' and image.getchannel('A').getextrema()!=(255,255):raise ValueError('Opaque source alpha differs')
 levels=integer_levels(image,tuple(row['repeat']))
 out=b'OCMRRGBA'+struct.pack('<III',levels[0].width,levels[0].height,len(levels))
 return out+b''.join(level.tobytes()for level in levels)

def prepare(data,cache):
 data=Path(data).resolve(strict=True);cache=Path(cache)
 if cache.is_symlink() or any(p.is_symlink()for p in cache.parents):raise ValueError('Symlink texture cache refused')
 cache=cache.resolve()
 if cache==data or data in cache.parents:raise ValueError('Cache must be outside owned data')
 sources={}
 for archive in sorted({r['archive']for r in INDEX}):
  matches=[p for p in data.rglob('*')if p.name.lower()==archive and p.is_file()]
  if not matches:continue
  if len(matches)!=1 or any(p.is_symlink()for p in [matches[0],*matches[0].parents]):raise ValueError('Need exactly one regular '+archive)
  wanted={r['member']for r in INDEX if r['archive']==archive}
  for name,raw in _export.members(matches[0].read_bytes()):
   if name.lower()in wanted:sources[(archive,name.lower())]=raw
 present={p.name.lower() for p in data.rglob('*') if p.is_file()}
 required=[r for r in INDEX if r['archive'] in present]
 if any((r['archive'],r['member']) not in sources for r in required):raise ValueError('Missing required indexed ground texture in present archive')
 selected=required
 if not selected:raise ValueError('No supported owned ground-texture archives found')
 payloads={r['output']:encode(sources[(r['archive'],r['member'])],r)for r in selected}
 for r in selected:
  if hashlib.sha256(payloads[r['output']]).hexdigest()!=r['output_sha256']:raise ValueError('Recipe output differs from reviewed image: '+r['archive']+'/'+r['member']+'; Pillow '+Image.__version__+'; expected '+r['output_sha256']+' actual '+hashlib.sha256(payloads[r['output']]).hexdigest())
 manifest='OPENCMR2-UPREZ-TEXTURES-1\n'+''.join(r['sha256']+'\t'+r['output']+'\t'+hashlib.sha256(payloads[r['output']]).hexdigest()+'\n'for r in selected)
 payloads['uprez-textures.tsv']=manifest.encode()
 key=hashlib.sha256(manifest.encode())
 for n in ['uprez_prepare.py','uprez-textures.json','road_periodic.py','cutout_filter.py','export-evidence.py','integer_filter.py']:key.update((HERE/n).read_bytes())
 pack=cache/('uprez-textures-'+key.hexdigest()[:24]);cache.mkdir(parents=True,exist_ok=True)
 expected={n:hashlib.sha256(v).hexdigest()for n,v in payloads.items()}
 if pack.exists():
  if pack.is_symlink() or {p.name for p in pack.iterdir()}!=set(payloads):raise ValueError('Changed/incomplete texture cache')
  for name,digest in expected.items():
   p=pack/name
   if p.is_symlink() or p.stat().st_size>LIMIT or hashlib.sha256(p.read_bytes()).hexdigest()!=digest:raise ValueError('Changed texture cache')
 else:
  with tempfile.TemporaryDirectory(prefix='.textures-',dir=cache)as tmp:
   out=Path(tmp)/'pack';out.mkdir()
   for n,raw in payloads.items():(out/n).write_bytes(raw)
   if pack.exists():raise ValueError('Concurrent texture preparation; retry')
   out.rename(pack)
 print('Finland1/France1/Greece1 exact-source ground and full-canvas cutout2x filtering only; no restored detail, native appearance/performance unverified')
 print('OPENCMR2_TEXTURES='+str(pack));return pack
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--input-dir',type=Path,required=True);p.add_argument('--cache-dir',type=Path,required=True);a=p.parse_args()
 try:prepare(a.input_dir,a.cache_dir)
 except (ValueError,OSError,KeyError,struct.error)as e:raise SystemExit('Stage texture preparation refused: '+str(e))
