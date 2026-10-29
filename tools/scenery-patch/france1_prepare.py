#!/usr/bin/env python3
"""Optional exact-source France1 opaque road2x filter, no artwork shipped."""
import hashlib,json,os,struct,tempfile
from pathlib import Path
try:
 from PIL import Image
 import io
 from road_periodic import resize,mip_chain
except ImportError:raise SystemExit('France1 textures need existing Python with Pillow; nothing installed.')
import importlib.util
_spec=importlib.util.spec_from_file_location('export_evidence',Path(__file__).with_name('export-evidence.py'))
_export=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_export)
HERE=Path(__file__).parent
INDEX=json.loads((HERE/'france1-textures.json').read_text())['sources']
LIMIT=32*1024*1024

def encode(raw,row):
 if len(raw)!=row['bytes'] or hashlib.sha256(raw).hexdigest()!=row['sha256']:raise ValueError('Original texture differs')
 image=Image.open(io.BytesIO(raw)).convert('RGBA')
 if image.size!=(row['width'],row['height']) or image.getchannel('A').getextrema()!=(255,255):raise ValueError('Opaque source size/alpha differs')
 levels=mip_chain(resize(image,(image.width*2,image.height*2),tuple(row['repeat'])),tuple(row['repeat']))
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
  if len(matches)!=1 or any(p.is_symlink()for p in [matches[0],*matches[0].parents]):raise ValueError('Need exactly one regular '+archive)
  wanted={r['member']for r in INDEX if r['archive']==archive}
  for name,raw in _export.members(matches[0].read_bytes()):
   if name.lower()in wanted:sources[(archive,name.lower())]=raw
 payloads={r['output']:encode(sources[(r['archive'],r['member'])],r)for r in INDEX}
 for r in INDEX:
  if hashlib.sha256(payloads[r['output']]).hexdigest()!=r['output_sha256']:raise ValueError('Recipe output differs from reviewed image')
 manifest='OPENCMR2-FRANCE1-TEXTURES-1\n'+''.join(r['sha256']+'\t'+r['output']+'\t'+hashlib.sha256(payloads[r['output']]).hexdigest()+'\n'for r in INDEX)
 payloads['france1-textures.tsv']=manifest.encode()
 key=hashlib.sha256()
 for n in ['france1_prepare.py','france1-textures.json','road_periodic.py','export-evidence.py']:key.update((HERE/n).read_bytes())
 pack=cache/('france1-textures-'+key.hexdigest()[:24]);cache.mkdir(parents=True,exist_ok=True)
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
 print('France1 opaque road2x filtering only; no restored detail, native appearance/performance unverified')
 print('OPENCMR2_TEXTURES='+str(pack));return pack
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--input-dir',type=Path,required=True);p.add_argument('--cache-dir',type=Path,required=True);a=p.parse_args()
 try:prepare(a.input_dir,a.cache_dir)
 except (ValueError,OSError,KeyError,struct.error)as e:raise SystemExit('France1 texture preparation refused: '+str(e))
