# Tiny circuits.
#
# Needs to run either via examples with 3 MPI ranks or
# simply as part of `pytest`.

import pytest
import os

from spatial_index import open_index
from spatial_index import IndexResolver
from spatial_index.io import MetaData, shared_temporary_directory


CIRCUIT_10_DIR = "tests/data/tiny_circuits/circuit-10"
CIRCUIT_1K_DIR = "tests/data/tiny_circuits/syn-2k"


def small_synapse_sonata_conf():
    filename = os.path.join(CIRCUIT_1K_DIR, "edges.h5")
    population = "neocortex_neurons__chemical_synapse"

    return filename, population


def small_morphology_sonata_conf():
    morph_dir = os.path.join(CIRCUIT_10_DIR, "morphologies/ascii")
    filename = os.path.join(CIRCUIT_10_DIR, "nodes.h5")
    population = "All"

    return morph_dir, filename, population


def small_sonata_conf(element_type):
    if element_type == "synapse":
        return small_synapse_sonata_conf()

    elif element_type == "morphology":
        return small_morphology_sonata_conf()

    else:
        raise ValueError("Broken test case.")


def check_builder_from_sonata(element_type,
                              index_variant,
                              build_callback,
                              mpi_comm=None):
    mpi_rank = 0 if mpi_comm is None else mpi_comm.Get_rank()

    Index = IndexResolver.index_class(element_type, index_variant)

    with shared_temporary_directory(prefix="from_sonata_file", mpi_comm=mpi_comm) as d:
        index_path = os.path.join(d, element_type)
        index = build_callback(element_type, index_variant, output_dir=index_path)

        if index is not None:
            assert len(index.available_fields) > len(index.builtin_fields)

        if mpi_rank == 0:
            loaded_index = open_index(index_path)
            meta_data = MetaData(index_path)

            assert isinstance(loaded_index, Index)
            assert meta_data.extended is not None


def from_sonata_file_callback(element_type, index_variant, output_dir=None):
    args = small_sonata_conf(element_type)

    Builder = IndexResolver.builder_class(element_type, index_variant)
    return Builder.from_sonata_file(*args, output_dir=output_dir)


def from_sonata_selection_callback(element_type, index_variant, output_dir=None):
    import libsonata

    args = small_sonata_conf(element_type)
    selection = libsonata.Selection([0, 1, 2])

    Builder = IndexResolver.builder_class(element_type, index_variant)
    return Builder.from_sonata_selection(*args, selection, output_dir=output_dir)


def from_sonata_file_gids_callback(element_type, index_variant, output_dir=None):
    args = small_sonata_conf(element_type)

    Builder = IndexResolver.builder_class(element_type, index_variant)
    return Builder.from_sonata_file(*args, gids=[0, 1], output_dir=output_dir)


def from_sonata_file_target_gids_callback(element_type, index_variant, output_dir=None):
    args = small_sonata_conf(element_type)

    Builder = IndexResolver.builder_class(element_type, index_variant)
    return Builder.from_sonata_file(*args, target_gids=[0, 1], output_dir=output_dir)


def check_builder_from_sonata_file(element_type, index_variant, mpi_comm=None):
    check_builder_from_sonata(
        element_type, index_variant, from_sonata_file_callback, mpi_comm=mpi_comm
    )


def check_morphology_from_sonata(index_variant, mpi_comm=None):
    element_type = "morphology"

    check_builder_from_sonata(
        element_type, index_variant, from_sonata_selection_callback, mpi_comm=mpi_comm
    )

    check_builder_from_sonata(
        element_type, index_variant, from_sonata_file_gids_callback, mpi_comm=mpi_comm
    )

    check_builder_from_sonata(
        element_type, index_variant, from_sonata_file_target_gids_callback,
        mpi_comm=mpi_comm
    )


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
def test_synapse_in_memory_from_sonata_file():
    check_builder_from_sonata_file("synapse", "in_memory")


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
@pytest.mark.mpi(min_size=3)
def test_synapse_multi_index_from_sonata_file():
    from mpi4py import MPI

    mpi_comm = MPI.COMM_WORLD
    check_builder_from_sonata_file("synapse", "multi_index", mpi_comm=mpi_comm)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
def test_morphology_in_memory_from_sonata():
    check_morphology_from_sonata("in_memory")


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
@pytest.mark.mpi(min_size=3)
def test_morphology_multi_index_from_sonata():
    from mpi4py import MPI

    mpi_comm = MPI.COMM_WORLD
    check_morphology_from_sonata("multi_index", mpi_comm=mpi_comm)
