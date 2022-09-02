#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment index and
    perform spatial queries
"""

import os
import sys

import numpy as np

import spatial_index
from spatial_index import MorphIndexBuilder


# Loading some small circuits and morphology files on BB5

CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit-2k"
NODE_FILE = os.path.join(CIRCUIT_2K, "circuit.mvd3")
MORPH_FILE = os.path.join(CIRCUIT_2K, "morphologies/ascii")
INDEX_FILENAME = "example_segment_index"


def build_segment_index():
    print("Creating circuit index...")
    builder = MorphIndexBuilder(MORPH_FILE, NODE_FILE)
    builder.process_range((700, 750))  # 50 cells
    # builder.process_all()  # Warning: Might exhaust memory
    print("Index contains", len(builder.index), "elements. Saving to disk")
    builder.index.write(INDEX_FILENAME)
    return builder.index


def build_query_segment_index(min_corner=[-50, 0, 0], max_corner=[0, 50, 50]):
    """Example on how to build and query a segment index

    NOTE: The index only contains the ids and 3D positions of the elements
        To retrieve other data of a segment it's necessary to retrieve IDs
        and query the data sources with it (method 1 below)
    """
    if not os.path.exists(INDEX_FILENAME):
        build_segment_index()

    index = spatial_index.open_index(INDEX_FILENAME)
    print(type(index))
    print("Done. Performing queries")

    # Method 1: Obtain the ids only (numpy Nx3)
    ids = index.window_query(min_corner, max_corner, fields="ids")
    print("Number of elements within window:", len(ids))
    if len(ids) > 0:
        gid, section_id, segment_id = ids[0]  # first element indices
    else:
        # No elements found within the window
        return

    # Similar, but query a spherical region
    ids = index._core_index.find_nearest([.0, .0, .0], 10)  # FIXME modernize
    print("Number of elements in spherical region:", len(ids))

    # Method 2: Get the position only directly from the index as numpy Nx3 (3D positions)
    pos = index.window_query(min_corner, max_corner, fields="positions")
    np.savetxt("query_SI_v6.csv", pos, delimiter=",", fmt="%1.3f")

    # Method 3, retrieve the tree objects for ids and position
    found_objects = index.window_query(min_corner, max_corner, fields="raw_elements")
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
    dict_query = index.window_query(min_corner, max_corner)
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
