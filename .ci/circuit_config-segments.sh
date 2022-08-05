set -e

if [[ -z "${SI_DIR}" ]]
then
    echo "SI_DIR not set."
    exit -1
fi
source "${SI_DIR}/.ci/si_datadir.sh"


circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"
nodes_file="${nodes_file:-"${SI_DATADIR}/${nodes_relfile}"}"
morphology_dir="${morphology_dir:-"${SI_DATADIR}/${morphology_reldir}"}"

output_dir="$(mktemp -d ~/tmp-spatial_index-XXXXX)"
direct_spi="${output_dir}/direct.spi"
circuit_spi="${output_dir}/circuit.spi"

spatial-index-nodes "${nodes_file}" "${morphology_dir}" -o "${direct_spi}"
spatial-index-circuit segments "${circuit_config_file}" -o "${circuit_spi}"

if ! spatial-index-compare segments "${direct_spi}" "${circuit_spi}"
then
    echo "The output from '*-nodes' and '*-circuit' differ."
    exit -1
fi
