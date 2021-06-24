#Just a small example script on how to load circuits, index them and perform some queries

import numpy as np
from spatial_index import circuit_indexer
from spatial_index import SphereIndex as IndexClass
from timeit import default_timer as timer

CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/circuit.mvd3"
#CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/home/bellotta/proj/TestData/circuitBuilding_1000neurons/circuits/circuit.mvd3"
#CIRCUIT_FILE = "/gpfs/bbp.cscs.ch/project/proj42/circuits/rat.CA1/20180309/circuit.mvd3"
MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/morphologies/ascii"
#MORPH_FILE = "/gpfs/bbp.cscs.ch/home/bellotta/proj/TestData/circuitBuilding_1000neurons/morphologies/ascii"
#MORPH_FILE = "/gpfs/bbp.cscs.ch/project/proj42/entities/morphologies/20180215/ascii"

# Parallel execution currently commented out since not tested yet
#start = timer()
#indexer = circuit_indexer.main_parallel(MORPH_FILE, CIRCUIT_FILE)
#end = timer()
#print(end - start)

start = timer()
indexer = circuit_indexer.main_serial(MORPH_FILE, CIRCUIT_FILE)
end = timer()
print("Serial execution happened in [s]: ")
print(end - start)

min_corner = np.array([-50, -50, -50], dtype=np.float32)
max_corner = np.array([50, 50, 50], dtype=np.float32)
center = np.array([0., 0., 0.], dtype=np.float32)

start = timer()
idx = indexer.spatialindex.find_intersecting_window(min_corner, max_corner)
pos = indexer.spatialindex.find_intersecting_window_pos(min_corner, max_corner)
end = timer()
print(indexer.spatialindex)
print ("Intersecting window query took [s]: ")
print(end - start)
for i in range(idx.size):
    gid, section_id, segment_id = idx[i]
    print("Coordinates of gid %d section %d segment %d: %s" % (gid, section_id, segment_id, pos[i]))

np.savetxt('query_SI_v6.csv', pos, delimiter=',', fmt='%1.3f')
idx = indexer.spatialindex.find_nearest(center, 10)
print("CENTER:")
print(idx)
