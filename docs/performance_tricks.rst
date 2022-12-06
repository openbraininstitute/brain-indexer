Performance Tricks
==================

This page contains practical tricks for improving performance. Certain usage
patterns can be prohibitively bad, so please browse the examples.

Keep The Index Alive
--------------------

It's important to avoid loading the index from disk more often than needed.
Consider the following example

.. code-block:: python

   import spatial_index

   query_boxes = make_query_boxes(n_queries)
   for box in query_boxes:
       index = spatial_index.open_index(index_path)
       results = index.box_query(*box)


This leads to the index being loaded from disk for every single query. The
important observation is that one query is typically is many order of magnitude
faster than loading an index, or part of an index from disk. Even for
moderately large number of queries, the above is a serious performance bug. If
ever possible restructure as follows:

.. code-block:: python

    import spatial_index

    index = spatial_index.open_index(index_path)

    query_boxes = make_query_boxes(n_queries)
    for box in query_boxes:
        results = index.box_query(*query_boxes)


Multi-Index: Cache-Friendliness
-------------------------------

For multi-indexes it's important to remember that they keep parts of the tree
in memory. For large circuits storing the entire index in memory would required
on the order of TBs of RAM. Hence, SpatialIndex isn't able to keep the entire
index in memory. Instead multi-indexes have a cache of a configurable size,
once the cache is full, a part of the tree needs to be evicted. If that part is
used again during a later query it'll need to be loaded from the filesystem
(often GPFS) again.

Notice that the order in which queries are performed effects how much eviction
happens. If one were able to sort the queries such that successive queries
are close to each other, then intuitively this should improve cache reuse.

To diagnose if a particular application is suffering from this issue, please
use the environment variable to activate dumping cache reuse statistics, see
:ref:`Environment Variables`.

If you're seeing high eviction numbers, e.g. if the number of unique sub-trees
loaded is small compared to the number of evictions, then you might benefit
from reordering the queries.

SpatialIndex implements functionality to order the queries, based on
space-filling curves. This can be used as follows:

.. code-block:: python

    import spatial_index
    import spatial_index.experimental

    index = spatial_index.open_index(index_path)

    query_boxes = make_query_boxes(n_queries)
    centroids = compute_centroids(query_boxes)

    query_order = spatial_index.experimental.space_filling_order(centroids)
    query_boxes = [query_boxes[i] for i in query_order]

    for box in query_boxes:
        index.box_query(*box)
