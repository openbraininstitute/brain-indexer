#! /usr/bin/env bash

set -e

if [[ -z ${SI_DIR} ]]
then
    echo "SI_DIR not set."
    exit -1
fi

output_dir=$(mktemp -d ~/tmp-spatial_index-XXXXX)


# This circuit config expects the CWD to be the directory in which the config
# file resides.

pushd ${SI_DIR}/tests/data
circuit_config_seg="circuit_config-2k.json"

# One-liner to generate an index of segments.
spatial-index-circuit segments "${circuit_config_seg}" -o "${output_dir}/circuit"

# One-liner to generate an index of synapses.
spatial-index-circuit synapses "${circuit_config_seg}" -o "${output_dir}/circuit"
