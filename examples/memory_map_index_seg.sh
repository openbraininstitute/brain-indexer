#!/bin/env bash
#
#SBATCH --job-name="hip_spatial_index"
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
TMP_FILE="/nvme/$USER/$SLURM_JOB_ID/segment_index.bin"
NODES_FILE=/gpfs/bbp.cscs.ch/project/proj42/circuits/CA1/20190306/circuit.mvd3
MORPHOLOGY_LIB=/gpfs/bbp.cscs.ch/project/proj42/entities/morphologies/20180417/ascii

# Run spatial-index-nodes to generate the indexed file
spatial-index-nodes --use-mem-map=1200000 $NODES_FILE $MORPHOLOGY_LIB -o $TMP_FILE

# Copy the indexed file to the desired directory when finished 
mv $TMP_FILE ./
