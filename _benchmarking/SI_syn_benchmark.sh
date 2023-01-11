#!/bin/bash -l

# Set the name of the job and the project account
#SBATCH --job-name=SI_syn_benchmark
#SBATCH --account=proj16

# Set the time required
#SBATCH --time=01:00:00

# Define the number of nodes and partition 
#SBATCH --nodes=1
#SBATCH --partition=prod
#SBATCH --exclusive

# Configure to use all the memory
#SBATCH --mem=0

# Load modules
module load unstable spatial-index

# Or alternatively load your venv
# . venv/bin/activate

for i in $(seq 1 1 5)
do
# dplace can be useful to pin the process to a core during benchmarks
    dplace python3 ./SI_syn_benchmark.py >> output_syn_SI.out 2>> time_syn_SI.csv
done
