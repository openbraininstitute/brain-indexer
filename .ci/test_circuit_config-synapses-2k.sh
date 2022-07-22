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

pushd ${SI_DATADIR}

circuit_config_seg="${SI_DATADIR}/circuit_config-2k.json"
edges_file=$(realpath edges_2k.h5)
output_dir=$(mktemp -d ~/tmp-spatial_index-XXXXX)
direct_spi="${output_dir}/direct.spi"
circuit_spi="${output_dir}/circuit.spi"

echo "DATADIR=${DATADIR}"
echo "SI_DATADIR=${SI_DATADIR}"
echo "circuit_config_seg=${circuit_config_seg}"
echo "edges_file=${edges_file}"
echo "output_dir=${output_dir}"
echo "direct_spi=${direct_spi}"
echo "circuit_spi=${circuit_spi}"

spatial-index-synapses ${edges_file} -o ${direct_spi}
spatial-index-circuit synapses "${circuit_config_seg}" -o ${circuit_spi}

if ! cmp "${direct_spi}" "${circuit_spi}"
then
    echo "The output from '*-synapses' and '*-circuit' differ."
    exit -1
fi
