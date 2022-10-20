#! /usr/bin/env bash

set -e

SI_DIR="$(dirname "$(realpath "$0")")"/..
source "${SI_DIR}/.ci/si_datadir.sh"

usecase_reldir="sonata_usecases/usecase5"

pushd "${SI_DATADIR}/${usecase_reldir}"

circuit_config_relfile="${usecase_reldir}/circuit_sonata.json"
circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"

output_dir="$(mktemp -d ~/tmp-spatial_index-XXXXX)"

segments_spi="${output_dir}/circuit-segments"
synapses_spi="${output_dir}/circuit-synapses"

spatial-index-circuit segments "${circuit_config_file}" \
    --populations nodeA \
    -o "${segments_spi}"

python3 << EOF
import spatial_index
index = spatial_index.open_index("${segments_spi}")
sphere = [0.0, 0.0, 0.0], 1000.0

results = index.sphere_query(*sphere, fields="gid")
assert "nodeA" in results

for result in results.values():
  assert result.size != 0
EOF

# We want this to fail until 'astrocytes' are supported.
if [ $(spatial-index-circuit segments "${circuit_config_file}" \
                                      --populations astrocyteA \
                                      -o "${segments_spi}" &> /dev/null) ]
then
  echo "'--populations astrocyteA' was accepted, but should not have been."
  exit -1
fi


spatial-index-circuit synapses "${circuit_config_file}" \
    --populations nodeA__nodeA__chemical astrocyteA__astrocyteA__electrical_synapse \
    -o "${synapses_spi}"

python3 << EOF
import spatial_index
index = spatial_index.open_index("${synapses_spi}")
sphere = [0.0, 0.0, 0.0], 1000.0

results = index.sphere_query(*sphere, fields="id")
assert "nodeA__nodeA__chemical" in results
assert "astrocyteA__astrocyteA__electrical_synapse" in results

for result in results.values():
    assert result.size != 0

EOF
