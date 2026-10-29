"""Private matched-road research cache contract. Never enables runtime geometry.

The binary deliberately does not offer a curved-road launch mode until contact,
FIN barriers and native validation are complete. This module validates private
research artifacts and their source identity without changing game/profile data.
"""
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import struct

SCHEMA = 'OPENCMR2-ROAD-RESEARCH-1'
MAX_FILE = 64 * 1024 * 1024
# Verified against privately exported original archive members, not filenames alone.
SOURCE_HASHES = {
 'fra01.hpc': '2d262119d5b768b2517dc7c2b7f66968bfe1a5becf0964e9ed2691316040fa3e',
 'fra01fin.dat': '5e99cc9e3f9eced8ee6f27de89cd79db0eeae0b30ca01463b06ebf9f0fbf4532',
 'fra01.srf': 'd087dd073fc11f11f35886cc2d436d0228ae834c1678a9f5e0c6363ca0b80565',
 'temp.c3d': 'a78fbf25efcecb20a0cd7360c2bfc28e24ba040a028f6225a0d1f690f9eed8d0',
}

def digest(path):
    if path.is_symlink() or not path.is_file() or path.stat().st_size > MAX_FILE:
        raise ValueError('Expected bounded regular file, not a symlink')
    return hashlib.sha256(path.read_bytes()).hexdigest()

def safe(root, relative):
    p = Path(relative)
    if p.is_absolute() or not p.parts or '..' in p.parts or '\\' in relative:
        raise ValueError('Unsafe research member path')
    target = root / p
    if any(q.is_symlink() for q in [target, *target.parents]):
        raise ValueError('Symlink research path refused')
    return target

def validate(document, source, artifacts):
    source, artifacts = Path(source).resolve(strict=True), Path(artifacts).resolve(strict=True)
    if document.get('schema') != SCHEMA or document.get('runtime_enable') is not False:
        raise ValueError('Research contract required; runtime activation is unavailable')
    if document.get('stage') != 'fra01' or document.get('source_archive') != 'fra01hi (1).bfl':
        raise ValueError('Unsupported exact research stage/archive')
    if document.get('maximum_offset_m', 999) > .35 or document.get('maximum_offset_m', -1) < 0:
        raise ValueError('Offset exceeds bounded research contract')
    required = {'fra01.hpc', 'fra01fin.dat', 'fra01.srf', 'temp.c3d'}
    rows = document.get('sources', [])
    if len(rows) != 4 or {r.get('member') for r in rows} != required:
        raise ValueError('Need all four original contact/barrier/surface/render sources')
    for row in rows:
        if row.get('sha256') != SOURCE_HASHES[row['member']] or digest(safe(source, row['member'])) != row.get('sha256'):
            raise ValueError('Original source hash differs; refusing unrelated stage')
    outputs = document.get('outputs', [])
    if not outputs or len(outputs) > 8:
        raise ValueError('Bounded output inventory required')
    seen = set()
    for row in outputs:
        name = row.get('member', '')
        if name in seen:
            raise ValueError('Duplicate output member')
        seen.add(name)
        if digest(safe(artifacts, name)) != row.get('sha256'):
            raise ValueError('Research artifact hash differs')
    # Parse the real collision block, not self-reported counts. Bounds checked
    # before every read; this does not replace geometry/native acceptance.
    if 'curved.hpc' not in seen:
        raise ValueError('Missing collision artifact')
    raw = safe(artifacts, 'curved.hpc').read_bytes()
    def take(offset, fmt):
        if offset + struct.calcsize(fmt) > len(raw):
            raise ValueError('Truncated collision artifact')
        return struct.unpack_from(fmt, raw, offset)
    counts = take(8, '<5h')
    if min(counts) < 0:
        raise ValueError('Negative quadtree count')
    pos = 20 + sum(counts)*8
    vertices = take(pos, '<H')[0]; pos += 4 + vertices*12
    triangles = take(pos, '<H')[0]; pos += 4
    if vertices != document.get('collision_vertices') or triangles != document.get('collision_triangles'):
        raise ValueError('Collision artifact count differs')
    for i in range(triangles):
        row = take(pos+i*8, '<4H')
        if max(row[:3]) >= vertices:
            raise ValueError('Collision artifact vertex index outside table')
    pos += triangles*8
    refs = take(pos, '<i')[0]; pos += 4
    if refs < 0 or pos+refs*2+4 != len(raw):
        raise ValueError('Collision artifact reference size differs')
    for i in range(refs):
        if not 0 <= take(pos+i*2, '<h')[0] < triangles:
            raise ValueError('Collision artifact triangle reference outside table')
    offset = 20
    for level, count in enumerate(counts):
        for i in range(count):
            n, unused, start = take(offset+i*8, '<hhi')
            if n == -1:
                if level == 4 or start < 0 or start+16 > counts[level+1]:
                    raise ValueError('Quadtree child block outside table')
            elif n < 0 or start < 0 or start+n > refs:
                raise ValueError('Quadtree leaf references outside table')
        offset += count*8
    columns, rows = take(pos+refs*2, '<hh')
    if columns <= 0 or rows <= 0 or columns*rows != counts[0]:
        raise ValueError('Invalid collision grid')
    if document.get('collision_triangles', 0) not in range(1, 32768):
        raise ValueError('Collision triangle count exceeds signed cached index contract')
    if document.get('collision_vertices', 0) not in range(1, 65536):
        raise ValueError('Collision vertex count exceeds unsigned vertex index contract')
    # FIN is not guessed, regenerated or silently warped by this experiment.
    if document.get('fin_policy') != 'original-unverified-alignment':
        raise ValueError('Unsupported FIN handling; barriers require independent grounding')
    if 'curved.c3d' in seen:
        from road_pair import validate_pair
        validate_pair(raw, safe(artifacts, 'curved.c3d').read_bytes())
    return document

def cache_key(document):
    h = hashlib.sha256(Path(__file__).read_bytes())
    h.update(Path(__file__).with_name('road_pair.py').read_bytes())
    h.update(json.dumps(document, sort_keys=True, separators=(',', ':')).encode())
    return h.hexdigest()[:24]

def prepare(document, source, artifacts, cache):
    """Validate then stage a read-only research package, never a launch pack."""
    cache = Path(cache)
    if cache.is_symlink() or any(p.is_symlink() for p in cache.parents):
        raise ValueError('Symlink research cache refused')
    cache = cache.resolve()
    for root in [Path(source).resolve(), Path(artifacts).resolve()]:
        if cache == root or root in cache.parents or cache in root.parents:
            raise ValueError('Research cache must be separate from source/artifacts')
    validate(document, source, artifacts)
    destination = cache / ('roads-research-' + cache_key(document))
    cache.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        existing = json.loads(safe(destination, 'research.json').read_text())
        if existing != document:
            raise ValueError('Existing research manifest differs')
        validate(existing, source, destination)
        return destination
    temporary = Path(tempfile.mkdtemp(prefix='.road-research-', dir=cache))
    try:
        for row in document['outputs']:
            target = safe(temporary, row['member'])
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(safe(Path(artifacts), row['member']), target)
        (temporary / 'research.json').write_text(json.dumps(document, indent=2) + '\n')
        validate(document, source, temporary)
        # Do not replace any existing cache: content-addressed outputs are immutable.
        if destination.exists():
            raise FileExistsError('Research cache appeared concurrently; retry read verification')
        temporary.rename(destination)
        return destination
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)

def require_runtime(document):
    raise ValueError('Curved roads are research only: FIN alignment, native handling, performance and binary opt-in are not validated')
