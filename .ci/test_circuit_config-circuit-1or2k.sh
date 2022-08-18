#! /usr/bin/env bash

set -e

SI_DIR="$(dirname "$(realpath "$0")")"/..
source "${SI_DIR}/.ci/si_datadir.sh"

circuit_reldir="circuit-1or2k"

pushd "${SI_DATADIR}/${circuit_reldir}"

circuit_config_relfile="${circuit_reldir}/circuit_sonata.json"
edges_relfile="${circuit_reldir}/edges_2k.h5"
nodes_relfile="${circuit_reldir}/nodes.h5"
morphology_reldir="${circuit_reldir}/ascii_sonata"
n_mpi_ranks=3

source ${SI_DIR}/.ci/circuit_config-synapses.sh
source ${SI_DIR}/.ci/circuit_config-segments.sh
