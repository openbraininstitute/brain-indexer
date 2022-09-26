#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a multi index from
    SONATA edge files; load the multi-index and perform some
    queries.

    Must be run with MPI, e.g., one of:
        mpiexec -n5 python synapse_multi_index_sonata.py
        srun -n5 python synapse_multi_index_sonata.py

    Detailed advice on picking the `srun` parameters can be found
    in the documentation.
"""
import os.path

from mpi4py import MPI

import spatial_index
from spatial_index import SynapseMultiIndexBuilder


CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v2/circuit-2k"
EDGE_FILE = os.path.join(CIRCUIT_2K, "edges.h5")
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
        core_index = spatial_index.open_index(OUTPUT_DIR, max_cache_size_mb=100)

        # Define a query window by its two extreme corners, and run the
        # query.
        min_corner, max_corner = [200, 200, 480], [300, 300, 520]
        found = core_index.box_query(min_corner, max_corner)

        # Now you can start doing science:
        print(found)


if __name__ == "__main__":
    example_create_multi_index_from_sonata()
    example_query_multi_index()
