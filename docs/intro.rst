Introduction
============

If you prefer a hands-on approach you might like to start with the Jupyter
Notebook ``basic_tutorial.ipynb`` or any of the examples in ``examples/``.


Using Existing Indexes
----------------------

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

    results = index.window_query(min_points, max_points)
    results = index.vicinity_query(center, radius)

The former returns all elements that intersect with the box defined by the
corners ``min_points`` and ``max_points``. The latter is used when the query
shape is a sphere. The detailed documentation of :ref:`queries <Queries>` contains several
examples.


Creating Index on the Fly
-------------------------
Work loads the required repeated queries will benefit from using a spatial
index, even for quite small number of indexed elements. Therefore, it is useful
to create small indexes on the fly. This section describes the available API for
this task.

If you're trying to pre-compute an index for later use, you might prefer the :ref`CLI
applications <CLI Interface>`. 


Indexing Nodes
~~~~~~~~~~~~~~

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

Indexing Morphologies
~~~~~~~~~~~~~~~~~~~~~
In SI the term *morphologies* refers to discrete neurons consisting of somas and segments.

Morphology indexes can be build directly from mvd3 and SONATA input files. For example by using
`MorphIndexBuilder` as follows:

.. code-block:: python

    index = MorphIndexBuilder.from_sonata_file(morph_dir, nodes_h5)
    index = MorphIndexBuilder.from_mvd_file(morph_dir, nodes_mvd)

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
attributes of the synapse stored in the SONATA file. Please see :ref:`Queries`_ for
more information about how to perform queries.

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
SpatialIndex implements two means for indexing large circuits:

* memory mapped files,
* multi indexes.

Memory mapped files are a seamless extension of regular in-memory indexes.
However, after running out of memory the hard-drive is used a backup RAM. This
works well when combined with fast storage media such as NVME SSDs; and
probably to a lesser extent regular SSDs and hard-drives. It definitely isn't
performant when memory mapping file on GPFS. Please read the detailed
:ref:`documentation <Memory Mapped Files>`.

Multi indexes subdivide the volume to be indexed into small subvolumes and uses
MPI to create subindexes for each of these subvolumes. More information can be
found :ref:`here <Multi Index>`.
