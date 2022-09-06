"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from the numpy output routine are the same as the
    results from the slim object output.
    The test is performed on both a Segment and Synapse index.
"""

import numpy as np
import os.path

from spatial_index import MorphIndexBuilder, SynapseIndexBuilder

try:
    import pytest
    pytest_skipif = pytest.mark.skipif
    pytest_long = pytest.mark.long
except ImportError:
    pytest_skipif = lambda *x, **kw: lambda y: y  # noqa
    pytest_long = lambda x: x  # noqa

# Circuit and morphology files for segment index test
CIRCUIT_FILE = "tests/data/circuit_mod.mvd3"
MORPH_FILE = "tests/data/soma_extended.h5"

# Edge file for synapse index test
_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, "data", "edges_2k.h5")


@pytest_skipif(not os.path.exists(EDGE_FILE),
               reason="Edge file not available")
def test_numpy_output_syn():
    index = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, "All")

    min_corner = [200, 200, 480]
    max_corner = [300, 300, 520]

    np_out = index.window_query(min_corner, max_corner)
    obj_out = index.window_query(min_corner, max_corner, fields="raw_elements")

    for i, obj in enumerate(obj_out):
        assert np.array_equal(obj.id, np_out['id'][i])
        assert np.array_equal(obj.pre_gid, np_out['pre_gid'][i])
        assert np.array_equal(obj.post_gid, np_out['post_gid'][i])
        assert np.array_equal(obj.centroid, np_out['position'][i])


def test_numpy_output_seg():
    builder = MorphIndexBuilder(MORPH_FILE, CIRCUIT_FILE)
    builder.process_range((700, 750))  # 50 cells
    index = builder.index

    print(f"{type(builder)}")
    print(f"{type(index)}")

    min_corner = [-50, 0, 0]
    max_corner = [0, 50, 50]

    np_out = index.window_query(min_corner, max_corner)
    obj_out = index.window_query(min_corner, max_corner, fields="raw_elements")

    for i, obj in enumerate(obj_out):
        assert np.array_equal(obj.gid, np_out['gid'][i])
        assert np.array_equal(obj.section_id, np_out['section_id'][i])
        assert np.array_equal(obj.segment_id, np_out['segment_id'][i])
        assert np.array_equal(obj.endpoints[0], np_out['endpoint1'][i])
        assert np.array_equal(obj.endpoints[1], np_out['endpoint2'][i])
