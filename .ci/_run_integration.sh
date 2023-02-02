#!/bin/bash
# A script that runs all integration tests in one go

set -euxo pipefail
cd ${SI_DIR:-"."}
pwd

rm -rf tmp-* || true

python3 .ci/test_validation_FLAT.py
srun -n5 python3 .ci/test_validation_FLAT.py --run-multi-index
python3 .ci/test_bluepy.py
python3 .ci/test_sonata_sanity.py

set +x
echo "[`date`] Integration Tests Finished"
