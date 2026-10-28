#!/usr/bin/env python3
"""Read-only, narrow private export for the observed debug/tree coverage gaps.

Only exact source-resolved stage slot1 font (general/hel_20pt) and eight
observed tree basenames are copied. Never changes game files or uploads data.
Python standard library only; output must not already exist.
"""
import argparse
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import zipfile

TREES = {'tre_bs6','tre_40','tre_bs1','tre_sp2','tre_bs4','tre_sap4','tre_sl2','tre_sap3'}
LIMIT = 256 * 1024 * 1024

def wanted(name):
    path = name.replace('\\','/').lower()
    leaf = path.rsplit('/',1)[-1]
    stem, dot, ext = leaf.rpartition('.')
    return bool(dot and ((stem in TREES and ext in ('dds','tga')) or
                         (stem == 'hel_20pt' and ext in ('dds','tga','pcf'))))

def members(raw):
    """CMR2 archive directory, matching GenericFileLoader and existing converter."""
    if raw[:2] == b'\x1f\x8b':
        with gzip.GzipFile(fileobj=io.BytesIO(raw)) as f:
            raw = f.read(LIMIT+1)
        if len(raw)>LIMIT or raw[:4]!=b'CMPR':
            raise ValueError('compressed archive size/header')
        raw = raw[8:]
    if len(raw)<4 or len(raw)>LIMIT: raise ValueError('archive size')
    end = len(raw)-4
    pos = struct.unpack_from('<I',raw,end)[0]
    if pos>end: raise ValueError('archive directory outside file')
    data_end = pos
    seen = set()
    while pos<end:
        if pos+12>end: raise ValueError('truncated directory')
        size, offset, length = struct.unpack_from('<III',raw,pos)
        pos+=12
        padded=(length+3)//4*4
        if not 0<length<=259 or pos+padded>end or offset+size>data_end:
            raise ValueError('invalid member bounds')
        name=raw[pos:pos+length].decode('ascii');pos+=padded
        if '\x00' in name or name in seen: raise ValueError('duplicate/invalid member name')
        seen.add(name)
        yield name,raw[offset:offset+size]

def export(data,output):
    data=data.resolve(strict=True);output=output.absolute()
    if not data.is_dir():raise ValueError('data must be directory')
    if output.exists():raise ValueError('output already exists; choose a new ZIP path')
    # Destination may be alongside the game, never inside it.
    resolved_parent=output.parent.resolve(strict=True)
    if resolved_parent==data or data in resolved_parent.parents:
        raise ValueError('output must be outside original game data')
    hits=[];failures=[];scanned=0;payloads={};total=0
    def record(origin,member,raw):
        nonlocal total
        if not wanted(member):return
        total+=len(raw)
        if total>32*1024*1024:raise ValueError('selected evidence exceeds32MB safety limit')
        digest=hashlib.sha256(raw).hexdigest()
        leaf=member.replace('\\','/').rsplit('/',1)[-1].lower()
        path='assets/'+digest+'/'+leaf
        payloads[path]=raw
        hits.append({'origin':origin,'member':member,'sha256':digest,'bytes':len(raw),'export_path':path})
    def scan(raw,origin,depth=0):
        nonlocal scanned
        if depth>2:failures.append({'origin':origin,'error':'nested archive depth limit'});return
        scanned+=1
        try:
            for name,content in members(raw):
                record(origin,name,content)
                if name.lower().endswith('.bfl'):scan(content,origin+'!'+name,depth+1)
        except (ValueError,UnicodeError,struct.error,OSError,EOFError) as e:
            failures.append({'origin':origin,'error':str(e)})
    for directory,dirs,files in os.walk(data,followlinks=False):
        dirs[:]=sorted(d for d in dirs if not (Path(directory)/d).is_symlink())
        for name in sorted(files):
            p=Path(directory)/name
            if p.is_symlink():continue
            rel=p.relative_to(data).as_posix()
            if name.lower().endswith('.bfl'):
                if p.stat().st_size>LIMIT:failures.append({'origin':rel,'error':'archive size limit'});continue
                scan(p.read_bytes(),rel)
            elif wanted(rel):
                if p.stat().st_size>16*1024*1024:raise ValueError('selected asset size limit: '+rel)
                record(rel,rel,p.read_bytes())
    manifest={'scope':'stageFontSlot1=general/hel_20pt and eight observed tree names; provenance candidates, not proof of runtime selection',
              'archives_scanned':scanned,'hits':hits,'parse_failures':failures,
              'missing_basenames':sorted((TREES|{'hel_20pt'})-{Path(x['export_path']).stem for x in hits})}
    # Exclusive creation prevents replacement even if another process races us.
    try:
        with output.open('xb') as f:
            with zipfile.ZipFile(f,'w',zipfile.ZIP_DEFLATED) as z:
                z.writestr('manifest.json',json.dumps(manifest,indent=2)+'\n')
                for path,raw in sorted(payloads.items()):z.writestr(path,raw)
    except Exception:
        # Do not remove any destination another process may have created.
        raise
    print('Private evidence ZIP:',output)
    print('Selected assets:',len(payloads),'provenance hits:',len(hits),'archives scanned:',scanned)
    print('Missing basenames:',', '.join(manifest['missing_basenames']) or 'none')
    print('Archive parse failures:',len(failures),'(details in manifest.json)')
    print('No originals changed. Nothing uploaded.')
    return manifest

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args()
    try:export(a.data,a.output)
    except (ValueError,OSError) as e:ap.exit(1,str(e)+'\n')
if __name__=='__main__':main()
