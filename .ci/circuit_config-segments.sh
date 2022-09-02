set -e

if [[ -z "${SI_DIR}" ]]
then
    echo "SI_DIR not set."
    exit -1
fi
source "${SI_DIR}/.ci/si_datadir.sh"
source "${SI_DIR}/.ci/si_assert_indexes_are_equal.sh"


circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"
nodes_file="${nodes_file:-"${SI_DATADIR}/${nodes_relfile}"}"
morphology_dir="${morphology_dir:-"${SI_DATADIR}/${morphology_reldir}"}"

output_dir="$(mktemp -d ~/tmp-spatial_index-XXXXX)"
direct_spi="${output_dir}/direct"
circuit_spi="${output_dir}/circuit"

spatial-index-nodes "${nodes_file}" "${morphology_dir}" -o "${direct_spi}"
spatial-index-circuit segments "${circuit_config_file}" -o "${circuit_spi}"

assert_indexes_are_equal "${direct_spi}" "${circuit_spi}"

if [[ ! -z ${n_mpi_ranks} ]]
then
    multi_direct_spi="${output_dir}/multi_direct"
    multi_circuit_spi="${output_dir}/multi_circuit"

    srun -n${n_mpi_ranks} spatial-index-nodes "${nodes_file}" "${morphology_dir}" -o "${multi_direct_spi}" --multi-index
    srun -n${n_mpi_ranks} spatial-index-circuit segments "${circuit_config_file}" -o "${multi_circuit_spi}" --multi-index

    assert_indexes_are_equal "${direct_spi}" "${multi_direct_spi}"
    assert_indexes_are_equal "${direct_spi}" "${multi_circuit_spi}"
fi
