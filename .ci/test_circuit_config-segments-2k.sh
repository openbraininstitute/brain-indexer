#! /usr/bin/env bash

set -e

if [[ ! -z ${DATADIR} ]]
then
    export SI_DATADIR=${DATADIR}/spatial_index
fi

if [[ -z ${SI_DATADIR} ]]
then
    echo "SI_DATADIR not set."
    exit -1
fi

circuit_config_seg="${SI_DATADIR}/circuit_config-2k.json"

pushd ${SI_DATADIR}

output_dir=$(mktemp -d ~/tmp-spatial_index-XXXXX)
direct_spi="${output_dir}/direct.spi"
circuit_spi="${output_dir}/circuit.spi"

spatial-index-nodes nodes.h5 ascii_sonata -o "${direct_spi}"
spatial-index-circuit segments "${circuit_config_seg}" -o "${circuit_spi}"

if ! spatial-index-compare segments "${direct_spi}" "${circuit_spi}"
then
    echo "The output from '*-nodes' and '*-circuit' differ."
    exit -1
fi
