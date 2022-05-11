#!/bin/bash -l

# Set the name of the job and the project account
#SBATCH --job-name=SI_seg_benchmark
#SBATCH --account=proj16

# Set the time required
#SBATCH --time=01:00:00

# Define the number of nodes and partition 
#SBATCH --nodes=1
#SBATCH --partition=prod_p2
#SBATCH --exclusive

# Configure to use all the memory
#SBATCH --mem=0

# Load modules
module load unstable spatial-index

#for i in $(seq 1 1 10)
#do
# dplace can be useful to pin the process to a core during benchmarks
dplace python ./SI_seg_benchmark.py >> output_seg_SI.out 2>> time_seg_SI.csv
#done
