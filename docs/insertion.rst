.. _`Insert`:

Inserting Elements Into an Index
================================

The spatial indexes used in brain-indexer are R-trees. In principle R-trees allow
inserting elements one-by-one. However, the efficiency of queries depends on the
internal structure of the R-tree. If knowledge about all points is known
upfront, the structure of the R-tree can be optimized better than when each
element is inserted individually. Therefore, we strongly recommend to use bulk
creation, e.g. via the CLI, whenever possible. Only when the application
genuinely requires adding elements one-by-one, e.g. because the location of the
next element requires querying the index with current elements, should the API
in this page be used.

Limitations
-----------

Given that this usecase is rare at BBP. The features have only been implemented
partially, when needed by particular applications.

Currently, only the in-memory `SphereIndex` supports adding spheres.

SphereIndex
-----------

Adding a sphere with radius `radius`, centroid `centroid` and ID `id`, can be
done as follows:

.. code-block: Python

    index.insert(centroid, radius, id)

When inserting ``N`` spheres, one can simply pass a numpy array of radii
(``shape == (N,)``), centroids (``shape == (N, 3)``) and IDs (``shape == (N,)``)
instead.

Note, this is not the same as bulk insertion, i.e. internally the elements are
still added one-by-one.
