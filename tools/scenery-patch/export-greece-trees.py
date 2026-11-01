#!/usr/bin/env python3
"""Read-only, narrow private export of Greece tree-card candidates.

Copies every tre* DDS/TGA member of exactly one hash-verified grehit.bfl and
grelot.bfl pair out of an owned game directory for off-screen review, the
same contract as export-evidence.py. Archive identity is pinned by the exact
sha256 of the six already-uprezzed members per archive recorded in
uprez-textures.json; a modified or wrong archive refuses before anything is
copied. Never changes game files or uploads data. Python standard library
only; output ZIP must not already exist and must live outside the game data.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import zipfile

_spec = importlib.util.spec_from_file_location('export_evidence', Path(__file__).with_name('export-evidence.py'))
_export = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_export)

HERE = Path(__file__).parent
ARCHIVES = ('grehit.bfl', 'grelot.bfl')
LIMIT = 256 * 1024 * 1024
SELECT_CAP = 32 * 1024 * 1024

def _identity_members():
    rows = json.loads((HERE / 'uprez-textures.json').read_text(encoding='utf-8'))['sources']
    out = {}
    for row in rows:
        if row['archive'] in ARCHIVES:
            out.setdefault(row['archive'], {})[row['member'].lower()] = row['sha256']
    return out

def _dims(leaf, raw):
    """(width, height) from a DDS or TGA header, else None."""
    if raw[:4] == b'DDS ' and len(raw) >= 20:
        height, width = struct.unpack_from('<II', raw, 12)
        if 0 < width <= 8192 and 0 < height <= 8192:
            return [width, height]
    if leaf.lower().endswith('.tga') and len(raw) >= 16:
        width, height = struct.unpack_from('<HH', raw, 12)
        if 0 < width <= 8192 and 0 < height <= 8192:
            return [width, height]
    return None

def _wanted(name):
    path = name.replace('\\', '/').lower()
    leaf = path.rsplit('/', 1)[-1]
    stem, dot, ext = leaf.rpartition('.')
    return bool(dot and stem.startswith('tre') and ext in ('dds', 'tga'))

def export(data, output):
    data = data.resolve(strict=True); output = output.absolute()
    if not data.is_dir(): raise ValueError('data must be directory')
    if output.exists(): raise ValueError('output already exists; choose a new ZIP path')
    resolved_parent = output.parent.resolve(strict=True)
    if resolved_parent == data or data in resolved_parent.parents:
        raise ValueError('output must be outside original game data')
    paths = {}
    for directory, dirs, files in os.walk(data, followlinks=False):
        dirs[:] = sorted(d for d in dirs if not (Path(directory) / d).is_symlink())
        for name in sorted(files):
            p = Path(directory) / name
            if name.lower() in ARCHIVES and not p.is_symlink():
                paths.setdefault(name.lower(), []).append(p)
    for name in ARCHIVES:
        if len(paths.get(name, [])) != 1:
            raise ValueError('Need exactly one ' + name + '; found ' + str(len(paths.get(name, []))))
    identity = _identity_members()
    hits = []; payloads = {}; total = 0
    for name in ARCHIVES:
        p = paths[name][0]
        if p.stat().st_size > LIMIT: raise ValueError('archive size limit: ' + name)
        found = {}
        for member, raw in _export.members(p.read_bytes()):
            leaf = member.replace('\\', '/').rsplit('/', 1)[-1].lower()
            known = identity.get(name, {}).get(leaf)
            if known:
                if hashlib.sha256(raw).hexdigest() != known:
                    raise ValueError('identity member differs: ' + name + '/' + leaf + ' (not the reviewed Greece archives)')
                found[leaf] = True
            if _wanted(member):
                total += len(raw)
                if total > SELECT_CAP: raise ValueError('selected evidence exceeds 32MB safety limit')
                digest = hashlib.sha256(raw).hexdigest()
                path = 'assets/' + name[:-4] + '/' + digest + '/' + leaf
                payloads[path] = raw
                hits.append({'archive': name, 'member': member, 'sha256': digest, 'bytes': len(raw),
                             'dimensions': _dims(leaf, raw), 'export_path': path})
        missing = sorted(set(identity.get(name, {})) - set(found))
        if missing:
            raise ValueError('identity members missing from ' + name + ': ' + ', '.join(missing))
    manifest = {'scope': 'grehit/grelot tre* tree-card candidates for visual review; provenance, not proof of runtime selection',
                'archives': {n: {'path': str(paths[n][0]), 'identity_members_verified': len(identity.get(n, {}))} for n in ARCHIVES},
                'hits': sorted(hits, key=lambda h: (h['archive'], h['member']))}
    try:
        with output.open('xb') as f:
            with zipfile.ZipFile(f, 'w', zipfile.ZIP_DEFLATED) as z:
                z.writestr('manifest.json', json.dumps(manifest, indent=2) + '\n')
                for path, raw in sorted(payloads.items()): z.writestr(path, raw)
    except Exception:
        raise
    print('Private Greece tree export:', output)
    print('Candidates:', len(payloads), 'across', len(ARCHIVES), 'hash-verified archives')
    print('No originals changed. Nothing uploaded.')
    return manifest

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    try: export(a.data, a.output)
    except (ValueError, OSError) as e: ap.exit(1, str(e) + '\n')

if __name__ == '__main__':
    main()
