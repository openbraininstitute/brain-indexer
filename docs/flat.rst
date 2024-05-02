Comparison to FLAT
==================

The page summarizes the differences to FLAT we are aware of.

Intersection Criteria
---------------------

Throughout this subsection we assume that the query shape is a box. In FLAT an
element intersects with the query shape if a bounding box of the element
intersects with the query shape. This is almost the same as using
``accuracy="bounding_box"`` in brain-indexer. However, in FLAT, the bounding
box of cylinders is not the minimal bounding box but rather

.. code-block:: python

    p1, p2 = cylinder.endpoints
    r = cylinder.radius

    min_corner = np.minimum(p1 - r, p2 - r)
    max_corner = np.maximum(p1 + r, p2 + r)

This isn't accurate. If the cylinder is axis aligned, say with the x-axis. Then,
then for the minimum bounding box:

.. code-block:: python

    assert min_corner[0] == min(p1[0], p2[0])

which is strictly larger by an additive constant ``r`` than the value FLAT uses.
brain-indexer always computes the exact minimum bounding box of the cylinder, taking into
account that the orientation of the cylinder.


Section IDs
-----------

Section IDs in FLAT and brain-indexer can differ. This seems to be due to FLAT using the
BBP SDK which handles uniforcations differently than brain-indexer. There might also be
differences due to the ordering of the branches.
