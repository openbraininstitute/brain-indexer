#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment index with SONATA and
    perform spatial queries
"""

import os
import sys
import libsonata
from spatial_index import MorphIndexBuilder


# Loading some small circuits and morphology files on BB5
CIRCUIT_1K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v0/circuit-1k"
NODES_FILE = os.path.join(CIRCUIT_1K, "nodes.h5")
MORPH_FILE = os.path.join(CIRCUIT_1K, "morphologies/ascii")


def example_sonata_index():
    # Create a new indexer and load the nodes and morphologies
    # directly from the SONATA file
    index = MorphIndexBuilder.from_sonata_file(MORPH_FILE, NODES_FILE, "All",
                                               target_gids=range(700, 900),
                                               progress=True)
    print("Index size:", len(index))

    # Way #1 - Get the ids, then query the node file for ANY data
    window = [200, 200, 480], [300, 300, 520]
    ids_in_region = index.window_query(*window, fields="ids")
    print("Found N points:", len(ids_in_region))

    # Way #2 - Get the queried object directly
    obj_in_region = index.window_query(*window, fields="raw_elements")
    print("Found N objects:", len(obj_in_region))
    for i, obj in enumerate(obj_in_region):
        print("Point #%d: GID: %s Section ID: %s Segment ID: %s Centroid: %s" %
              (i, obj.gid, obj.section_id, obj.segment_id, obj.centroid))

    # Optionally, you can also use a SONATA selection to create a new index
    # and then query it using the usual methods
    selection = libsonata.Selection(values=[4, 8, 15, 16, 23, 42])
    index_selection = MorphIndexBuilder.from_sonata_selection(
        MORPH_FILE, NODES_FILE, "All", selection
    )
    print("Index size:", len(index_selection))
    inds = index_selection.window_query(
        [15, 900, 15], [20, 1900, 20], fields="raw_elements"
    )
    print("Found N objects:", len(inds))
    for i, obj in enumerate(inds):
        print("Point #%d: GID: %s Section ID: %s Segment ID: %s Centroid: %s" %
              (i, obj.gid, obj.section_id, obj.segment_id, obj.centroid))


if __name__ == "__main__":
    if len(sys.argv) > 2:
        NODES_FILE = sys.argv[1]
        MORPH_FILE = sys.argv[2]
    if not os.path.exists(NODES_FILE):
        print("NODE file is not available:", NODES_FILE)
        sys.exit(1)
    if not os.path.exists(MORPH_FILE):
        print("MORPH file is not available:", MORPH_FILE)
        sys.exit(1)
    example_sonata_index()
