#!/usr/bin/env python3
"""Exact original fixed-strip source metadata extractor (position/UV only).

Reads an owned stage archive (hi or lo), walks the documented C3D mesh layout
(game/Sector.cpp, game/Mesh.h), rebuilds Mesh_BuildParts' per-texture index
runs, and prints the SHA-256 of each strip candidate's raw vertex position/UV
bytes in draw order - the identity contract game/FixedStripSources.inc gates
on (game/SceneryPack.cpp). Contains no artwork; originals are read-only.
Validated: every hash this prints for fin01hi.bfl temp.sht tresh* parts is
already present in FixedStripSources.inc (58/58).
"""
import struct, hashlib, sys
from pathlib import Path
import importlib.util
_spec = importlib.util.spec_from_file_location('export_evidence', Path(__file__).with_name('export-evidence.py'))
_export = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_export)

def _names(b):
    tnoff = struct.unpack_from('<i', b, 0xc)[0]
    names = []
    i = 0
    while tnoff + (i+1)*260 <= len(b):
        s = b[tnoff+i*260: tnoff+(i+1)*260].split(b'\x00')[0]
        if not s: break
        names.append(s.decode('ascii','replace').split('\\')[-1]); i += 1
    return names

def strip_hashes(blob, member, stems):
    """Yield (texture_stem, sha256) for each strip-like part of `member`."""
    members = dict(_export.members(blob))
    if member not in members: raise ValueError('member not in archive: ' + member)
    b = members[member]
    if b[:4] != b'PP_F': raise ValueError('not a C3D model: ' + member)
    nm, no, ns = struct.unpack_from('<3H', b, 26)
    tri_count = struct.unpack_from('<I', b, 32)[0]
    ts = 48 + nm*288 + no*448 + ns*136
    vs = ts + tri_count*76
    names = _names(b)
    for mi in range(nm + no):
        m = 48 + mi*288 if mi < nm else 48 + nm*288 + (mi-nm)*448
        flags = struct.unpack_from('<I', b, m+48)[0]
        if not (flags & 0x800) or (flags & 2): continue  # SceneryPack gate
        vo = struct.unpack_from('<I', b, m+12)[0]
        to, tc = struct.unpack_from('<II', b, m+36)
        parts = []
        last = -99
        for ti in range(tc):
            off = ts + to + ti*76
            if off + 76 > len(b): raise ValueError('triangle outside model')
            tex = struct.unpack_from('<i', b, off+4)[0]
            ids = struct.unpack_from('<3H', b, off+64)
            if tex != last:
                parts.append((tex, [])); last = tex
            parts[-1][1].extend(ids)
        for tex, idx in parts:
            n = len(idx)
            if n < 6 or n > 12000 or tex < 0 or tex >= len(names): continue
            stem = names[tex].lower()
            if not any(stem.startswith(s) for s in stems): continue
            h = hashlib.sha256()
            for j in idx:
                base = vs + vo + j*48
                if base + 40 > len(b): raise ValueError('vertex outside model')
                h.update(b[base:base+12]); h.update(b[base+32:base+40])
            yield stem, h.hexdigest()

if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('archive', type=Path)
    ap.add_argument('--member', default='temp.sht')
    ap.add_argument('--stems', default='bsh_sh1,tre_s1')
    a = ap.parse_args()
    for stem, digest in strip_hashes(a.archive.read_bytes(), a.member, a.stems.split(',')):
        print(stem, digest)
