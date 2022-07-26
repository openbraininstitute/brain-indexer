Introduction
============

Design
------

SpatialIndex allows to create indexes and perform volumetric queries with efficiency in mind. It was developed on top of the Boost Geometry.Index libraries, renown for their excellent performance.

SpatialIndex specializes to several core geometries, namely spheres and cylinders, and derivates: indexed spheres/segments and "unions" of them (variant). These objects are directly stored in the tree structure and have respective bounding boxes created on the fly. As such some CPU cycles were traded for substantial memory savings, aiming at near-billion indexed geometries per cluster node.


Public API: C++
---------------

The core library is implemented as a C++14 Header-Only libray and offers a wide range of possibilities.

By including `spatial_index/index.hpp` and `spatial_index/util.hpp` the user has access to the whole API which is highly flexible due to templates. For instance the IndexTree class can be templated to any of the defined structures or new user defined geometries, providing the user with a boost index rtree object which automamtically supports serialization and higher level API functions.

Please refer to the unit tests (under `tests/cpp/unit_tests.cpp` to see examples on how to create several kinds of IndexTrees.


Public API: Python
------------------

The Python API gives access to the most common instances of 'IndexTree's, while adding higher level API and IO helpers.
The user may instantiate a Spatial Index of Morphology Pieces (both Somas and Segments) or, for maximum efficiency, Spheres alone.

As so, if it is enough for the user to create a Spatial Index of spherical geometries, like soma placement, he should rather use :class:`spatial_index.SphereIndex`. For the cases where Spatial Indexing of cylindrical or mixed geometries is required, e.g. full neuron geometries, the user shall use  :class:`spatial_index.MorphIndex`. Furthermore, the latter features higher level methods to help in the load of entire Neuron morphologies.

Query Semantics
---------------
Every query consists of a *query shape* and a *geometry mode*. The query shape defines the region for which elements in the index are considered (usually returned, sometimes counted, depending on the query). The geometry mode defines precisely when an element should be returned.

A query shape is said to overlap with an indexed shape in *bounding box geometry* if the bounding box of the indexed shape intersects with the query shape. Note: this is the semantics adopted by FLAT.

A query shape is said to overlap with an indexed shape in *exact geometry* if first the bounding box of the indexed shape intersects with the query shape; and additionally intersects with the query shape either exactly or in the case of Box/Cylinder and Cylinder/Cylinder as Box/Capsule and Capsule/Capsule respectively.

A query returns all indexed elements that overlap with the query shape in the requested geometry mode. The query shape can be either an axis-aligned box in which case the query is called a *window query*; a sphere which we call a *vicinity query*; or a cylinder (mostly for internal purposes such as placing segments).


Indexing BBP Node and Edge files
================================

Indexing Nodes
--------------

A common case is to create a spatial index of spheres which have an id (e.g. somas identified by their gid).
SphereIndex is, in this case, the most appropriate class.

The constructor accepts all the components (gids, points, and radii) as individual numpy arrays.

.. code-block:: python

    from spatial_index import SphereIndex
    import numpy as np
    ids = np.arange(3, dtype=np.intp)
    centroids = np.array([[0, 0, 0], [1, 0, 0], [2, 0, 0]], dtype=np.float32)
    radius = np.ones(3, dtype=np.float32)
    index = SphereIndex(centroids, radius, ids)

You can also build the segment index using `MorphIndexBuilder` specifying the morphology and circuit file to load and then starting the processing via the `process_range` method.

.. code-block:: python

    from spatial_index import MorphIndexBuilder
    indexer = MorphIndexBuilder(MORPH_FILE, CIRCUIT_FILE)
    indexer.process_range(()) ## process all nodes

**Note:** *By using a constructor which initializes the index to some data, internally the "pack" algorithm is used. This results in optimal tree balance and faster queries.*

Once the index is constructed one can make queries, e.g.:

    >>> index.find_nearest(2.2, 0, 0, 1)
    array([2], dtype=uint64)

Or even save it for later reuse:

    >>> index.dump("myindex.spi")
    >>> del index
    >>> other_index = SphereIndex("myindex.spi")
    >>> other_index.find_nearest(2.2, 0, 0, 1)
    array([2], dtype=uint64)

Indexing Edge files
-------------------

Another common example is to create a spatial index of synapses imported from a sonata file.
In this case SynapseIndexBuilder is the appropriate class to use:

.. code-block:: python

    from spatial_index import SynapseIndexBuilder
    from libsonata import Selection
    indexer = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, "All", return_indexer=True)

Then one can query the synapses index by getting the gids first and then querying the edge file for the synapse data.
Keep in mind that the resulting objects only have two properties: gid and centroid.

.. code-block:: python

    points_in_region = indexer.index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    z_coords = indexer.edges.get_attribute("afferent_center_z", Selection(points_in_region))

Otherwise one can query directly from the index:

.. code-block:: python

    objs_in_region = indexer.index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])

And then fetching the necessary information directly from the structure you just created.

Command Line Interface
======================

There are three executables

* ``spatial-index-circuit`` is convenient for indexing both segments and synpses
  when the circuit is defined in a SONATA circuit configuration file. Therefore,
  if you already have a circuit config files, this is the right command to use.

    .. command-output:: spatial-index-circuit --help
  

* ``spatial-index-nodes`` is convenient for indexing segments if one wants to
  specify the paths of the input files directly.

    .. command-output:: spatial-index-nodes --help


* ``spatial-index-synapses`` like ``spatial-index-nodes`` but for synapses.

    .. command-output:: spatial-index-synapses --help



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

More examples
-------------

In the `examples` folder there are some more examples on how to use Spatial Index. Please check them out.
Also some interesting snippets on how to use a specific function can be found in the various python files found in the `tests` folder.
