#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a circuit segment index and
    perform spatial queries using memory mapped files
"""

from spatial_index import MorphIndexBuilder

# Pre-made index locations
PRE_MADE_INDEX_2K = "/gpfs/bbp.cscs.ch/project/proj16/bellotta/memory_map"
PRE_MADE_INDEX_FILE = PRE_MADE_INDEX_2K + "/segment_index.bin"

# Test node and morph file locations
CIRCUIT_2K = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k"
NODE_FILE = CIRCUIT_2K + "/circuit.mvd3"
MORPH_FILE = CIRCUIT_2K + "/morphologies/ascii"

# Loading pre-made index that you can
# create using the spatial-index-nodes command
index_pre = MorphIndexBuilder.load_disk_mem_map(PRE_MADE_INDEX_FILE)

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
disk_mem_map = MorphIndexBuilder.DiskMemMapProps("mem_map.bin", 2048, True, True)

# Then create a MorphIndexBuilder object specifying
# the path to the morphology directory and the nodes file
index = MorphIndexBuilder.create(
    MORPH_FILE,
    NODE_FILE,
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
