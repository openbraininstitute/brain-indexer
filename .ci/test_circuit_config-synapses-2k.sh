#! /usr/bin/env bash

set -e

if [[ -z ${SI_DIR} ]]
then
    echo "SI_DIR not set."
    exit -1
fi

circuit_config_seg="${SI_DIR}/tests/data/circuit_config-2k.json"

pushd ${SI_DIR}/tests/data

output_dir=$(mktemp -d ~/tmp-spatial_index-XXXXX)
spatial-index-synapses edges_2k.h5 -o "${output_dir}/direct.spi"
spatial-index-circuit synapses "${circuit_config_seg}" -o "${output_dir}/circuit.spi"

if ! cmp "${output_dir}/direct.spi" "${output_dir}/circuit.spi"
then
    echo "The output from '*-synapses' and '*-circuit' differ."
    exit -1
fi
