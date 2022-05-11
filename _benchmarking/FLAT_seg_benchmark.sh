#!/bin/bash -l

# Set the name of the job and the project account
#SBATCH --job-name=FLAT_seg_benchmark
#SBATCH --account=proj16

# Set the time required
#SBATCH --time=01:00:00

# Define the number of nodes and partition 
#SBATCH --nodes=1
#SBATCH --partition=prod_p2
#SBATCH --exclusive

# Configure to use all the memory
#SBATCH --mem=0

# SDKGenerator needs LD_PRELOAD to find libnuma.so
# dplace can be useful to pin the process to a core during benchmarks
DPLACE='env LD_PRELOAD=/usr/lib64/libnuma.so dplace' 

#Load old nix modules
export MODULEPATH=/nix/var/nix/profiles/per-user/modules/bb5-x86_64/module-skylae/release/share/modulefiles:/nix/var/nix/profiles/per-user/modules/bb5-x86_64/modules-all/release/share/modulefiles:$MODULEPATH
module load nix/hpc/flatindexer-py3/1.8.12 nix/python/3.6-full

start_global=$(date +%s%N | cut -b1-13) #start measuring time for the whole process
start=$(date +%s%N | cut -b1-13) #start measuring time for Indexing

#Run indexing. Here are reported the datasets used in the benchmarks.
#$DPLACE SDKGenerator /gpfs/bbp.cscs.ch/project/proj16/bellotta/index_benchmark/BlueConfig_scx_v6_mod segment Mosaic SEGMENT 1>&2
$DPLACE SDKGenerator /gpfs/bbp.cscs.ch/home/bellotta/proj/TestData/circuitBuilding_1000neurons/BlueConfig3.in segment Column SEGMENT 1>&2
#$DPLACE SDKGenerator /gpfs/bbp.cscs.ch/home/bellotta/proj/blueconfigs/thalamus/BlueConfig segment All SEGMENT 1>&2
end=$(date +%s%N | cut -b1-13) #stop measuring time for Indexing
elapsed_indexing=$(python3<<<"print(($end - $start)/1000)") #save indexing time

#Run simple box query
elapsed_query=$($DPLACE python3 FLAT_seg_benchmark.py 2>>output_seg_FI.out) 
end_global=$(date +%s%N | cut -b1-13) 
elapsed_global=$(python3<<<"print(($end_global - $start_global)/1000)")

echo "$elapsed_global,$elapsed_indexing,$elapsed_query" >> time_seg_FI.csv
echo "$elapsed_global,$elapsed_indexing,$elapsed_query" #Comma separated results on stdout