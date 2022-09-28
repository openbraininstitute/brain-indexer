#!/bin/bash
# A script that runs the MPI test through pytest. Please launch in an
# allocation, e.g.,
#    salloc --account proj16 -n3 .ci/run_mpi_pytest.sh

srun -n3 pytest -m "mpi" --with-mpi
