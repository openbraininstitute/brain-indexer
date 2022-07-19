"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from Spatial Index are the same as the
    results from FLAT Indexer, ran on the same scx-v6-2k dataset.
"""

import numpy as np
import logging
import os
import sys
from spatial_index import MorphIndexBuilder
from spatial_index import MorphMultiIndexBuilder
try:
    import pytest
    pytest_skipif = pytest.mark.skipif
    pytest_long = pytest.mark.long
except ImportError:
    pytest_skipif = lambda *x, **kw: lambda y: y  # noqa
    pytest_long = lambda x: x  # noqa

# Loading some small circuits and morphology files on BB5
CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k"
CIRCUIT_FILE = CIRCUIT_2K + "/circuit.mvd3"
MORPH_FILE = CIRCUIT_2K + "/morphologies/ascii"


class Options:
    use_mem_mapped_file = False


def do_multi_index_query_serial(min_corner, max_corner):
    from mpi4py import MPI

    output_dir = "tmp-jdiwo"
    MorphMultiIndexBuilder.from_mvd_file(
        MORPH_FILE, CIRCUIT_FILE,
        target_gids=range(700, 1200),
        output_dir=output_dir,
        progress=False,
        return_indexer=True)

    if MPI.COMM_WORLD.Get_rank() == 0:
        index = MorphMultiIndexBuilder.open_index(output_dir, max_subtrees=100)
        return index.find_intersecting_window(min_corner, max_corner)


def do_query_serial(min_corner, max_corner):
    if Options.use_mem_mapped_file:
        logging.info("Using mem-mapped")
        memmap_props = MorphIndexBuilder.DiskMemMapProps(
            "mapfile.bin",
            1000,   # ~1GB
            True    # Truncate
        )
    else:
        memmap_props = None

    index = MorphIndexBuilder.from_mvd_file(MORPH_FILE, CIRCUIT_FILE,
                                            disk_mem_map=memmap_props,
                                            target_gids=range(700, 1200),
                                            progress=True)
    idx = index.find_intersecting_window(min_corner, max_corner)
    index.find_intersecting_window_pos(min_corner, max_corner)
    index.find_intersecting_window_objs(min_corner, max_corner)
    return idx


def query_window():
    min_corner = np.array([-50, -50, -50], dtype=np.float32)
    max_corner = np.array([50, 50, 50], dtype=np.float32)
    return min_corner, max_corner


def check_vs_FLAT(si_ids):
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


@pytest_skipif(not os.path.exists(CIRCUIT_FILE),
               reason="Circuit file not available")
@pytest_long
def test_validation_FLAT():
    check_vs_FLAT(do_query_serial(*query_window()))


@pytest_skipif(True,
               reason="Awaits better MPI integration with tests.")
@pytest_long
def test_multi_index_validation_FLAT():
    from mpi4py import MPI

    idx = do_multi_index_query_serial(*query_window())

    if MPI.COMM_WORLD.Get_rank() == 0:
        check_vs_FLAT(idx)


if __name__ == "__main__":
    run_multi_index = len(sys.argv) > 1 and sys.argv[1] == '--run-multi-index'

    if run_multi_index:
        test_multi_index_validation_FLAT()
