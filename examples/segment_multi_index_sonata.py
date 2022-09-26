#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment multi-index
    with SONATA; load the multi-index and perform some queries.

    Must be run with MPI, e.g., one of:
        mpiexec -n5 python segment_multi_index_sonata.py
        srun -n5 python segment_multi_index_sonata.py

    Detailed advice on picking the `srun` parameters can be found
    in the documentation.
"""

import os

from mpi4py import MPI

import spatial_index
from spatial_index import MorphMultiIndexBuilder

# Loading some small circuits and morphology files on BB5
CIRCUIT_1K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v2/circuit-1k"
NODES_FILE = os.path.join(CIRCUIT_1K, "nodes.h5")
MORPH_FILE = os.path.join(CIRCUIT_1K, "morphologies/ascii")

OUTPUT_DIR = "tmp-doei"


def example_create_multi_index_from_sonata():
    # Create a new indexer and load the nodes and morphologies
    # directly from the SONATA file
    MorphMultiIndexBuilder.from_sonata_file(
        MORPH_FILE,
        NODES_FILE,
        "All",
        output_dir=OUTPUT_DIR,
    )


def example_query_multi_index():
    if MPI.COMM_WORLD.Get_rank() == 0:
        # The index may use at most roughly 1e6 bytes.
        core_index = spatial_index.open_index(OUTPUT_DIR, max_cache_size_mb=100)

        # Define a query window by its two extreme corners, and run the
        # query.
        min_corner, max_corner = [-50, 0, 0], [0, 50, 50]
        found = core_index.box_query(min_corner, max_corner)

        # Now you're ready for the real science:
        print(found)


if __name__ == "__main__":
    example_create_multi_index_from_sonata()
    example_query_multi_index()
