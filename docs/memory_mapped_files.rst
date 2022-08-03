.. _Memory Mapped Files:

Memory mapped files (experimental)
==================================

**Keep in mind that the "memory mapped file" is still an experimental feature and therefore could lead to unexpected/untested results.
Plese contact the HPC team or the developers of SpatialIndex if you have any questions.**

SpatialIndex can also use memory mapped files to index very large circuits without the risk of triggering Out Of Memory errors.
Memory mapped files are available for both segment and synapse indexing.
Please use a fast filesystems, preferably on SSD/NVME, and avoid slow distributed filesystems like GPFS that would make the process extremely slow.
On BB5 you can request for a NVME node using:

.. code-block:: bash

    salloc -p prod -C nvme ...

Memory mapped files for segment indexing
----------------------------------------

In order to use memory mapped files for segment indexing, you need to use the `--use-mem-map=SIZE_OF_MAPPED_FILE` parameter in the `spatial-index-nodes` CLI utility:

.. code-block:: bash

    spatial-index-nodes --use-mem-map=SIZE_IN_MB NODES_FILE MORPHOLOGY -o OUTPUT_FILE
    spatial-index-circuit segments --use-mem-map=SIZE_IN_MB SONATA_CIRCUIT_CONFIG -o OUTPUT_FILE

One can also use the normal :class:`spatial_index.MorphIndexBuilder` Python class to generate a memory mapped index in a program/script. Please refer to the specific class :class:`spatial_index.MorphIndexBuilder` documentation for more details.

You can find some examples on how to use the memory mapped indexer in the `examples/` directory, specifically in the `memory_map_index.py` file and `memory_map_index.sh` files.

Memory mapped files for synapse indexing
----------------------------------------

To use memory mapped files for synapse indexing you can use the `--use-mem-map=SIZE_OF_MAPPED_FILE` parameter in the `spatial-index-synapses` CLI utility:

.. code-block:: bash

    spatial-index-synapses --use-mem-map=SIZE_IN_MB EDGES_FILE POPULATION -o OUTPUT_FILE
    spatial-index-circuit synapses --use-mem-map=SIZE_IN_MB SONATA_CIRCUIT_CONFIG --populations POPULATIONS,... -o OUTPUT_FILE

Otherwise you can also use the normal :class:`spatial_index.SynapseIndexBuilder` Python class to generate a memory mapped index in a program/script. Please refer to the specific class :class:`spatial_index.SynapseIndexBuilder` documentation for more details.

"""

