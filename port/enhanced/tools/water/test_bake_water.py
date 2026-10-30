# SPDX-License-Identifier: GPL-3.0-or-later
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest
import bake_water as water


class WaterBakeTest(unittest.TestCase):
    def square(self,x=0,z=0):
        return [((x,0,z),(x+4,0,z),(x,4,z)),((x,4,z),(x+4,0,z),(x+4,4,z))]

    def test_patch_seams_are_not_banks(self):
        meta,cells=water.bake_body(1,self.square()+self.square(4),(0,0,0))
        width=int(meta[1][3])
        seam=cells[4*width+8]
        self.assertGreater(seam[0],.5)
        self.assertGreater(seam[1],1.)

    def test_slopes_keep_their_rest_height(self):
        triangles=[((0,0,1),(4,0,3),(0,4,1)),((0,4,1),(4,0,3),(4,4,3))]
        meta,cells=water.bake_body(4,triangles,(1,0,0))
        width=int(meta[1][3])
        for y in range(int(meta[2][3])):
            for x in range(width):
                c=cells[y*width+x]
                if c[0]>.5:
                    world_x=meta[0][0]+(x+.5)*.5
                    self.assertAlmostEqual(c[0]-512,1+world_x*.5)

    def test_overhead_bridge_does_not_block_water(self):
        bridge=[((0,0,1.5),(4,0,1.5),(0,4,1.5)),((0,4,1.5),(4,0,1.5),(4,4,1.5))]
        _,base=water.bake_body(1,self.square(),(0,0,0))
        _,actual=water.bake_body(1,self.square(),(0,0,0),solids=bridge)
        self.assertEqual(base,actual)

    def test_wall_blocks_and_outlet_absorbs(self):
        wall=[((2,0,-2),(2,4,-2),(2,0,2)),((2,4,-2),(2,4,2),(2,0,2))]
        meta,cells=water.bake_body(1,self.square(),(0,0,0),solids=wall,
                                   open_edges=[{'a':(0,0,0),'b':(0,4,0),'width':1}])
        width=int(meta[1][3])
        self.assertLessEqual(cells[4*width+4][0],0)
        self.assertGreater(cells[4*width+1][2],0)
        self.assertEqual(cells[4*width+7][2],0)

    def test_binary_offsets_and_body_indices(self):
        bodies=[water.bake_body(1,self.square(z=z),(0,0,0)) for z in (0,10)]
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'water.bin'
            count=water.write_bake(path,bodies,b'x'*32)
            raw=path.read_bytes()
            self.assertEqual(struct.unpack_from('<8sIII',raw), (b'ORWATER1',3,2,count))
            self.assertEqual(len(raw),52+(10+2*count)*16)
            second_offset=struct.unpack_from('<f',raw,52+(5+3)*16+12)[0]
            self.assertEqual(second_offset,len(bodies[0][1]))
            cell_offset=52+10*16+int(second_offset)*16
            self.assertEqual(struct.unpack_from('<f',raw,cell_offset+12)[0],1.)

    def test_rest_mesh_preserves_overlapping_art_layers(self):
        surfaces=[(2,self.square(z=z),(0,0,0)) for z in (3.,3.2)]
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'water.mesh'
            water.write_rest_mesh(path,surfaces,fingerprint=b'x'*32)
            raw=path.read_bytes()
            self.assertEqual(struct.unpack_from('<8sI',raw),(b'ORWMESH2',4))
            self.assertEqual(raw[16:48],b'x'*32)
            self.assertGreater(len(raw),48+4*40)
            self.assertAlmostEqual(struct.unpack_from('<I9f',raw,48)[3],3.)
            self.assertAlmostEqual(struct.unpack_from('<I9f',raw,48+80)[3],3.2,places=5)

    def test_dense_mesh_spacing_area_and_uv_mapping(self):
        triangle=((0,0,0),(100,0,0),(100,.25,0))
        weights=water.dense_triangle_weights(triangle)
        self.assertLess(len(weights),20000)
        points=[tuple(sum(p[axis]*v for p,v in zip(triangle,(1-u-v,u,v))) for axis in range(2)) for u,v in weights]
        area=0
        for i in range(0,len(points),3):
            a,b,c=points[i:i+3]
            area+=abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))*.5
            for p,q in ((a,b),(b,c),(c,a)):
                self.assertLessEqual(math.dist(p,q),.50001)
        self.assertAlmostEqual(area,12.5)
        self.assertTrue(all(u>=0 and v>=0 and u+v<=1 for u,v in weights))

    def test_hydraulics_floor_and_still_pond(self):
        body,cells=water.bake_body(1,self.square(z=1),(0,0,0))
        floor=self.square(z=.5)
        field=water.hydraulic_field(body,cells,floor)
        for c,h in zip(cells,field):
            if c[0]>.5:
                self.assertAlmostEqual(h[0],.5)
                self.assertEqual(h[1:3],(0.,0.))
        body,cells=water.bake_body(4,self.square(z=1),(1,0,0))
        field=water.hydraulic_field(body,cells,floor)
        self.assertTrue(any(math.hypot(*h[1:3])>.5 for h in field))

    def test_optical_depth_is_not_clamped_to_solver_depth(self):
        body,cells=water.bake_body(1,self.square(z=8),(0,0,0))
        field=water.hydraulic_field(body,cells,self.square(z=0))
        wet=[h for c,h in zip(cells,field) if c[0]>.5]
        self.assertTrue(wet)
        for h in wet:
            self.assertEqual(h[0],1.5)
            self.assertAlmostEqual(h[3],8.)

    def test_flow_rest_mesh_averages_legacy_wave_endpoints(self):
        ram=bytearray(32*1024*1024)
        struct.pack_into('<6f',ram,0x1cad00,16,16,-8,-8,1,1)
        struct.pack_into('<3f',ram,0x1e34c0,0,0,1)
        struct.pack_into('<H',ram,0x1e34c0+0x1e,1)
        struct.pack_into('<I',ram,0x1fa590,1)
        pointer=0x300000
        for address,count in ((0x1e2fc0,8),(0x1e3380,2),(0x1fbc80,4),(0x202d80,3)):
            for i in range(count):
                struct.pack_into('<I',ram,address+i*0x60,pointer)
                struct.pack_into('<I',ram,address+i*0x60+0xc,3)
                for j,point in enumerate(((0,0,2,4),(1,0,4,6),(0,1,6,8))):
                    struct.pack_into('<4f',ram,pointer+j*16,*point)
                pointer+=0x40
        surfaces=water.read_surfaces(ram)
        self.assertEqual({kind for kind,_,_ in surfaces},{1,2,3,4,5,6,7})
        self.assertTrue(all(flow==(0.,0.,0.) for kind,_,flow in surfaces if kind>=5))
        # The draw callback hides off-camera pond groups by clearing +0x1e.
        # The bake must retain their authored geometry regardless of visibility.
        struct.pack_into('<H',ram,0x1e34c0+0x1e,0)
        self.assertEqual(water.read_surfaces(ram),surfaces)
        struct.pack_into('<3f',ram,0x1e34c0+0x1190,32,0,1)
        struct.pack_into('<I',ram,0x1fa590+4,1)
        self.assertEqual(sum(kind==1 for kind,_,_ in water.read_surfaces(ram)),2)
        for surface,triangles,_ in water.read_surfaces(ram):
            if surface!=1:
                self.assertEqual([p[2] for p in triangles[0]],[3.,5.,7.])

    def test_pal_reads_its_own_records(self):
        at=water.ADDRESSES['pal']
        ram=bytearray(32*1024*1024)
        struct.pack_into('<6f',ram,at['grid'],16,16,-8,-8,1,1)
        struct.pack_into('<3f',ram,at['tiles'],0,0,1)
        struct.pack_into('<I',ram,at['masks'],1)
        pointer=0x300000
        for address,count in zip(at['layers'],(8,2,4,3)):
            for i in range(count):
                struct.pack_into('<I',ram,address+i*0x60,pointer)
                struct.pack_into('<I',ram,address+i*0x60+0xc,3)
                for j,point in enumerate(((0,0,2,4),(1,0,4,6),(0,1,6,8))):
                    struct.pack_into('<4f',ram,pointer+j*16,*point)
                pointer+=0x40
        self.assertEqual({kind for kind,_,_ in water.read_surfaces(ram,'pal')},{1,2,3,4,5,6,7})
        with self.assertRaises(ValueError): water.read_surfaces(ram)
        with tempfile.TemporaryDirectory() as temp:
            manifest=Path(temp)/'manifest.json'
            self.assertEqual(water.disc_version(Path(temp)/'water.bin'),'ntsc')
            manifest.write_text('{"version":"pal"}')
            self.assertEqual(water.disc_version(Path(temp)/'water.bin'),'pal')
            manifest.write_text('{"version":"jp"}')
            with self.assertRaises(ValueError): water.disc_version(Path(temp)/'water.bin')

    def test_cave_pools_have_no_invented_current_or_outlets(self):
        for source in (5,6,7):
            body,cells=water.bake_body(source,self.square(z=1),(0,0,0))
            self.assertEqual(body[4][:2],(0.,0.))
            self.assertTrue(all(c[2]==0 for c in cells))
            self.assertTrue(all(h[1:3]==(0.,0.) for h in water.hydraulic_field(body,cells,())))

    def test_exported_profile_and_explicit_override(self):
        with tempfile.TemporaryDirectory() as temp:
            output=Path(temp)/'water.bin'
            self.assertEqual(water.load_boundaries(output),{})
            profile=output.parent/'water-boundaries.json'
            configured={'open_edges':[{'a':[0,0,1],'b':[2,0,1],'width':2}]}
            profile.write_text(json.dumps(configured))
            self.assertEqual(water.load_boundaries(output),configured)
            override=output.parent/'override.json'
            override.write_text('{"open_edges":[]}')
            self.assertEqual(water.load_boundaries(output,override),{'open_edges':[]})
            with self.assertRaises(FileNotFoundError):
                water.load_boundaries(output,output.parent/'missing.json')
            profile.write_text('{"open_edges":[{"a":[0,0,1],"b":[1,0,1],"width":-1}]}')
            with self.assertRaises(ValueError): water.load_boundaries(output)

    def test_invalid_inputs_rejected(self):
        with self.assertRaises(ValueError): water.read_surfaces(bytes(32))
        with self.assertRaises(ValueError): water.bake_body(1,[((math.nan,0,0),(1,0,0),(0,1,0))],(0,0,0))


if __name__=='__main__': unittest.main()
