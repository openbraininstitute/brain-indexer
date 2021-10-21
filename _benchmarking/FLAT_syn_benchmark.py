import numpy as np
import sys
import libFLATIndex as FI
from numpy.lib.npyio import savetxt
from timeit import default_timer as timer
from random import uniform

# Set N_QUERIES to the the first parameter of the command line as an integer with a default value of 100000
N_QUERIES = int(sys.argv[1]) if len(sys.argv) > 1 else 10000

#idx = FI.loadIndex("/gpfs/bbp.cscs.ch/home/bellotta/proj/blueconfigs/scx-2k-v6/SEGMENT")
idx = FI.loadIndex("/gpfs/bbp.cscs.ch/project/proj64/circuits/O1.v6a/20181207/connectome/functional/SYNAPSE") #Load already generated Index from Flat

max_points = np.random.uniform(low=0, high=50, size=(N_QUERIES, 3)).astype(np.float32)
min_points = np.random.uniform(low=51, high=100, size=(N_QUERIES, 3)).astype(np.float32)

start = timer()

for i in range(N_QUERIES):
    result = np.array(FI.windowQuery(idx,min_points[i][0],min_points[i][1],min_points[i][2],max_points[i][0],max_points[i][1],max_points[i][2])) #Simple Box Query

end = timer()
query_time = end - start
print(query_time)
#result.dump("dump_1k.txt")
#savetxt('query_1k.csv', result, delimiter=',', fmt='%1.3f') #print results to external file
print(result, file=sys.stderr) #push results to std.error
