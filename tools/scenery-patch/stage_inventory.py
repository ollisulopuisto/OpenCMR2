#!/usr/bin/env python3
"""Read-only, metadata-only inventory of the .bfl archives in an owned game folder.

Lists every archive (also one level of nested archives) with each member's name, size, SHA-256 and a coarse class from its name. It copies
no asset bytes, writes one new JSON file outside the game folder, changes nothing and uploads nothing. Python standard library only.
Use it to see which textures, tree and spectator cards, and geometry files a stage's archives hold before choosing what to convert.
Class is a name-prefix guess, not proof of what the game draws at runtime.
"""
import argparse, hashlib, importlib.util, json, os, struct
from pathlib import Path

_spec = importlib.util.spec_from_file_location('export_evidence', Path(__file__).with_name('export-evidence.py'))
_ee = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_ee)

PREFIX = {'tre': 'tree', 'bsh': 'bush', 'shr': 'shrub', 'spc': 'spectator', 'spe': 'spectator'}
def classify(name):
    leaf = name.replace('\\', '/').rsplit('/', 1)[-1].lower()
    stem, _, ext = leaf.rpartition('.')
    if ext == 'bfl': return 'archive'
    if ext in ('dds', 'tga', 'bmp', 'png'): return PREFIX.get(stem[:3], 'texture')
    if ext in ('cin', 'rdb', 'sdb', 'ssp', 'tsp', 'ptd'): return 'stage-data'
    return 'other-' + (ext or 'noext')

def inventory(data, output):
    data = data.resolve(strict=True); output = output.absolute()
    if not data.is_dir(): raise ValueError('data must be a directory')
    if output.exists(): raise ValueError('output already exists; choose a new file')
    parent = output.parent.resolve(strict=True)
    if parent == data or data in parent.parents: raise ValueError('output must be outside the game folder')
    archives, failures = [], []
    def scan(raw, origin, depth=0):
        entry = {'origin': origin, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(), 'members': []}
        try:
            for name, content in _ee.members(raw):
                entry['members'].append({'name': name, 'bytes': len(content), 'sha256': hashlib.sha256(content).hexdigest(), 'class': classify(name)})
                if name.lower().endswith('.bfl'):
                    if depth >= 1: failures.append({'origin': origin + '!' + name, 'error': 'nested archive depth limit'})
                    else: scan(content, origin + '!' + name, depth + 1)
        except (ValueError, UnicodeError, struct.error, OSError, EOFError) as e:
            failures.append({'origin': origin, 'error': str(e)})
        counts = {}
        for m in entry['members']: counts[m['class']] = counts.get(m['class'], 0) + 1
        entry['class_counts'] = dict(sorted(counts.items()))
        archives.append(entry)
    for directory, dirs, files in os.walk(data, followlinks=False):
        dirs[:] = sorted(d for d in dirs if not (Path(directory) / d).is_symlink())
        for name in sorted(files):
            p = Path(directory) / name
            if p.is_symlink() or not name.lower().endswith('.bfl'): continue
            if p.stat().st_size > _ee.LIMIT: failures.append({'origin': p.relative_to(data).as_posix(), 'error': 'archive size limit'}); continue
            scan(p.read_bytes(), p.relative_to(data).as_posix())
    doc = {'scope': 'metadata only: names, sizes, hashes, name-prefix class. No asset bytes. Class is not proof of runtime use.',
           'archives': archives, 'parse_failures': failures}
    with output.open('x') as f: json.dump(doc, f, indent=1); f.write('\n')
    return doc

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    try: doc = inventory(a.data, a.output)
    except (ValueError, OSError) as e: ap.exit(1, str(e) + '\n')
    for e in doc['archives']: print(e['origin'], e['bytes'], 'bytes', e['class_counts'])
    print('parse failures:', len(doc['parse_failures']), '- wrote', a.output, '- no originals changed, nothing uploaded')

if __name__ == '__main__': main()
