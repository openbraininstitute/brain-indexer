#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment index and
    perform spatial queries using memory mapped files
"""

import os.path

import spatial_index
from spatial_index import MorphIndexBuilder

CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v0/circuit-2k"

# Pre-made index locations
PRE_MADE_INDEX_DIR = os.path.join(CIRCUIT_2K, "indexes")
PRE_MADE_INDEX_FILE = os.path.join(PRE_MADE_INDEX_DIR, "segment_index.bin")

# Test node and morph file locations
NODE_FILE = os.path.join(CIRCUIT_2K, "circuit.mvd3")
MORPH_FILE = os.path.join(CIRCUIT_2K, "morphologies/ascii")

# Loading pre-made index that you can
# create using the spatial-index-nodes command
index_pre = spatial_index.open_index(PRE_MADE_INDEX_FILE)

# Perform queries normally
min_corner = [0, 0, 0]
max_corner = [50, 50, 50]

ids = index_pre.find_intersecting_window(min_corner, max_corner)
print("Pre-Made Index - Number of elements within window:", len(ids))

# Otherwise you can create the index from scratch.

# WARNING: This functionality has to be considered extremely experimental!!!
# In most cases you should use a pre-made index created
# using the spatial-index-nodes command.
# You need to resort to this method only as an exceptional measure.
# If you REALLY think you need to create a new big index
# please contact the HPC team or the main developers of Spatial Index.

# Specify the index output filename, size and shrink on close
disk_mem_map = MorphIndexBuilder.DiskMemMapProps("seg_map.bin", 2048, True)

# Then create a MorphIndexBuilder object specifying
# the path to the morphology directory and the nodes file
index = MorphIndexBuilder.create(
    MORPH_FILE,
    NODE_FILE,
    gids=range(700, 900),
    disk_mem_map=disk_mem_map,
    progress=True
)

# Then perform queries on the index normally
# A memory-mapped index will be also saved
# You can load it for following queries
min_corner = [0, 0, 0]
max_corner = [50, 50, 50]

ids = index.find_intersecting_window(min_corner, max_corner)

print("New Index - Number of elements within window:", len(ids))
