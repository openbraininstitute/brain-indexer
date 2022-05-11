#!/bin/env bash
#
#SBATCH --job-name="syn_mmap_index"
#SBATCH --time=48:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --account=proj16
#SBATCH --partition=prod
#SBATCH --constraint=nvme,skl
#SBATCH --qos=longjob
set -ex

# Activate the virtualenv where SpatialIndex is installed
. venv/bin/activate

# Set the paths to the nodes, morphology and output files
TMP_FILE="/nvme/$USER/$SLURM_JOB_ID/synapse_index.bin"
POPULATION="hippocampus_neurons__hippocampus_neurons__chemical"
EDGES_FILE=/gpfs/bbp.cscs.ch/project/proj42/circuits/CA1.O1/mooc-circuit/sonata/edges/edges.h5

# Run spatial-index-nodes to generate the indexed file
spatial-index-synapses --use-mem-map=1000000 --shrink-on-close $EDGES_FILE $POPULATION -o $TMP_FILE

# Copy the indexed file to the desired directory when finished 
mv $TMP_FILE ./
