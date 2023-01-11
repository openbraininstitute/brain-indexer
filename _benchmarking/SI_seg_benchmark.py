import numpy as np
import sys

from spatial_index import MorphIndexBuilder
from timeit import default_timer as timer

import spatial_index

print("SEGMENT INDEX BENCHMARKING IN PROGRESS... PLEASE WAIT!")

N_QUERIES = int(sys.argv[1]) if len(sys.argv) > 1 else 10000

CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/nodes.h5"
MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/morphologies/ascii"

# Serial Execution timing
start_global = timer()
start = timer()
builder = MorphIndexBuilder(MORPH_FILE, CIRCUIT_FILE)
builder.process_all(progress=False)

# Alternatively you can use the uniform index created
# using the create_uniform_index.py script
# index = spatial_index.open_index("uniform_index")

end = timer()
index_time = end - start

index = builder.index

print("Elements in index: ", len(index))

# Generate a numpy array of N_QUERIES 3D points
# and fill them with random floating numbers in a certain interval
max_points = np.random.uniform(low=0, high=10, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=-10, high=0, size=(N_QUERIES, 3)).astype(np.float32)

# Query Execution timing
start = timer()
for i in range(N_QUERIES):
    idx = index.box_query(min_points[i], max_points[i])
end = timer()
query_time = end - start
global_time = timer() - start_global
# End of timing

# Print results
print("{},{},{}".format(global_time, index_time, query_time), file=sys.stderr)
print("Last number of results: ")
print(len(idx['gid']))
