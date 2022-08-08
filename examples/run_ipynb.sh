#! /usr/bin/env bash

set -e

if [[ $# -ne 1 ]]
then
    echo "Usage: $0 JUPYTER_NOTEBOOK"
    exit -1
fi

notebook="$1"

jupyter nbconvert --to python "${notebook}"
ipython "${notebook%.ipynb}".py
