#!/usr/bin/env python3
"""Read-only Finland 1 tree/forest placement inventory, not a 3D-success claim.

Only names, hashes, record IDs and counts leave the game archives. No textures,
vertices, matrices or collision data are exported. Hi/lo archives stay separate:
adding their counts would double-count alternative detail versions. Original
geometry is not classified here; this inventory is not proof of native
replacement draws or of true 3D trees. Python standard library only.
"""
import argparse
import collections
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import zlib

_spec = importlib.util.spec_from_file_location('evidence', Path(__file__).with_name('export-evidence.py'))
_e = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_e)
LIMIT = 256 * 1024 * 1024
NODE, MESH, OBJECT, SECTOR, PORTAL, TRI = 0x18c, 0x120, 0xa0, 0x88, 0x1c, 76


def analyse(raw):
    def take(offset, fmt):
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(raw):
            raise ValueError('record outside model')
        return struct.unpack_from(fmt, raw, offset)
    if len(raw) < 48 or raw[:4] != b'PP_F':
        raise ValueError('not a PP_F stage model')
    nn, nm, no, ns = take(24, '<4H')
    nt, = take(32, '<i')
    portals, = take(40, '<H')
    names_at, = take(12, '<I')
    mesh_at = 48 + nn * NODE
    object_at = mesh_at + (nm + no) * MESH
    tri_at = object_at + no * OBJECT + ns * SECTOR + portals * PORTAL
    if nt < 0 or tri_at + nt * TRI > len(raw) or names_at < tri_at + nt * TRI:
        raise ValueError('invalid triangle/name table bounds')
    textures = []
    if names_at > len(raw):
        raise ValueError('texture name table outside model')
    # Like the existing brush exporter: names are fixed 260-byte records at
    # header+0xc. Header+0x24 counts texture-record groups, NOT name records.
    for i in range((len(raw)-names_at)//260):
        name = raw[names_at+i*260:names_at+(i+1)*260].split(b'\0')[0]
        if not name:
            break
        textures.append(name.decode('ascii').replace('\\', '/').rsplit('/', 1)[-1].lower())
    candidates = {}
    unclassified = set()
    for mi in range(nm + no):
        lods, = take(mesh_at + mi*MESH + 0x113, '<B')
        if lods > 1:
            raise ValueError('multiple LOD records not supported')
        if lods == 0:
            continue
        offset, count = take(mesh_at + mi*MESH + 36, '<II')
        if offset % TRI or offset + count * TRI > nt * TRI:
            raise ValueError('mesh triangle range outside table')
        names = set()
        for j in range(count):
            # Every animation/material texture lane, not only frame zero.
            for tx in take(tri_at + offset + j*TRI + 4, '<10i'):
                if tx == -1:
                    continue
                if not 0 <= tx < len(textures):
                    raise ValueError('triangle texture outside name table')
                name = textures[tx]
                if name.startswith(('tre', 'bsh')):
                    names.add(name)
                else:
                    unclassified.add(name)
        if names:
            candidates[mi] = sorted(names)
    references = collections.defaultdict(list)
    for ni in range(nn):
        ptr, = take(48 + ni*NODE + 12, '<I')
        if ptr == 0xffffffff:
            continue
        if ptr % MESH or ptr // MESH >= nm + no:
            raise ValueError('scene-node mesh reference outside table')
        node_type, = take(48 + ni*NODE + 0x178, '<i')
        references[ptr//MESH].append({'kind': 'scene-node', 'record': ni,
                                      'node_type_unverified': node_type})
    for oi in range(no):
        ptr, = take(object_at + oi*OBJECT + 12, '<I')
        if ptr == 0xffffffff:
            continue
        if ptr % MESH or ptr // MESH >= no:
            raise ValueError('stage-object mesh reference outside table')
        references[nm+ptr//MESH].append({'kind': 'stage-object', 'record': oi})
    # Sector references are possible baked geometry, not one tree per sector.
    sector_refs = collections.defaultdict(list)
    for si in range(ns):
        ptr, = take(object_at + no*OBJECT + si*SECTOR + 0x10, '<I')
        if ptr == 0xffffffff:
            continue
        if ptr % MESH or ptr // MESH >= nm + no:
            raise ValueError('sector mesh reference outside table')
        sector_refs[ptr//MESH].append(si)
    rows = []
    for mi, names in candidates.items():
        categories = {name: ('forest-strip-name' if name.startswith(('tresbg', 'tresfg', 'tresh'))
                             else 'bush-name' if name.startswith('bsh')
                             else 'tree-name-unverified') for name in names}
        rows.append({'mesh_record': mi, 'textures': names,
                     'name_categories_not_geometry_proof': categories,
                     'sector_references': sector_refs[mi],
                     'placements': references[mi],
                     'placement_count': len(references[mi]),
                     'unresolved_placement': not bool(references[mi]) or bool(sector_refs[mi])})
    return {'scene_nodes': nn, 'ordinary_meshes': nm, 'object_meshes': no,
            'candidate_mesh_records': rows,
            'referenced_candidate_placements': sum(r['placement_count'] for r in rows),
            'other_material_names_require_manual_tree_check': sorted(unclassified),
            'unresolved_mesh_records': sum(r['unresolved_placement'] for r in rows)}


def export(data, output):
    data = Path(data).resolve(strict=True)
    output = Path(output).absolute()
    if not data.is_dir():
        raise ValueError('data must be a directory')
    parent = output.parent.resolve(strict=True)
    if parent == data or data in parent.parents:
        raise ValueError('output must be outside original game data')
    if output.exists() or output.is_symlink():
        raise ValueError('output already exists')
    report = {'schema': 'OPENCMR2-FIN01-TREE-PLACEMENT-1',
              'all_trees_3d_verified': False,
              'candidate_detection': 'tre*/bsh* texture names only; inspect other material names for uncategorized trees; texture-name categories are not geometry proof',
              'geometry_classification': 'not attempted; original cards, crossed cards and true 3D are not distinguished',
              'scope': 'candidate placement records only; no runtime or true-3D acceptance; hi/lo alternatives are not additive',
              'archives': [], 'refusals': []}
    for directory, dirs, files in os.walk(data, followlinks=False):
        dirs[:] = sorted(d for d in dirs if not (Path(directory)/d).is_symlink())
        for leaf in sorted(files):
            path = Path(directory)/leaf
            if leaf.lower() not in ('fin01hi.bfl', 'fin01lo.bfl') or path.is_symlink():
                continue
            relative = path.relative_to(data).as_posix()
            entry = {'archive': relative, 'members': []}
            report['archives'].append(entry)
            try:
                if path.stat().st_size > LIMIT:
                    raise ValueError('archive size limit')
                content = path.read_bytes()
                entry['sha256'] = hashlib.sha256(content).hexdigest()
                members = list(_e.members(content))
                seen = set()
                for name, raw in members:
                    name = name.replace('\\', '/').rsplit('/', 1)[-1].lower()
                    if name not in ('temp.sht', 'temp.obj', 'temp.c3d'):
                        continue
                    if name in seen:
                        raise ValueError('ambiguous duplicate model member: '+name)
                    seen.add(name)
                    if raw[:4] != b'PP_F':
                        raise ValueError(name + ': unknown model header')
                    entry['members'].append({'member': name,
                                             'sha256': hashlib.sha256(raw).hexdigest(),
                                             'analysis': analyse(raw)})
                if not entry['members']:
                    raise ValueError('no known stage model members found')
            except (ValueError, OSError, EOFError, UnicodeError, struct.error, zlib.error) as exc:
                report['refusals'].append({'archive': relative, 'why': str(exc)})
    if not report['archives']:
        raise ValueError('no regular fin01hi.bfl / fin01lo.bfl found')
    report['parse_complete'] = not report['refusals']
    with output.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    try:
        report = export(args.data, args.output)
    except (ValueError, OSError) as exc:
        ap.exit(1, str(exc)+'\n')
    print('Finland 1 inventory:', args.output)
    print('No originals changed or uploaded. This is not proof of 3D replacement.')
    return 0 if report['parse_complete'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
