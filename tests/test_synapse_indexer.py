#!/bin/env python
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import os.path
import pytest
from libsonata import Selection

import spatial_index
from spatial_index import SynapseIndexBuilder

_CURDIR = os.path.dirname(__file__)
EDGE_2K_FILE = os.path.join(_CURDIR, "data", "edges_2k.h5")
pytest_skipif = pytest.mark.skipif


@pytest_skipif(not os.path.exists(EDGE_2K_FILE),
               reason="Edge file not available")
def test_syn_index():
    index = SynapseIndexBuilder.from_sonata_file(EDGE_2K_FILE, "All")
    print("Index size:", len(index))

    sonata_dataset = spatial_index.io.open_sonata_edges(EDGE_2K_FILE, "All")
    assert sonata_dataset.size == len(index)

    # Way #1 - Get the ids, then query the edge file for ANY data
    ids_in_region = index.box_query([200, 200, 480], [300, 300, 520], fields="id")
    print("Found N synapses:", len(ids_in_region))
    assert sonata_dataset.size > len(ids_in_region) > 0

    z_coords = sonata_dataset.get_attribute("afferent_center_z",
                                            Selection(ids_in_region))
    for z in z_coords:
        assert 480 < z < 520

    # Way #2, get the objects: position and id directly from index
    objs_in_region = index.box_query(
        [200, 200, 480], [300, 300, 520],
        fields="raw_elements"
    )
    assert len(objs_in_region) == len(ids_in_region)
    for i, obj in enumerate(objs_in_region):
        pos = obj.centroid
        assert 480 < pos[2] < 520
        if i % 20 == 0:
            print("Sample synapse id:", obj.id, "Position", obj.centroid)

    # Test counting / aggregation
    total_in_region = index.box_counts([200, 200, 480], [300, 300, 520])
    assert len(ids_in_region) == total_in_region

    aggregated = index.box_counts([200, 200, 480], [300, 300, 520], group_by="gid")
    print("Synapses belong to {} neurons".format(len(aggregated)))
    assert len(aggregated) == 19
    assert aggregated[351] == 2
    assert aggregated[159] == 2
    assert aggregated[473] == 4
    assert total_in_region == sum(aggregated.values())
