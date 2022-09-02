set -e

if [[ -z "${SI_DIR}" ]]
then
    echo "SI_DIR not set."
    exit -1
fi
source "${SI_DIR}/.ci/si_datadir.sh"
source "${SI_DIR}/.ci/si_assert_indexes_are_equal.sh"

circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"
edges_file="${edges_file:-"${SI_DATADIR}/${edges_relfile}"}"

output_dir="$(mktemp -d ~/tmp-spatial_index-XXXXX)"

direct_spi="${output_dir}/direct"
circuit_spi="${output_dir}/circuit"

spatial-index-synapses "${edges_file}" -o "${direct_spi}"
spatial-index-circuit synapses "${circuit_config_file}" -o "${circuit_spi}"

assert_indexes_are_equal "${direct_spi}" "${circuit_spi}"

if [[ ! -z ${n_mpi_ranks} ]]
then
    multi_direct_spi="${output_dir}/multi_direct"
    multi_circuit_spi="${output_dir}/multi_circuit"

    srun -n${n_mpi_ranks} spatial-index-synapses "${edges_file}" -o "${multi_direct_spi}" --multi-index
    srun -n${n_mpi_ranks} spatial-index-circuit synapses "${circuit_config_file}" -o "${multi_circuit_spi}" --multi-index

    assert_indexes_are_equal "${direct_spi}" "${multi_direct_spi}"
    assert_indexes_are_equal "${direct_spi}" "${multi_circuit_spi}"
fi
