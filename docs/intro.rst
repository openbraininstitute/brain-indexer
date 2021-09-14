Introduction
============

Design
------

Spatial Index was designed with efficiency in mind. It was developed on top of the Boost Geometry.Index libraries, renown for their excellent performance.

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

You can also build the segment index using `NodeMorphIndexer` specifying the morphology and circuit file to load and then starting the processing via the `process_range` method.

.. code-block:: python

    from spatial_index import node_indexer
    indexer = node_indexer.NodeMorphIndexer(MORPH_FILE, CIRCUIT_FILE)
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
In this case SynapseIndexer is the appropriate class to use:

.. code-block:: python

    from spatial_index import SynapseIndexer
    from libsonata import Selection
    index = SynapseIndexer.from_sonata_file(EDGE_FILE, "All")

Then one can query the synapses index by getting the gids first and then querying the edge file for the synapse data.
Keep in mind that the resulting objects only have two properties: gid and centroid.

.. code-block:: python

    points_in_region = index.find_intersecting_window([200, 200, 480], [300, 300, 520])
    z_coords = index.edges.get_attribute("afferent_center_z", Selection(points_in_region))

Otherwise one can query directly from the index:

.. code-block:: python

    objs_in_region = index.find_intersecting_window_objs([200, 200, 480], [300, 300, 520])

And then fetching the necessary information directly from the structure you just created.

More examples
--------------

In the `_examples` folder there are some more examples on how to use Spatial Index. Please check them out.
Also some interesting snippets on how to use a specific function can be found in the various python files found in the `tests` folder.
