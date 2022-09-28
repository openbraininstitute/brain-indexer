# Tiny circuits.
#
# Needs to run either via examples with 3 MPI ranks or
# simply as part of `pytest`.

import pytest
import os

from spatial_index import open_index
from spatial_index import IndexResolver
from spatial_index.io import MetaData, shared_temporary_directory


CIRCUIT_10_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v3/circuit-10"
CIRCUIT_1K_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v3/circuit-1k"


def small_synpase_sonata_conf():
    filename = os.path.join(CIRCUIT_1K_DIR, "edges.h5")
    population = "All"

    return filename, population


def small_morphology_sonata_conf():
    morph_dir = os.path.join(CIRCUIT_10_DIR, "morphologies/ascii")
    filename = os.path.join(CIRCUIT_10_DIR, "nodes.h5")
    population = "All"

    return morph_dir, filename, population


def small_sonata_conf(element_type):
    if element_type == "synapse":
        return small_synpase_sonata_conf()

    elif element_type == "morphology":
        return small_morphology_sonata_conf()

    else:
        raise ValueError("Broken test case.")


def check_morphology_from_sonata_file(element_type, index_variant, mpi_comm=None):
    mpi_rank = 0 if mpi_comm is None else mpi_comm.Get_rank()

    args = small_sonata_conf(element_type)
    Index = IndexResolver.index_class(element_type, index_variant)
    Builder = IndexResolver.builder_class(element_type, index_variant)

    with shared_temporary_directory(prefix="from_sonata_file", mpi_comm=mpi_comm) as d:
        index_path = os.path.join(d, element_type)
        Builder.from_sonata_file(*args, output_dir=index_path)

        if mpi_rank == 0:
            loaded_index = open_index(index_path)
            meta_data = MetaData(index_path)

            print(meta_data._raw_meta_data)

            assert isinstance(loaded_index, Index)
            assert meta_data.extended is not None


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
@pytest.mark.parametrize("element_type", ["synapse"])
def test_morphology_in_memory_from_sonata_file(element_type):
    check_morphology_from_sonata_file(element_type, "in_memory")


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
@pytest.mark.mpi(min_size=3)
@pytest.mark.parametrize("element_type", ["synapse"])
def test_morphology_multi_index_from_sonata_file(element_type):
    from mpi4py import MPI

    mpi_comm = MPI.COMM_WORLD
    check_morphology_from_sonata_file(element_type, "multi_index", mpi_comm=mpi_comm)
