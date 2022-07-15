#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment multi index
    with SONATA (and perform spatial queries).
"""

from mpi4py import MPI

from spatial_index import MorphMultiIndexBuilder

# Loading some small circuits and morphology files on BB5
CIRCUIT_1K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-1k"
NODES_FILE = CIRCUIT_1K + "/nodes.h5"
MORPH_FILE = CIRCUIT_1K + "/morphologies/ascii"

OUTPUT_DIR = "tmp-doei"


def example_create_multi_index_from_sonata():
    # Create a new indexer and load the nodes and morphologies
    # directly from the SONATA file
    MorphMultiIndexBuilder.from_sonata_file(
        MORPH_FILE,
        NODES_FILE,
        "All",
        output_dir=OUTPUT_DIR,
        return_indexer=False
    )


def example_query_multi_index_from_sonata():
    if MPI.COMM_WORLD.Get_rank() == 0:
        index = MorphMultiIndexBuilder.open_index(OUTPUT_DIR, max_subtrees=100)

        min_corner, max_corner = [-50, 0, 0], [0, 50, 50]

        ids = index.find_intersecting_window(min_corner, max_corner)
        print("Number of elements within window:", len(ids))
        if len(ids) > 0:
            gid, section_id, segment_id = ids[0]


if __name__ == "__main__":
    example_create_multi_index_from_sonata()
    example_query_multi_index_from_sonata()
