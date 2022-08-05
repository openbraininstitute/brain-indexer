set -e

if [[ -z "${SI_DIR}" ]]
then
    echo "SI_DIR not set."
    exit -1
fi
source "${SI_DIR}/.ci/si_datadir.sh"

circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"
edges_file="${edges_file:-"${SI_DATADIR}/${edges_relfile}"}"

output_dir="$(mktemp -d ~/tmp-spatial_index-XXXXX)"
direct_spi="${output_dir}/direct.spi"
circuit_spi="${output_dir}/circuit.spi"

spatial-index-synapses "${edges_file}" -o "${direct_spi}"
spatial-index-circuit synapses "${circuit_config_file}" -o "${circuit_spi}"

if ! spatial-index-compare synapses "${direct_spi}" "${circuit_spi}"
then
    echo "The output from '*-synapses' and '*-circuit' differ."
    exit -1
fi
