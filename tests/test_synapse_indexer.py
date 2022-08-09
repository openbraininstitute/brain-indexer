#!/bin/env python
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import h5py
import numpy
import os.path
import pytest
from libsonata import Selection
from spatial_index import SynapseIndexBuilder

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, "data", "edges_2k.h5")
pytest_skipif = pytest.mark.skipif


@pytest.fixture(scope="module")
def syn_index():
    """Returns the extended index built from a Sonata file
    The low-level index is avail at .raw_index
    """
    return SynapseIndexBuilder.from_sonata_file(EDGE_FILE, "All")


_CURDIR = os.path.dirname(__file__)
EDGE_FILE = os.path.join(_CURDIR, "data", "edges_2k.h5")


@pytest_skipif(not os.path.exists(EDGE_FILE),
               reason="Edge file not available")
def test_syn_index(syn_index):
    index = syn_index
    print("Index size:", len(index))

    f = h5py.File(EDGE_FILE, 'r')
    ds = f['/edges/All/0/afferent_center_x']
    assert len(ds) == len(index)

    # Way #1 - Get the ids, then query the edge file for ANY data
    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    print("Found N synapses:", len(points_in_region))
    assert len(ds) > len(points_in_region) > 0

    z_coords = syn_index.dataset.get_attribute("afferent_center_z",
                                               Selection(points_in_region))
    for z in z_coords:
        assert 480 < z < 520

    # Way #2, get the objects: position and id directly from index
    objs_in_region = index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])
    assert len(objs_in_region) == len(points_in_region)
    for i, obj in enumerate(objs_in_region):
        pos = obj.centroid
        assert 480 < pos[2] < 520
        if i % 20 == 0:
            print("Sample synapse id:", obj.id, "Position", obj.centroid)

    # Test counting / aggregation
    total_in_region = index.count_intersecting([200, 200, 480], [300, 300, 520])
    assert len(points_in_region) == total_in_region

    aggregated = index.count_intersecting_agg_gid([200, 200, 480], [300, 300, 520])
    print("Synapses belong to {} neurons".format(len(aggregated)))
    assert len(aggregated) == 19
    assert aggregated[351] == 2
    assert aggregated[159] == 2
    assert aggregated[473] == 4
    assert total_in_region == sum(aggregated.values())


@pytest_skipif(not os.path.exists(EDGE_FILE),
               reason="Edge file not available")
def test_extended_syn_index(syn_index):
    WINDOW = ([200, 200, 480], [300, 300, 520])
    result = syn_index.window_q(*WINDOW)
    assert isinstance(result, dict)
    assert 'afferent_section_id' in result
    assert len(result['afferent_section_id']) == len(result['id']) > 10

    ids = syn_index.window_q(*WINDOW, output="ids")
    assert numpy.array_equal(result["id"], ids)
