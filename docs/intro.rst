Introduction
============

A spatial index is a data structure that's designed to quickly find all
elements that intersect with a given query shape. The word *index* is used
in the database sense; not as an integer identifier.

Here quick means, that we do not need to look through all element to figure out
which elements intersect with the query shape. In SpatialIndex we use an
implementation of an R-tree, see `boost::rtree`_. The idea behind an R-tree is that the
leaves of the tree contain the bounding boxes of the elements; and internal
nodes store the bounding box of their descendants. This structure is depicted
in :numref:`index`.

.. _boost::rtree: https://www.boost.org/doc/libs/1_80_0/libs/geometry/doc/html/geometry/spatial_indexes.html

.. _index:
.. figure:: img/index.png
   :scale: 20 %

   The gray non-axis aligned boxes are represent the elements in the tree. The
   gray outlines represent the bounding box of internal nodes.

Given such a tree, when performing the query one only needs to descend into
subtrees, if they query shape intersects with the bounding box of the subtree.
By design this piece of information is stored in the root of the subtree. This
is show in  :numref:`query`.

.. _query:
.. figure:: img/query.png
   :scale: 20 %

   The yellow box is the query shape. The elements found by this query are
   shown in green, any other elements are drawn in gray. The outlines show
   which parts of the tree need to be considered when performing a query. In
   the first level the entire left side is excluded, then the lower half. In
   the third level, both subtrees need to be looked at.

The trick is to create the tree such that the bounding boxes of internal nodes
don't overlap too much or needlessly.

One typical work flow for indexes is to create them once up front, store them
and open them whenever one needs to perform spatial queries. Naturally, there
are other workflows which will also be covered, but for now it's enough to know
that the cost of building the index is often less than the naive approach. Even
for few elements and not very many queries, say thousands.

Because it's common (at BBP) that someone has precomputed the index for you, we
start explaining the syntax of queries.


Using Existing Indexes
----------------------
If you prefer a hands-on approach you might like to continue with the Jupyter
Notebook ``basic_tutorial.ipynb`` or any of the examples in ``examples/``.

Given an index stored at ``index_path``, one may want to open the index and
perform queries.

Opening An Existing Index
~~~~~~~~~~~~~~~~~~~~~~~~~

Indexes are usually stored in their own folder. This folder contains a file called ``meta_data.json``.
In order to open an index one may simply call

.. code-block:: python

    index = spatial_index.open_index(path_to_index)

where ``path_to_index`` is the path to the folder containing the index. This can be used for all
variants of indexes.

Performing Simple Queries
~~~~~~~~~~~~~~~~~~~~~~~~~

After opening the index one may query it as follows:

.. code-block:: python

    results = index.box_query(min_points, max_points)
    results = index.sphere_query(center, radius)

The former returns all elements that intersect with the box defined by the
corners ``min_points`` and ``max_points``. The latter is used when the query
shape is a sphere. The detailed documentation of :ref:`queries <Queries>` contains several
examples.


Creating An Index on the Fly
----------------------------
Workflows that require repeated queries will benefit from using a spatial
index, even for quite a small number of indexed elements. Therefore, it is useful
to create small indexes on the fly. This section describes the available API for
this task.

If you're trying to pre-compute an index for later use, you might prefer the
:ref:`CLI applications <CLI Interface>`. 


Indexing Nodes
~~~~~~~~~~~~~~

A common case is to create a spatial index of spheres which have an id (e.g. somas identified by their gid).
SphereIndex is, in this case, the most appropriate class.

The constructor accepts all the components (gids, points, and radii) as individual numpy arrays.

.. code-block:: python

    from spatial_index import SphereIndexBuilder
    import numpy as np
    ids = np.arange(3, dtype=np.intp)
    centroids = np.array([[0, 0, 0], [1, 0, 0], [2, 0, 0]], dtype=np.float32)
    radius = np.ones(3, dtype=np.float32)
    index = SphereIndexBuilder.from_numpy(centroids, radius, ids)

Indexing Morphologies
~~~~~~~~~~~~~~~~~~~~~
In SI the term *morphologies* refers to discrete neurons consisting of somas and segments.

Morphology indexes can be build directly from SONATA input files. For example by using
`MorphIndexBuilder` as follows:

.. code-block:: python

    index = MorphIndexBuilder.from_sonata_file(morph_dir, nodes_h5)

where ``morph_dir`` is the path of the directory containing the morphologies in
either ASCII, SWC or HDF5 format. Both function have a keyword argument which
allows one to optionally specify the GIDs of all neurons to be indexed.

By passing the keyword argument ``output_dir`` the index is stored at the
specified location and can be opened/reused at later point in time.

Indexing Synapses
~~~~~~~~~~~~~~~~~

Another common example is to create a spatial index of synapses imported from a sonata file.
In this case ``SynapseIndexBuilder`` is the appropriate class to use:

.. code-block:: python

    from spatial_index import SynapseIndexBuilder
    from libsonata import Selection
    index = SynapseIndexBuilder.from_sonata_file(EDGE_FILE, "All")

Building a synapse index through this API enables queries to fetch any
attributes of the synapse stored in the SONATA file. Please see
:ref:`Queries` for more information about how to perform queries.

Passing the keyword argument ``output_dir`` ensures that the index is also
stored to disk.

Precomputing Indexes For Later Use
----------------------------------
When the number of indexed elements is large, considerable resources are needed
to compute the index. Therefore, it can make sense to precompute the index once
and store it for later (frequent) reuse. The most conventient way is through the
CLI applications. Note that indexes can exceed the amount of available RAM, in
this case please consult `Large Indexes`_.

.. _`CLI Interface`:

Command Line Interface
~~~~~~~~~~~~~~~~~~~~~~

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


Large Indexes
~~~~~~~~~~~~~
SpatialIndex implements Multi-Indexing for indexing large circuits.

Multi indexes subdivide the volume to be indexed into small subvolumes and uses
MPI to create subindexes for each of these subvolumes. More information can be
found :ref:`here <Multi Index>`.
