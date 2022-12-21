#!/bin/env python
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import os.path
import pytest
from libsonata import Selection

import spatial_index
from spatial_index import SynapseIndexBuilder

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, "data", "edges.h5")
pytest_skipif = pytest.mark.skipif
POPULATION = "neocortex_neurons__chemical_synapse"


@pytest_skipif(not os.path.exists(EDGE_FILE),
               reason="Edge file not available")
def test_syn_index():
    index = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, POPULATION)
    print("Index size:", len(index))

    sonata_dataset = spatial_index.io.open_sonata_edges(EDGE_FILE, POPULATION)
    assert sonata_dataset.size == len(index)

    query_shape = index.bounds()

    # Way #1 - Get the ids, then query the edge file for ANY data
    ids_in_region = index.box_query(*query_shape, fields="id")
    print("Found N synapses:", len(ids_in_region))
    assert sonata_dataset.size == len(ids_in_region) > 0

    z_coords = sonata_dataset.get_attribute("afferent_center_z",
                                            Selection(ids_in_region))
    for z in z_coords:
        assert query_shape[0][2] <= z <= query_shape[1][2]

    # Test counting / aggregation
    total_in_region = index.box_counts(*query_shape)
    assert len(ids_in_region) == total_in_region

    aggregated = index.box_counts(*query_shape, group_by="post_gid")
    print("Synapses belong to {} neurons".format(len(aggregated)))
    assert len(aggregated) == 47
    assert aggregated[4] == 30
    assert aggregated[8] == 14
    assert aggregated[10] == 21
    assert total_in_region == sum(aggregated.values())
