#!/bin/sh
set -e

if [ $# -eq 0 ]; then
    echo "Please specify name of the Virtualenv to create. If a venv with the same name exists, it will be used."
    exit 1
fi

module load unstable python

#check if the virtualenv exists
if [ -d $1 ]; then
    echo "Virtualenv $1 already exists"
else
    python -m venv "$1"
fi

module load unstable cmake gcc boost/1.70.0
. "$1"/bin/activate
pip install git+ssh://git@bbpgitlab.epfl.ch/hpc/SpatialIndex.git
