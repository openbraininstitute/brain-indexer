#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment index and
    perform spatial queries
"""

import numpy as np
import os
import sys
from spatial_index import MorphIndexBuilder


# Loading some small circuits and morphology files on BB5

CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k"
NODE_FILE = CIRCUIT_2K + "/circuit.mvd3"
MORPH_FILE = CIRCUIT_2K + "/morphologies/ascii"
INDEX_FILENAME = "example_segment_index.spi"


def build_segment_index():
    print("Creating circuit index...")
    indexer = MorphIndexBuilder(MORPH_FILE, NODE_FILE)
    indexer.process_range((700, 750))  # 50 cells
    # indexer.process_all()  # Warning: Might exhaust memory
    print("Index contains", len(indexer.index), "elements. Saving to disk")
    indexer.index.dump(INDEX_FILENAME)
    return indexer.index


def build_query_segment_index(min_corner=[-50, 0, 0], max_corner=[0, 50, 50]):
    """Example on how to build and query a segment index

    NOTE: The index only contains the ids and 3D positions of the elements
        To retrieve other data of a segment it's necessary to retrieve IDs
        and query the data sources with it (method 1 below)
    """
    if not os.path.isfile(INDEX_FILENAME):
        indexer = build_segment_index()
        print(type(indexer))
    else:
        print("Loading from existing segment index:", INDEX_FILENAME)
        indexer = MorphIndexBuilder.load_dump(INDEX_FILENAME)
        print(type(indexer))
    print("Done. Performing queries")

    # Method 1: Obtain the ids only (numpy Nx3)
    ids = indexer.find_intersecting_window(min_corner, max_corner)
    print("Number of elements within window:", len(ids))
    if len(ids) > 0:
        gid, section_id, segment_id = ids[0]  # first element indices
    else:
        # No elements found within the window
        return

    # Similar, but query a spherical region
    ids = indexer.find_nearest([.0, .0, .0], 10)
    print("Number of elements in spherical region:", len(ids))

    # Method 2: Get the position only directly from the index as numpy Nx3 (3D positions)
    pos = indexer.find_intersecting_window_pos(min_corner, max_corner)
    np.savetxt("query_SI_v6.csv", pos, delimiter=",", fmt="%1.3f")

    # Method 3, retrieve the tree objects for ids and position
    found_objects = indexer.find_intersecting_window_objs(min_corner, max_corner)
    for i, obj in enumerate(found_objects):
        object_ids = obj.ids  # as tuple of gid, section, segment  # noqa
        # Individual propertioes
        print("Segment ids:", obj.gid, obj.section_id, obj.segment_id,
              "Centroid:", obj.centroid)
        if i >= 20:
            print("...")
            break

    # Method 4, retrieve all the information in the payload
    # and output them as a dictionary of numpy arrays.
    # Segment information includes: gid, section_id, segment_id
    # radius, endpoint1/2 and kind.
    dict_query = indexer.find_intersecting_window_np(min_corner, max_corner)
    print(dict_query)


if __name__ == "__main__":
    nargs = len(sys.argv)
    if nargs not in (1, 3):
        print("Usage:", sys.argv[0], "[ <node_file_(mvd/sonata)> <morphology_dir> ]")
        sys.exit(1)
    if len(sys.argv) == 3:
        NODE_FILE, MORPH_FILE = sys.argv[1:3]
    if not os.path.exists(NODE_FILE):
        print("Node file is not available:", NODE_FILE)
        sys.exit(1)

    build_query_segment_index()
