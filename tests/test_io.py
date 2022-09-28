import pytest

import os
from spatial_index.io import shared_temporary_directory


@pytest.mark.mpi
def test_mpi_temporary_directory():
    from mpi4py import MPI

    comm = MPI.COMM_WORLD
    with shared_temporary_directory(prefix="foo", mpi_comm=comm) as tmp_dir:
        comm.Barrier()
        assert os.path.exists(tmp_dir)

    comm.Barrier()
    assert not os.path.exists(tmp_dir)


def test_nompi_temporary_directory():
    with shared_temporary_directory(prefix="foo") as tmp_dir:
        assert os.path.exists(tmp_dir)

    assert not os.path.exists(tmp_dir)
