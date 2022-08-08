#!/bin/env bash
#
#SBATCH --job-name="seg_mmap_test_SI"
#SBATCH --time=1:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --account=proj16
#SBATCH --partition=prod
#SBATCH --constraint=nvme,skl
set -ex

# Activate the virtualenv where SpatialIndex is installed
. venv/bin/activate

# Set the paths to the nodes, morphology and output files
TMP_FILE="/nvme/$USER/$SLURM_JOB_ID/segment_index.bin"
NODES_FILE=/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/circuit.mvd3
MORPHOLOGY_LIB=/gpfs/bbp.cscs.ch/project/proj12/jenkins/cellular/circuit-2k/morphologies/ascii

# Run spatial-index-nodes to generate the indexed file
spatial-index-nodes --use-mem-map=1000000 --shrink-on-close $NODES_FILE $MORPHOLOGY_LIB -o $TMP_FILE

# Copy the indexed file to the desired directory when finished 
mv $TMP_FILE ./
