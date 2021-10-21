import numpy as np
import sys
import libFLATIndex as FI
from numpy.lib.npyio import savetxt
from timeit import default_timer as timer
from random import uniform

# Set N_QUERIES to the the first parameter of the command line as an integer with a default value of 100000
N_QUERIES = int(sys.argv[1]) if len(sys.argv) > 1 else 100

start = timer()

# By default, the FLAT index loaded is the one created by `FLAT_seg_benchmark.sh` 
# but you can also load a different one (some examples are provided here and commented out) 

#idx = FI.loadIndex("/gpfs/bbp.cscs.ch/home/bellotta/proj/blueconfigs/scx-2k-v6/SEGMENT")
#idx = FI.loadIndex("/gpfs/bbp.cscs.ch/project/proj83/circuits/Bio_M/20200805/SEGMENT")
#idx = FI.loadIndex("/gpfs/bbp.cscs.ch/project/proj64/circuits/O1.v6a/20181207/SEGMENT")

idx = FI.loadIndex("./SEGMENT") #Load already generated Index from Flat

import_time = timer() - start

#Generate a numpy array of N_QUERIES 3D points and fill them with random floating numbers in a certain interval
max_points = np.random.uniform(low=0, high=20, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=-20, high=0, size=(N_QUERIES, 3)).astype(np.float32)

start = timer()

for i in range(N_QUERIES):
    result = np.array(FI.windowQuery(idx,min_points[i][0],min_points[i][1],min_points[i][2],max_points[i][0],max_points[i][1],max_points[i][2])) #Simple Box Query

end = timer()
query_time = end - start

print(query_time)

print("RESULTS LENGTH: ", len(result), file=sys.stderr)
print("Import time: ", import_time, file=sys.stderr)

# Optional save results to file and std.error

#result.dump("dump.dat")
#savetxt('query_1k.csv', result, delimiter=',', fmt='%1.3f')
#print(result, file=sys.stderr) #push results to std.error
