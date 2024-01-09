"""
    Blue Brain Project - Spatial-Index

    Test indexing circuits, from SONATA and morphology libraries
"""
import os

import numpy.testing as nptest

from spatial_index.morphology_builder import MorphIndexBuilder

import morphio
import quaternion as npq
import libsonata
try:
    import pytest
    pytest_skipif = pytest.mark.skipif
except ImportError:
    pytest_skipif = lambda *x, **kw: lambda y: y  # noqa

POPULATION = "All"
_CURDIR = os.path.dirname(__file__)
MORPHOLOGY_FILES = [
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
    m2 = _3DMorphology(
        morphio.Morphology(MORPHOLOGY_FILES[0]),
        (0.8728715609439696, 0.4364357804719848, 0.2182178902359924, 0.0),
        (1, .5, .25)
    )

    soma_pt, soma_rad, section_pts = m2.compute_final_points()
    nptest.assert_allclose(soma_pt, [1., 0.5, 0.25])
    nptest.assert_almost_equal(soma_rad, 4.41688, decimal=6)
    m2_s3_p12 = section_pts[m2.morph.section_offsets[2] + 12]
    nptest.assert_allclose(m2_s3_p12, [25.1764, 3.07876, 34.16223], rtol=1e-6)


class Test2Info:
    MORPHOLOGY_DIR = os.path.join(_CURDIR, "data/ascii_sonata")
    SONATA_NODES = os.path.join(_CURDIR, "data/nodes.h5")


@pytest_skipif(not os.path.exists(Test2Info.SONATA_NODES),
               reason="Circuit file not available")
def test_sonata_index():
    index = MorphIndexBuilder.from_sonata_file(
        Test2Info.MORPHOLOGY_DIR, Test2Info.SONATA_NODES, POPULATION, range(700, 800)
    )
    assert len(index) == 588961
    points_in_region = index.box_query(
        [200, 200, 480], [300, 300, 520],
        fields="raw_elements"
    )
    assert len(points_in_region) > 0
    obj_in_region = index.box_query(
        [0, 0, 0], [10, 10, 10],
        fields="raw_elements"
    )
    assert len(obj_in_region) > 0
    for obj in obj_in_region:
        assert -1 <= obj.centroid[0] <= 11


@pytest_skipif(not os.path.exists(Test2Info.SONATA_NODES),
               reason="Circuit file not available")
def test_sonata_selection():
    selection = libsonata.Selection([4, 8, 15, 16, 23, 42])
    index = MorphIndexBuilder.from_sonata_selection(
        Test2Info.MORPHOLOGY_DIR, Test2Info.SONATA_NODES, POPULATION, selection,
    )

    assert len(index) == 25618
    obj_in_region = index.box_query(
        [15, 900, 15], [20, 1900, 20],
        fields="raw_elements"
    )
    assert len(obj_in_region) > 0
    for obj in obj_in_region:
        assert 13 <= obj.centroid[0] <= 22
