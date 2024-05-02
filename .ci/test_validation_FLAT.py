"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from Spatial Index are the same as the
    results from FLAT Indexer, ran on the same scx-v6-2k dataset.
"""

import numpy as np
import os
import sys

import brain_indexer
from brain_indexer import MorphIndexBuilder

# Loading some small circuits and morphology files on BB5
CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k"
CIRCUIT_FILE = os.path.join(CIRCUIT_2K, "nodes.h5")
MORPH_FILE = os.path.join(CIRCUIT_2K, "morphologies/ascii")
POPULATION = "All"

# File containing the results from FLAT Indexer
REF_PATH = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v5/FLAT_validation"
REF_FILE = os.path.join(REF_PATH, "query_2k_v6.csv")


def do_multi_index_query_serial(corner, opposite_corner):
    from mpi4py import MPI
    from brain_indexer import MorphMultiIndexBuilder

    output_dir = "tmp-jdiwo"
    MorphMultiIndexBuilder.from_sonata_file(
        MORPH_FILE, CIRCUIT_FILE, POPULATION,
        gids=range(700, 1200),
        output_dir=output_dir
    )

    if MPI.COMM_WORLD.Get_rank() == 0:
        index = brain_indexer.open_index(output_dir, max_cache_size_mb=1000)
        return index.box_query(corner, opposite_corner, fields="ids")


def do_query_serial(corner, opposite_corner):
    index = MorphIndexBuilder.from_sonata_file(
        MORPH_FILE, CIRCUIT_FILE, POPULATION,
        gids=range(700, 1200),
        progress=True
    )

    idx = index.box_query(corner, opposite_corner, fields="ids")
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
        REF_FILE,
        delimiter=",",
        usecols=range(8, 11),
        converters={8: float, 9: float, 10: float},
        dtype=[('gid', np.uint64), ('section_id', np.uint32), ('segment_id', np.uint32)]
    )

    # Add 1 to every element of si_ids gid field.
    # This is done because FLAT is 1-indexed.
    si_ids['gid'] += 1

    si_ids.sort()
    flat_ids.sort()

    # Check if SI_data and flat_data are the same
    assert np.array_equal(si_ids, flat_ids)


def test_validation_FLAT():
    check_vs_FLAT(do_query_serial(*query_window()))
    print("Success! In-memory index and FLAT don't differ.")


def test_multi_index_validation_FLAT():
    from mpi4py import MPI

    idx = do_multi_index_query_serial(*query_window())

    if MPI.COMM_WORLD.Get_rank() == 0:
        check_vs_FLAT(idx)

    MPI.COMM_WORLD.Barrier()
    if MPI.COMM_WORLD.Get_rank() == 0:
        print("Success! Multi-index and FLAT don't differ.")


if __name__ == "__main__":
    run_multi_index = len(sys.argv) > 1 and sys.argv[1] == '--run-multi-index'

    if not run_multi_index:
        test_validation_FLAT()

    else:
        test_multi_index_validation_FLAT()
