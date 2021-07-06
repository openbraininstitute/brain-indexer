import os.path
import numpy.testing as nptest
from collections import namedtuple

from spatial_index.node_indexer import NodeMorphIndexer

_CURDIR = os.path.dirname(__file__)
FILETEST = "tests/data/circuit_mod.mvd3"
MORPHOLOGY_FILES = [
    os.path.join(_CURDIR, "data/soma_spherical.h5"),
    os.path.join(_CURDIR, "data/soma_extended.h5")
]


def test_morph_loading():
    import morphio
    import quaternion as npq
    morph_info = namedtuple("morph_info", ("morph", "rotation", "translation"))
    m1 = morph_info(morphio.Morphology(MORPHOLOGY_FILES[0]), (1, 0, 0, 0), (1, 1, 1))
    m2 = morph_info(
        morphio.Morphology(MORPHOLOGY_FILES[1]),
        (0.8728715609439696, 0.4364357804719848, 0.2182178902359924, 0.0),
        (1, .5, .25)
    )

    def compute_final_points(m):
        rot_quat = npq.quaternion(m.rotation[3], *m.rotation[:3]).normalized()
        soma_pts = m.translation  # by definition soma is at 0,0,0
        soma_rad = m.morph.soma.max_distance
        section_pts = npq.rotate_vectors(rot_quat, m.morph.points) + m.translation
        return soma_pts, soma_rad, section_pts

    soma_pt, soma_rad, section_pts = compute_final_points(m1)
    nptest.assert_allclose(soma_pt, [1., 1., 1.])
    nptest.assert_almost_equal(soma_rad, 0, decimal=6)
    m1_s3_p12 = section_pts[m1.morph.section_offsets[2] + 12]
    nptest.assert_allclose(m1_s3_p12, [7.98622, 13.17931, 18.53813], rtol=1e-6)

    soma_pt, soma_rad, section_pts = compute_final_points(m2)
    nptest.assert_allclose(soma_pt, [1., 0.5, 0.25])
    nptest.assert_almost_equal(soma_rad, 4.41688, decimal=6)
    m2_s3_p12 = section_pts[m2.morph.section_offsets[2] + 12]
    nptest.assert_allclose(m2_s3_p12, [25.1764, 3.07876, 34.16223], rtol=1e-6)


def test_serial_exec():
    NodeMorphIndexer(MORPHOLOGY_FILES[1], FILETEST)


def test_parallel_exec():
    NodeMorphIndexer.create_parallel(MORPHOLOGY_FILES[1], FILETEST)
