"""
    Blue Brain Project - Spatial-Index

    Test indexing circuits, from mvd and morphology libraries
"""
import logging
import os

import numpy.testing as nptest
from spatial_index.node_indexer import MorphIndexBuilder
import morphio
import quaternion as npq
import libsonata
try:
    import pytest
    pytest_skipif = pytest.mark.skipif
except ImportError:
    pytest_skipif = lambda *x, **kw: lambda y: y  # noqa


_CURDIR = os.path.dirname(__file__)
FILETEST = "tests/data/circuit_mod.mvd3"
MORPHOLOGY_FILES = [
    os.path.join(_CURDIR, "data/soma_spherical.h5"),
    os.path.join(_CURDIR, "data/soma_extended.h5")
]


class _3DMorphology:
    __slots__ = ("morph", "rotation", "translation")

    def __init__(self, morph, rotation, translation):
        self.morph, self.rotation, self.translation = morph, rotation, translation

    def compute_final_points(self):
        """ The kernel used to compute final positions, inspiring the final impl.
            We test it against known data (test_morph_loading) and against the core impl.
        """
        rot_quat = npq.quaternion(self.rotation[3], *self.rotation[:3]).normalized()
        soma_pts = self.translation  # by definition soma is at 0,0,0
        soma_rad = self.morph.soma.max_distance
        section_pts = npq.rotate_vectors(rot_quat, self.morph.points) + self.translation
        return soma_pts, soma_rad, section_pts


def test_morph_loading():
    m1 = _3DMorphology(
        morphio.Morphology(MORPHOLOGY_FILES[0]),
        (1, 0, 0, 0),
        (1, 1, 1)
    )
    m2 = _3DMorphology(
        morphio.Morphology(MORPHOLOGY_FILES[1]),
        (0.8728715609439696, 0.4364357804719848, 0.2182178902359924, 0.0),
        (1, .5, .25)
    )

    soma_pt, soma_rad, section_pts = m1.compute_final_points()
    nptest.assert_allclose(soma_pt, [1., 1., 1.])
    nptest.assert_almost_equal(soma_rad, 0, decimal=6)
    m1_s3_p12 = section_pts[m1.morph.section_offsets[2] + 12]
    nptest.assert_allclose(m1_s3_p12, [7.98622, 13.17931, 18.53813], rtol=1e-6)

    soma_pt, soma_rad, section_pts = m2.compute_final_points()
    nptest.assert_allclose(soma_pt, [1., 0.5, 0.25])
    nptest.assert_almost_equal(soma_rad, 4.41688, decimal=6)
    m2_s3_p12 = section_pts[m2.morph.section_offsets[2] + 12]
    nptest.assert_allclose(m2_s3_p12, [25.1764, 3.07876, 34.16223], rtol=1e-6)


def test_serial_exec():
    node_indexer = MorphIndexBuilder(MORPHOLOGY_FILES[1], FILETEST)
    node_indexer.process_range((0, 1))  # Process first neuron
    index = node_indexer.index
    assert len(index) > 1700
    objs_in_region = index.find_intersecting_window_objs([100, 50, 100], [200, 100, 200])
    assert len(objs_in_region) == 23

    m = _3DMorphology(
        morphio.Morphology(MORPHOLOGY_FILES[1]),
        node_indexer.mvd.rotations(0)[0],
        node_indexer.mvd.positions(0)[0],
    )
    _, _, final_section_pts = m.compute_final_points()

    for obj in objs_in_region:
        assert 49 <= obj.centroid[1] <= 101  # centroid can be sightly outside the area
        # Compare obtained centroid to raw point extracted from mvd and processed
        seq_point_i = m.morph.section_offsets[obj.section_id - 1] + obj.segment_id
        point_built_from_mvd = final_section_pts[seq_point_i]
        # The point is the start of the segment, hence give good tolerance
        nptest.assert_allclose(obj.centroid, point_built_from_mvd, atol=0.5)


def test_memory_mapped_file_morph_index():
    MEM_MAP_FILE = "rtree_image.bin"
    mem_map_props = MorphIndexBuilder.DiskMemMapProps(MEM_MAP_FILE, 1, True)
    node_indexer = MorphIndexBuilder(MORPHOLOGY_FILES[1], FILETEST,
                                     mem_map_props=mem_map_props)
    node_indexer.process_range((0, 1))
    index = node_indexer.index
    assert len(index) > 1700
    # We can share the mem file file
    index2 = MorphIndexBuilder.load_disk_mem_map(MEM_MAP_FILE)
    assert len(index2) > 1700, len(index2)


class Test2Info:
    MORPHOLOGY_DIR = os.path.join(_CURDIR, "data/ascii_sonata")
    SONATA_NODES = os.path.join(_CURDIR, "data/nodes.h5")


@pytest_skipif(not os.path.exists(Test2Info.SONATA_NODES),
               reason="Circuit file not available")
def test_sonata_index():
    index = MorphIndexBuilder.from_sonata_file(
        Test2Info.MORPHOLOGY_DIR, Test2Info.SONATA_NODES, "All", range(700, 800)
    )
    assert len(index) == 588961
    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    assert len(points_in_region) == 28
    obj_in_region = index.find_intersecting_window_objs([0, 0, 0], [10, 10, 10])
    assert len(obj_in_region) == 2
    for obj in obj_in_region:
        assert -1 <= obj.centroid[0] <= 11


@pytest_skipif(not os.path.exists(Test2Info.SONATA_NODES),
               reason="Circuit file not available")
def test_sonata_selection():
    selection = libsonata.Selection([4, 8, 15, 16, 23, 42])
    index = MorphIndexBuilder.from_sonata_selection(
        Test2Info.MORPHOLOGY_DIR, Test2Info.SONATA_NODES, "All", selection
    )
    assert len(index) == 25618
    obj_in_region = index.find_intersecting_window_objs([15, 900, 15], [20, 1900, 20])
    assert len(obj_in_region) == 21
    for obj in obj_in_region:
        assert 13 <= obj.centroid[0] <= 22


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    test_serial_exec()
    test_sonata_index()
    test_memory_mapped_file_morph_index()
