#!/bin/bash -l

# Set the name of the job and the project account
#SBATCH --job-name=FLAT_syn_benchmark
#SBATCH --account=proj16

# Set the time required
#SBATCH --time=01:00:00

# Define the number of nodes and partition 
#SBATCH --nodes=1
#SBATCH --partition=prod_p2
#SBATCH --exclusive

# Configure to use all the memory
#SBATCH --mem=0

DPLACE='env LD_PRELOAD=/usr/lib64/libnuma.so dplace' #SDKGenerator needs LD_PRELOAD to find libnuma.so

#Load old nix modules
export MODULEPATH=/nix/var/nix/profiles/per-user/modules/bb5-x86_64/module-skylae/release/share/modulefiles:/nix/var/nix/profiles/per-user/modules/bb5-x86_64/modules-all/release/share/modulefiles:$MODULEPATH
module load nix/hpc/flatindexer-py3/1.8.12 nix/python/3.6-full

start_global=$(date +%s%N | cut -b1-13) #start measuring time for the whole process
#start=$(date +%s%N | cut -b1-13) #start measuring time for Indexing

#Run indexing (if necessary)
#$DPLACE SDKGenerator /gpfs/bbp.cscs.ch/home/bellotta/proj/blueconfigs/quick-hip-multipopulation/BlueConfig synapse Mosaic SYNAPSE 1>&2
#end=$(date +%s%N | cut -b1-13) #stop measuring time for Indexing
#elapsed_indexing=$(python3<<<"print(($end - $start)/1000)") #save indexing time

#Run simple box query
elapsed_query=$($DPLACE python3 FLAT_syn_benchmark.py 2>output_syn_FI.out) 
end_global=$(date +%s%N | cut -b1-13) 
elapsed_global=$(python3<<<"print(($end_global - $start_global)/1000)")

echo "$elapsed_global,$elapsed_query" >> time_syn_FI.csv
echo "$elapsed_global,$elapsed_query" #Comma separated results on stdout