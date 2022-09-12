# Tiny circuits.
#
# Needs to run either via examples with 3 MPI ranks or
# simply as part of `pytest`.

import pytest
import os
import tempfile

from spatial_index import open_index
from spatial_index import IndexResolver


CIRCUIT_10_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit-10"
CIRCUIT_1K_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit-1k"


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


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR), reason="Missing data file.")
@pytest.mark.parametrize("element_type", ["synapse", "morphology"])
def test_morphology_in_memory_from_sonata_file(element_type):
    # This test also exercises:
    #    - from_sonata_tgids
    #    - from_sonata_selection

    args = small_sonata_conf(element_type)
    index_variant = "in_memory"

    Builder = IndexResolver.builder_class(element_type, index_variant)

    index = Builder.from_sonata_file(*args)
    assert isinstance(index, IndexResolver.index_class(element_type, index_variant))

    with tempfile.TemporaryDirectory(prefix="from_sonata_file") as d:
        index_path = os.path.join(d, element_type)

        Builder.from_sonata_file(*args, output_dir=index_path)
        loaded_index = open_index(index_path)

        assert isinstance(loaded_index, type(index))
