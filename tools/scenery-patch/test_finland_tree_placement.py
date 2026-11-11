import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from test_convert import bfl

spec = importlib.util.spec_from_file_location('placement', Path(__file__).with_name('export-finland-tree-placement.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def model(nodes=0, objects=1, texture='tre_fir1.dds', animation=None):
    # One ordinary mesh shared by every node, plus one object mesh per object.
    nm = 1 if nodes else 0
    nn = bytearray(nodes*m.NODE)
    for i in range(nodes):
        struct.pack_into('<I', nn, i*m.NODE+12, 0)
    meshes = bytearray((nm+objects)*m.MESH)
    for i in range(nm+objects):
        struct.pack_into('<II', meshes, i*m.MESH+36, 0, 1)
        meshes[i*m.MESH+0x113] = 1
    oo = bytearray(objects*m.OBJECT)
    for i in range(objects):
        struct.pack_into('<I', oo, i*m.OBJECT+12, i*m.MESH)
    tri = bytearray(m.TRI)
    struct.pack_into('<10i', tri, 4, 0, *([-1]*9))
    if animation:
        struct.pack_into('<i', tri, 8, 1)
    head = bytearray(48)
    head[:4] = b'PP_F'
    struct.pack_into('<4H', head, 24, nodes, nm, objects, 0)
    struct.pack_into('<i', head, 32, 1)
    body = nn + meshes + oo + tri
    struct.pack_into('<I', head, 12, 48+len(body))
    names = texture.encode().ljust(260, b'\0')
    if animation:
        names += animation.encode().ljust(260, b'\0')
    return bytes(head+body+names)


class PlacementTests(unittest.TestCase):
    def test_counts_placements_not_texture_files_or_animation_frames(self):
        r = m.analyse(model(nodes=3, objects=2, animation='tre_fir2.dds'))
        self.assertEqual(r['referenced_candidate_placements'], 5)
        self.assertEqual(len(r['candidate_mesh_records']), 3)
        self.assertEqual(len(r['candidate_mesh_records'][0]['textures']), 2)
        self.assertEqual(r['unresolved_mesh_records'], 0)

    def test_strips_and_unrecognized_tree_names_stay_in_audit(self):
        for texture in ('tresbg1a.dds', 'tre_unseen.dds', 'bsh_01.dds'):
            r = m.analyse(model(texture=texture))
            self.assertEqual(r['referenced_candidate_placements'], 1)
        self.assertEqual(m.analyse(model(texture='road.dds'))['referenced_candidate_placements'], 0)

    def test_unreferenced_mesh_does_not_disappear_or_become_success(self):
        raw = bytearray(model(objects=1))
        struct.pack_into('<I', raw, 48+m.MESH+12, 0xffffffff)
        r = m.analyse(raw)
        self.assertEqual(r['referenced_candidate_placements'], 0)
        self.assertEqual(r['unresolved_mesh_records'], 1)

    def test_invalid_counts_and_references_fail_closed(self):
        for offset, fmt, value in [(32, '<i', -1), (12, '<I', 999999),
                                    (48+36, '<I', 1),
                                    (48+m.MESH+12, '<I', m.MESH)]:
            raw = bytearray(model())
            struct.pack_into(fmt, raw, offset, value)
            with self.assertRaises(ValueError):
                m.analyse(raw)
        with self.assertRaises(ValueError):
            m.analyse(b'PP_F')

    def test_read_only_no_assets_no_hi_lo_sum_and_refusals(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = root/'game'
            data.mkdir()
            (data/'fin01hi.bfl').write_bytes(bfl([('temp.sht', model(nodes=2)), ('private.dds', b'PRIVATE_PIXELS')]))
            (data/'fin01lo.bfl').write_bytes(bfl([('temp.sht', model(nodes=1))]))
            before = {p.name:p.read_bytes() for p in data.iterdir()}
            out = root/'inventory.json'
            r = m.export(data, out)
            self.assertTrue(r['parse_complete'])
            self.assertFalse(r['all_trees_3d_verified'])
            self.assertEqual(len(r['archives']), 2)
            self.assertNotIn('total', r)
            text = out.read_text()
            self.assertNotIn('PRIVATE_PIXELS', text)
            self.assertNotIn('private.dds', text)
            self.assertEqual(before, {p.name:p.read_bytes() for p in data.iterdir()})
            with self.assertRaises(ValueError):
                m.export(data, out)
            with self.assertRaises(ValueError):
                m.export(data, data/'out.json')
            (data/'fin01lo.bfl').write_bytes(b'bad')
            r = m.export(data, root/'bad.json')
            self.assertFalse(r['parse_complete'])
            self.assertEqual(len(r['refusals']), 1)

    def test_sector_reference_is_not_counted_as_one_tree(self):
        raw = bytearray(model(objects=1))
        sector_at = 48+m.MESH+m.OBJECT
        raw[sector_at:sector_at] = bytearray(m.SECTOR)
        struct.pack_into('<H', raw, 30, 1)
        names_at, = struct.unpack_from('<I', raw, 12)
        struct.pack_into('<I', raw, 12, names_at+m.SECTOR)
        r = m.analyse(raw)
        self.assertEqual(r['candidate_mesh_records'][0]['sector_references'], [0])
        self.assertTrue(r['candidate_mesh_records'][0]['unresolved_placement'])
        self.assertEqual(r['referenced_candidate_placements'], 1)

    def test_lod_zero_ignored_and_multi_lod_refused(self):
        raw = bytearray(model())
        raw[48+0x113] = 0
        self.assertEqual(m.analyse(raw)['candidate_mesh_records'], [])
        raw[48+0x113] = 2
        with self.assertRaises(ValueError):
            m.analyse(raw)

    def test_other_materials_remain_visible_for_manual_check(self):
        r = m.analyse(model(texture='unknown_forest.dds'))
        self.assertEqual(r['other_material_names_require_manual_tree_check'], ['unknown_forest.dds'])

    def test_symlink_archives_are_not_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = root/'game'
            data.mkdir()
            outside = root/'outside.bfl'
            outside.write_bytes(bfl([('temp.sht', model())]))
            (data/'fin01hi.bfl').symlink_to(outside)
            with self.assertRaises(ValueError):
                m.export(data, root/'out.json')


if __name__ == '__main__':
    unittest.main()
