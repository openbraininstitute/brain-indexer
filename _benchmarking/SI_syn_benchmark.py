from spatial_index import SynapseIndexBuilder
from timeit import default_timer as timer

import os.path
import sys
import numpy as np

_CURDIR = os.path.dirname(__file__)

EDGE_FILE = "/gpfs/bbp.cscs.ch/project/proj42/circuits/CA1.O1/mooc-circuit/sonata/edges/edges.h5"
POPULATION = "hippocampus_neurons__hippocampus_neurons__chemical"
N_QUERIES = 10000

max_points = np.random.uniform(low=175, high=180, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=145, high=150, size=(N_QUERIES, 3)).astype(np.float32)

print("SYNAPSE INDEX BENCHMARKING IN PROGRESS... PLEASE WAIT!")

start_global = timer()
start = timer()

index = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, POPULATION)

end = timer()
index_time = end - start

start = timer()
# Get the ids, then query the edge file for ANY data
for i in range(N_QUERIES):
    points_in_region = index.box_query(min_points[i], max_points[i])
end = timer()
query_time = end - start

global_time = timer() - start_global

print("{},{},{}".format(global_time, index_time, query_time), file=sys.stderr)
print("Index size:", len(index))
print("Found N points:", len(points_in_region))
