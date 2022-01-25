import numpy as np
import sys
from spatial_index import node_indexer
from spatial_index import SphereIndex as IndexClass
from timeit import default_timer as timer
from random import uniform

print ("SEGMENT INDEX BENCHMARKING IN PROGRESS... PLEASE WAIT!")

N_QUERIES = int(sys.argv[1]) if len(sys.argv) > 1 else 100

#CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/circuit.mvd3"
#CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/home/bellotta/proj/TestData/circuitBuilding_1000neurons/circuits/circuit.mvd3"
#CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj42/circuits/rat.CA1/20180309/circuit.mvd3"
CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-thalamus/circuit.mvd3"
#MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/morphologies/ascii"
#MORPH_FILE = "/gpfs/bbp.cscs.ch/home/bellotta/proj/TestData/circuitBuilding_1000neurons/morphologies/ascii"
#MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj42/entities/morphologies/20180215/ascii"
MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-thalamus/morph_release/ascii"

# Optional load of dumped index
#indexer = node_indexer.MorphIndex("/gpfs/bbp.cscs.ch/project/proj16/leite/out.spi")

#Serial Execution timing
start_global = timer()
start = timer()
indexer = node_indexer.NodeMorphIndexer(MORPH_FILE, CIRCUIT_FILE)
indexer.process_all(progress=True)
end = timer()
index_time = end - start

print ("Elements in indexer: ", len(indexer))

#Generate a numpy array of N_QUERIES 3D points and fill them with random floating numbers in a certain interval
max_points = np.random.uniform(low=0, high=10, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=-10, high=0, size=(N_QUERIES, 3)).astype(np.float32)

#Query Execution timing
start = timer()
for i in range(N_QUERIES):
    idx = indexer.find_intersecting_window(min_points[i], max_points[i])
end = timer()
query_time = end - start
global_time = timer() - start_global
# End of timing

# Print results
print ("{},{},{}".format(global_time, index_time, query_time), file=sys.stderr)
print ("Total number of results: ")
print (len(idx))

# Option position retrieval for debugging purposes
#pos = indexer.find_intersecting_window_objs(min_points[1], max_points[1])
#print (min_points[1], max_points[1])

# Optional print position of query results for debug purposes
#for i in range(idx.size):
#    gid, segment_i = idx[i]
#print("Coordinates of gid %d segment %d: %s" % (gid, segment_i, pos[i]))

# Optional dump results to file
#np.savetxt('query_SI_1k.csv', pos, delimiter=',', fmt='%1.3f')
