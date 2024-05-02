import brain_indexer
from mpi4py import MPI


def call_some_mpi_from_python():
    rank = MPI.COMM_WORLD.Get_rank()
    print(f"mpi4py: {rank=}")


if __name__ == "__main__":
    call_some_mpi_from_python()
    brain_indexer.core.call_some_mpi_from_cxx()

    print("Seems like MPI and mpi4py are compatible.")
