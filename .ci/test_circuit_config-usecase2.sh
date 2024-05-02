#! /usr/bin/env bash

set -e

SI_DIR="$(dirname "$(realpath "$0")")"/..
source "${SI_DIR}/.ci/si_datadir.sh"

usecase_reldir="sonata_usecases/usecase2"

pushd "${SI_DATADIR}/${usecase_reldir}"

circuit_config_relfile="${usecase_reldir}/circuit_sonata.json"
circuit_config_file="${circuit_config_file:-"${SI_DATADIR}/${circuit_config_relfile}"}"

output_dir="$(mktemp -d ~/tmp-brain_indexer-XXXXX)"

segments_spi="${output_dir}/circuit-segments"
segments_NodeA_spi="${output_dir}/circuit-NodeA-segments"
synapses_spi="${output_dir}/circuit-synapses"

brain-indexer-circuit segments "${circuit_config_file}" -o "${segments_spi}"
python3 << EOF
import brain_indexer
index = brain_indexer.open_index("${segments_spi}")
sphere = [0.0, 0.0, 0.0], 1000.0

result = index.sphere_query(*sphere, fields="gid")
assert result.size != 0, f"'result.size' isn't zero."
EOF

brain-indexer-circuit segments "${circuit_config_file}" --populations "NodeA" -o "${segments_NodeA_spi}"
python3 << EOF
import brain_indexer
index = brain_indexer.open_index("${segments_spi}")
sphere = [0.0, 0.0, 0.0], 1000.0

result = index.sphere_query(*sphere, fields="gid")
assert result.size != 0, f"'result.size' isn't zero."
EOF


brain-indexer-circuit synapses "${circuit_config_file}" \
    --populations NodeA__NodeA__chemical VirtualPopA__NodeA__chemical \
    -o "${synapses_spi}"

python3 << EOF
import brain_indexer
index = brain_indexer.open_index("${synapses_spi}")
sphere = [0.0, 0.0, 0.0], 1000.0

results = index.sphere_query(*sphere, fields="id")
for pop in ["NodeA__NodeA__chemical", "VirtualPopA__NodeA__chemical"]:
    assert pop in results, f"'{pop}' missing."

for result in results.values():
    assert result.size != 0, f"{pop}: size isn't zero."

EOF
