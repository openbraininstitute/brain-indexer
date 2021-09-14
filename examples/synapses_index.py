#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to load circuits,
    index them and perform some queries
"""

from spatial_index import SynapseIndexer
from libsonata import Selection

import os.path
import sys

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, os.pardir, "tests", "data", "edges_2k.h5")


def example_syn_index():
    index = SynapseIndexer.from_sonata_file(EDGE_FILE, "All")
    print("Index size:", len(index))

    # Way #1 - Get the ids, then query the edge file for ANY data
    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    print("Found N points:", len(points_in_region))

    z_coords = index.edges.get_attribute("afferent_center_z", Selection(points_in_region))
    print("First 10 Z coordinates: ", z_coords[:10])

    # Way #2, get the objects: position and id directly from index
    objs_in_region = index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])
    for i, obj in enumerate(objs_in_region):
        if i % 20 == 0:
            print("Sample synapse id:", obj.gid, "Position", obj.centroid)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        EDGE_FILE = sys.argv[1]
    if not os.path.exists(EDGE_FILE):
        print("EDGE file is not available:", EDGE_FILE)
        sys.exit(1)
    example_syn_index()
