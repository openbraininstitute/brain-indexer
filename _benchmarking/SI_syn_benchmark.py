from spatial_index import SynapseIndexer
from timeit import default_timer as timer
from libsonata import Selection
from random import uniform

import os.path
import sys
import numpy as np

_CURDIR = os.path.dirname(__file__)
EDGE_FILE = "/gpfs/bbp.cscs.ch/project/proj16/leite/circuit_o1v6_edges.h5"
#EDGE_FILE = "/gpfs/bbp.cscs.ch/project/proj42/circuits/CA1.O1/mooc-circuit/sonata/edges/edges.h5"
N_QUERIES = 10000

max_points = np.random.uniform(low=51, high=100, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=0, high=50, size=(N_QUERIES, 3)).astype(np.float32)

print ("SYNAPSE INDEX BENCHMARKING IN PROGRESS... PLEASE WAIT!")

start_global = timer()
start = timer()

#index = SynapseIndexer.from_sonata_file(EDGE_FILE, "All")
#index = SynapseIndexer.from_sonata_file(EDGE_FILE, "hippocampus_neurons__hippocampus_neurons__chemical")
index = SynapseIndexer.from_sonata_file(EDGE_FILE, "default")

end = timer()
index_time = end - start

start = timer()
# Way #1 - Get the ids, then query the edge file for ANY data
for i in range(N_QUERIES):
    points_in_region = index.find_intersecting_window(min_points[i], max_points[i])
    
end = timer()
query_time = end - start

# Way #2, get the objects: position and id directly from index
start = timer()
for i in range(N_QUERIES):
    objs_in_region = index.find_intersecting_window_objs(min_points[i], max_points[i])

end = timer()
query_time2 = end - start

#for i, obj in enumerate(objs_in_region):
#    if i % 20 == 0:
#        print("Sample synapse id:", obj.gid, "Position", obj.centroid)

global_time = timer() - start_global

print ("{},{},{},{}".format(global_time, index_time, query_time, query_time2), file=sys.stderr)
print ("Index size:", len(index))
print("Found N points:", len(points_in_region))
