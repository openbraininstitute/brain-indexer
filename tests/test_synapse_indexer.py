#!/bin/env python
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

from spatial_index.synapse_indexer import SynapseIndexer
from libsonata import Selection
import h5py


def test_syn_index():
    EDGE_FILE = ("/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/"
                 "touches/functional/circuit_from_parquet.h5")
    index = SynapseIndexer(EDGE_FILE, None, "All")
    print("Index size:", len(index))

    f = h5py.File(EDGE_FILE, 'r')
    ds = f['/edges/All/0/afferent_center_x']
    assert len(ds) == len(index)

    # Way #1 - Get the ids, then query the edge file for ANY data
    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    print("Found N points:", len(points_in_region))
    assert len(ds) > len(points_in_region) > 0

    z_coords = index.edges.get_attribute("afferent_center_z", Selection(points_in_region))
    for z in z_coords:
        assert 480 < z < 520

    # Way #2, get the objects: position and id directly from index
    objs_in_region = index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])
    assert len(objs_in_region) == len(points_in_region)
    for i, obj in enumerate(objs_in_region):
        pos = obj.centroid
        assert 480 < pos[2] < 520
        if i % 20 == 0:
            print("Sample synapse id:", obj.gid, "Position", obj.centroid)


if __name__ == "__main__":
    test_syn_index()
