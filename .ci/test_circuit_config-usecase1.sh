#! /usr/bin/env bash

set -e

SI_DIR="$(dirname "$(realpath "$0")")"/..
source "${SI_DIR}/.ci/si_datadir.sh"

usecase_reldir="sonata_usecases/usecase1"

pushd "${SI_DATADIR}/${usecase_reldir}"

circuit_config_relfile="${usecase_reldir}/circuit_sonata.json"
edges_relfile="${usecase_reldir}/edges.h5"
nodes_relfile="${usecase_reldir}/nodes.h5"
morphology_dir="/gpfs/bbp.cscs.ch/project/proj12/example_data/sonata/v2/components/CircuitA/morphologies/swc"

source "${SI_DIR}/.ci/circuit_config-segments.sh"
source "${SI_DIR}/.ci/circuit_config-synapses.sh"
