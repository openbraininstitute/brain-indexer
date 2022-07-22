#!/bin/env python
"""
    Blue Brain Project - Spatial-Index

    A small example script on how to create a synapse index and
    perform spatial queries using memory mapped files
"""

from spatial_index import SynapseIndexBuilder

# Pre-made index locations
PRE_MADE_INDEX = "/gpfs/bbp.cscs.ch/project/proj16/bellotta/memory_map"
PRE_MADE_INDEX_FILE = PRE_MADE_INDEX + "/synapse_index.bin"

# Test edges file location and population name
EDGES_PATH = "/gpfs/bbp.cscs.ch/project/proj42/circuits/CA1.O1/mooc-circuit/sonata/edges"
EDGES_FILE = EDGES_PATH + "/edges.h5"
POPULATION = "hippocampus_neurons__hippocampus_neurons__chemical"


# Loading pre-made index that you can
# create using the spatial-index-nodes command
index_pre = SynapseIndexBuilder.load_disk_mem_map(PRE_MADE_INDEX_FILE)

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
disk_mem_map = SynapseIndexBuilder.DiskMemMapProps("syn_map.bin", 2048, True)

# Then create a SynapseIndexBuilder object specifying
# the path to the edges file and the population name

index = SynapseIndexBuilder.from_sonata_file(
    EDGES_FILE,
    POPULATION,
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
