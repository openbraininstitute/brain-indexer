#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a multi index from
    SONATA edge files; load the multi-index and perform some
    queries.

    Must be run with MPI:
        mpiexec -n5 python synapse_multi_index_sonata.py
        srun -n5 python synapse_multi_index_sonata.py
"""

from spatial_index import SynapseMultiIndexBuilder
from mpi4py import MPI

import os.path

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, os.pardir, "tests", "data", "edges_2k.h5")
OUTPUT_DIR = "tmp-vnwe"


def example_create_multi_index_from_sonata():
    # A multi index of synapses is built using the `SynapseMultiIndexBuilder`.
    SynapseMultiIndexBuilder.from_sonata_file(
        EDGE_FILE,
        "All",
        output_dir=OUTPUT_DIR
    )


def example_query_multi_index():
    if MPI.COMM_WORLD.Get_rank() == 0:
        # The index may use at most roughly 1e6 bytes.
        index = SynapseMultiIndexBuilder.open_index(OUTPUT_DIR, mem=int(1e6))

        # Define a query window by its two extreme corners, and run the
        # query.
        min_corner, max_corner = [200, 200, 480], [300, 300, 520]
        found = index.find_intersecting_window_np(min_corner, max_corner)

        # Now you can start doing science:
        print(found)


if __name__ == "__main__":
    example_create_multi_index_from_sonata()
    example_query_multi_index()
