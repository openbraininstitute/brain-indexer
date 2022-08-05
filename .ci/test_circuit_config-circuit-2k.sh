#! /usr/bin/env bash

set -e

SI_DIR="$(dirname "$(realpath "$0")")"/..
source "${SI_DIR}/.ci/si_datadir.sh"

pushd "${SI_DATADIR}"

circuit_config_relfile="circuit_config-2k.json"
edges_relfile=edges_2k.h5
nodes_relfile=nodes.h5
morphology_reldir=ascii_sonata

source ${SI_DIR}/.ci/circuit_config-synapses.sh
source ${SI_DIR}/.ci/circuit_config-segments.sh
