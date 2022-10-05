#! /usr/bin/env bash

set -e

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Single rank usage: $0 JUPYTER_NOTEBOOK"
    echo "Multi rank usage: $0 JUPYTER_NOTEBOOK NRANKS"
    exit -1
fi

notebook="$1"
jupyter nbconvert --to python "${notebook}"

if [[ $# -eq 2 ]]
then
    srun -n $2 ipython "${notebook%.ipynb}".py
else
    ipython "${notebook%.ipynb}".py
fi
