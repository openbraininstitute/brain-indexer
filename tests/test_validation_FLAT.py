"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from Spatial Index are the same as the
    results from FLAT Indexer, ran on the same scx-v6-2k dataset.
"""

import numpy as np
import os
from spatial_index import NodeMorphIndexer
try:
    import pytest
    pytest_skipif = pytest.mark.skipif
except ImportError:
    pytest_skipif = lambda *x, **kw: lambda y: y  # noqa

# Loading some small circuits and morphology files on BB5
CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k"
CIRCUIT_FILE = CIRCUIT_2K + "/circuit.mvd3"
MORPH_FILE = CIRCUIT_2K + "/morphologies/ascii"


def do_query_serial(min_corner, max_corner):
    indexer = NodeMorphIndexer.create(MORPH_FILE, CIRCUIT_FILE)
    idx = indexer.find_intersecting_window(min_corner, max_corner)
    indexer.find_intersecting_window_pos(min_corner, max_corner)
    indexer.find_intersecting_window_objs(min_corner, max_corner)
    return idx


@pytest_skipif(not os.path.exists(CIRCUIT_FILE),
               reason="Circuit file not available")
def test_validation_FLAT():
    # Defining first and second corner for box query
    min_corner = np.array([-50, -50, -50], dtype=np.float32)
    max_corner = np.array([50, 50, 50], dtype=np.float32)

    si_ids = do_query_serial(min_corner, max_corner)
    assert len(si_ids) > 0

    # Import data/query_2k_v6.csv in a numpy array
    # read as record array, otherwise sort works column-wise
    flat_ids = np.loadtxt(
        "tests/data/query_2k_v6.csv",
        delimiter=",",
        usecols=range(8, 11),
        converters={8: float, 9: float, 10: float},
        dtype=[('gid', np.uint64), ('section_id', np.uint32), ('segment_id', np.uint32)]
    )

    si_ids.sort()
    flat_ids.sort()

    # Check if SI_data and flat_data are the same
    assert np.array_equal(si_ids, flat_ids)

if __name__ == "__main__":
    test_validation_FLAT()
