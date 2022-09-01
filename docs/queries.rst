.. _`Queries`:

Queries
=======
The strength of a spatial index is that one can query it to get all elements
within a certain query shape, quickly. Here "quickly" means without looping
over all elements in the index.


Query Shapes
------------
A query returns all indexed elements that intersect with the *query shape* in
the requested geometry mode. The query shape can be either an axis-aligned box
in which case the query is called a *window query*; a sphere which we call a
*vicinity query*; or a cylinder (mostly for internal purposes such as placing
segments).

Indexed Elements
----------------
SI supports indexes containing points, boxes, spheres and cylinders. From these
we can build indexes for

* *morphologies* which refers a discretization of the morphology in terms of a
  sphere for the soma and cylinders for the segments of the axons or dendrites,

* *synapses* which are treated as points.


Regular Queries
---------------
Regular queries are queries which return attributes of the indexed elements
that intersect with a given query shape. Please see `Counting Queries`_ only
the number of index elements is needed.

Keyword argument: ``fields``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The preferred output format of queries are numpy arrays containing the values
of interest. Which attributes are returned is controlled by a keyword argument
``fields``. If fields is a string then a ``list`` or numpy array is returned.
To allow retrieving multiple attributes in a single query, a (non-string)
iterable can be passed to ``fields``. In this case a dictionary of the
retrieved attributes is returned.

Currently, some attributes are fully supported and other we will call partially
supported. The difference is that any combination of fully supported fields is
valid. While partially supported fields may not appear together with other
fields (fully or partially supported).

Morphology Indexes
^^^^^^^^^^^^^^^^^^
For morphology indexes the fully supported fields are:

* ``"gid"`` which is the GID of the neuron,
* ``"section_id"`` which is the section ID of the neuron,
* ``"segement_id"`` which is the segment ID of the neuron,
* ``endpoint1`` which for somas is the center of the sphere, for segments its
  the center of one of the caps of the cylinder,
* ``endpoint2`` which for somas is ``nan``, for segments its
  the center of one of the other cap of the cylinder,
* ``radius`` which is the radius of either the sphere or cylinder.
* ``kind`` an integer that can be compared against ``int(EntryKind.soma)``
  to know if one is dealing with a soma or a segment.

Partially supported fields:

* ``"position"`` the center of the sphere or cylinder.
* ``"ids"`` the three ids packed into a single struct.
* ``"raw_elements"`` in rare cases one may be interested in a list
  of Python objects, i.e., either ``core.Soma`` or ``core.Segment``.


Examples
++++++++

.. code-block:: python

    >>> index = spatial_index.open(morph_index_path)

    >>> index.window_query(*window, fields="gid")
    np.array([12, 3, 32, ...], np.int64)

    >>> index.window_query(*window, fields=["gid"])
    {
      "gid": np.array([12, 3, 32, ...], np.int64)
    }

    >>> index.window_query(*window, fields=["gid", "radius"])
    {
      "gid": np.array([...], ...),
      "radius": np.array([...], ...)
    }

    >>> index.window_query(*window)
    {
      "gid": ...,
      "section_id": ...,
      ...
      "kind": ...
    }


Synapse Indexes
^^^^^^^^^^^^^^^
For synapse indexes the fully supported fields are:

* ``"id"`` which is the ID of the synapse,
* ``"post_gid"`` which is the GID of the post-synaptic neuron,
* ``"pre_gid"`` which is the GID of the pre-synaptic neuron,
* ``centroid`` which is the coordinates of the synapse.

Partially supported fields:

* ``"position"`` the center of the sphere or cylinder.
* ``"ids"`` the three ids packed into a single struct.
* ``"raw_elements"`` in rare cases one may be interested in a list
  of Python objects, i.e., either ``core.Soma`` or ``core.Segment``.


SONATA Fields
^^^^^^^^^^^^^
Synapse indexes created from SONATA input files, can be queried for attributes
stored in the input file. This is accomplishes passing the SONATA name of the
attribute to ``fields``. SONATA fields can be combined with any other fully
supported field.

As an example the section and segment id on the pre- and post-synapse can be
obtained as follows:

.. code-block: python
   >>> index.window_query(
           *window,
           fields=[
               "id",
               "pre_gid", "post_gid",
               "afferent_section_id", "afferent_segment_id",
               "efferent_section_id", "efferent_segment_id",
           ]
       )
   {
     "id": ...,
     ...
     "efferent_segment_id": ...
   }


.. _`kw-accuracy`:

Keyword argument: ``accuracy``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The query always reports all elements that intersect (as opposed
to contained in) with the query shape. However, it is not always possible to
decide efficiently if the element intersects exactly with the query shape. In
particular, when the indexed element is a cylinder/segment, closed formulas
rarely exist. Therefore, SI exposes a keyword argument ``accuracy`` which
controls how accurately the indexed element is treated during queries. There
are two values:

* ``best_effort``  As the name indicates exact closed formulas are used if
  available. If not the cylinder is approximated by a capsule, i.e., a
  cylinder with two half spheres on either end. For capsules efficient
  closed formulas to detect intersection always exist. The final twist is
  that in all cases there is a pre-check to see if the exact bounding boxes
  of the query shape and of the indexed element intersect.

* ``bounding_box`` The indexed elements are treated as if they were
  equal to their exact minimal bounding box. This is similar to how the FLAT
  index treated indexed elements. This is the default.

Examples
^^^^^^^^

.. code-block:: python

    >>> index = spatial_index.open_index(morph_index_path)
    >>> index.window_query(*window, accuracy="best_effort")
    {
      "gid": ...,
      ...
      "kind": ...,
    }

Counting Queries
----------------
Counting queries are queries for which only the number of index elements is
returned. If information about the individual indexed elements themselves is need, please consult `Regular Queries`_.

The API for counting queries is simple and the accuracy can be controlled in
the same way as for :ref:`regular indexes <kw-accuracy>`.

.. code-block:: python

   >>> index.window_counts(*window)
   9238

   >>> index.vicinity_counts(*sphere)
   2789

Keyword argument: ``group_by``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For synapse indexes a special mode of counting is supported. For indexes of
synapses from N source populations into a single target population, one can
group the synapses by the GID of the target neuron; and then count the number
of synapses per target GID.

This is enabled through the keyword argument ``group_by="gid"``.

.. code-block:: python

   # The keys of the dictionary are the target GIDs, and
   # the values are the number of synapses are contained in
   # `window` with the specified target GID.
   >>> index.window_counts(*window, group_by="gid")
   {
     2379: 23,
     293: 1,
     ...
   }

