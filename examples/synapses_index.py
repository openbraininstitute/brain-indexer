#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to load circuits,
    index them and perform some queries
"""

from spatial_index import SynapseIndexBuilder
from libsonata import Selection

import os.path
import sys

_CURDIR = os.path.dirname(__file__)
CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v0/circuit-2k"
EDGE_FILE = os.path.join(CIRCUIT_2K, "edges.h5")


def example_syn_index():
    # Creating a synapse indexer by using the `return_indexer=True` flag
    indexer = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, "All")
    print("Index size:", len(indexer.index))

    # Specify the corners for the box query
    min_corner = [200, 200, 480]
    max_corner = [300, 300, 520]

    # Method #1 - Get the ids, then query the edge file for ANY data
    points_in_region = indexer.index.find_intersecting_window(min_corner,
                                                              max_corner)
    print("Found N points:", len(points_in_region))

    z_coords = indexer.dataset.get_attribute("afferent_center_z",
                                             Selection(points_in_region))
    print("First 10 Z coordinates: ", z_coords[:10])

    # Method #2, get the objects: position and id directly from index
    objs_in_region = indexer.index.find_intersecting_window_objs(min_corner,
                                                                 max_corner)
    for i, obj in enumerate(objs_in_region):
        if i % 20 == 0:
            print("Sample synapse id:", obj.id, "Position", obj.centroid)
            print("Post-syn Neuron gid:", obj.post_gid,
                  "pre-syn Neuron gid:", obj.pre_gid)

    # Method #3, get the information as a dictionary of numpy arrays
    # Information for synapses includes: id, pre_gid, post_gid,
    # centroid and kind.
    dict_query = indexer.index.find_intersecting_window_np(min_corner,
                                                           max_corner)
    print(dict_query)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        EDGE_FILE = sys.argv[1]
    if not os.path.exists(EDGE_FILE):
        print("EDGE file is not available:", EDGE_FILE)
        sys.exit(1)
    example_syn_index()
