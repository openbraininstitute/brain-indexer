#!/bin/env python
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

from spatial_index.synapse_indexer import SynapseIndexer
from libsonata import Selection
import h5py

import os.path
import sys

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, os.pardir, "tests", "data", "edges_2k.h5")

def example_syn_index():
    index = SynapseIndexer.from_sonata_file(EDGE_FILE, "All")
    print("Index size:", len(index))

    f = h5py.File(EDGE_FILE, 'r')
    ds = f['/edges/All/0/afferent_center_x']

    # Way #1 - Get the ids, then query the edge file for ANY data
    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    print("Found N points:", len(points_in_region))

    z_coords = index.edges.get_attribute("afferent_center_z", Selection(points_in_region))

    # Way #2, get the objects: position and id directly from index
    objs_in_region = index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])
    for i, obj in enumerate(objs_in_region):
        pos = obj.centroid
        if i % 20 == 0:
            print("Sample synapse id:", obj.gid, "Position", obj.centroid)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        EDGE_FILE = sys.argv[1]
    if not os.path.exists(EDGE_FILE):
        print("EDGE file is not available:", EDGE_FILE)
        sys.exit()
    example_syn_index()
