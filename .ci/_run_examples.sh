#!/bin/bash
# A script that runs all examples in one go
# Please launch in an allocation, e.g. `salloc -Aproj16 -n 6 .ci/_run_examples.sh`

set -euxo pipefail
cd ${SI_DIR:-"."}
pwd

rm -r usecase1 || true
rm -r circuit2k || true
rm -rf tmp-* || true
rm -r example_segment_index || true
rm -rf multi_index_2k || true

python3 examples/segment_index_sonata.py
python3 examples/segment_index.py
python3 examples/synapses_index.py
srun -n5 python3 examples/segment_multi_index_sonata.py
srun -n3 python3 examples/synapse_multi_index_sonata.py
bash .ci/test_circuit_config-circuit-1or2k.sh
bash .ci/test_circuit_config-usecase1.sh
bash .ci/test_circuit_config-usecase2.sh
bash .ci/test_circuit_config-usecase3.sh
bash .ci/test_circuit_config-usecase4.sh
# Usecase5 deactivated since some of the targets specified in the circuit config
# are not yet compatible with SpatialIndex.
# bash .ci/test_circuit_config-usecase5.sh
bash examples/run_ipynb.sh examples/basic_tutorial.ipynb
bash examples/run_ipynb.sh examples/advanced_tutorial.ipynb 3

set +x
echo "[`date`] Example Tests Finished"
