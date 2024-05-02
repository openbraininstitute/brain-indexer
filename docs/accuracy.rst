Overlap Detection Details
=========================

Collision detection between cylinders and other simple geometry objects is
surprisingly difficult; and closed formulas seem to not always exist. At the
same time, using expensive solvers is not ideal in brain-indexer.

This page explains, in detail, the different overlap detection methods and the
approximations they make.

Bounding Box Overlap Detection Details
--------------------------------------

The semantically most simple approximation is to treat all indexed elements as
if they were equal to their bounding box. Depending of the relative size of the
query shape and the indexed elements, this approximation might be quite
accurate, e.g., if the box is large compared to the largest element in the index,
then just treating the indexed elements as boxes might be sufficient, see
:numref:`boxsphere` for an illustration.

.. _boxsphere:
.. figure:: img/accuracy/bounding_box/box_sphere.png
   :width: 100 %

   The yellow shape denotes the query shape. All selected elements are shown in
   green, any other element in gray. The bounding box of elements is show in
   light blue.


Best-Effort Overlap Detection Details
-------------------------------------

When using the ``"best_effort"`` overlap detection methods, we occasionally
approximate a cylinder by a capsule. The name alludes to the fact that it
attempts to approximate the true geometric collision detection, using closed
formulas. Note, that ``"best_effort"`` is at least as accurate as
``"bounding_box"`` and is also more accurate than consistently treating
cylinders as if they were capsules.

Sphere/Cylinder overlap
^^^^^^^^^^^^^^^^^^^^^^^

This can be decided exactly with the following idea. If the projection of the
center of the sphere onto the axis of the cylinder lies between the two
endpoints, then check the distance of the center of the sphere to this point.
If not compute the point on the caps that is closest to the center of the
sphere; and check the distance to the center of the sphere.

Box/Cylinder overlap
^^^^^^^^^^^^^^^^^^^^

No exact formulas seem to exist for computing if a box and a cylinder overlap.
Hence, we approximate this collision detection by the collision of a box with a
capsule. However, collision detection is performed in two steps:

1. Compute if the exact minimum bounding box of the cylinder overlaps with the
   query box.
2. If they do, then compute if the query box collides with the capsule.

Please consult :numref:`boxcylinder` for a depiction of this two stage process.

.. _boxcylinder:
.. figure:: img/accuracy/best_effort/box-cap.png
   :width: 50 %

   The yellow shape denotes the query shape. All selected elements are shown in
   green, any other element in gray. The bounding box of elements is show in
   light blue.

Cylinder/Cylinder overlap
^^^^^^^^^^^^^^^^^^^^^^^^^

Again no exact formulas exist and we must resort to approximating both cylinders
by capsules. Again we use a two stage process:

1. Compute if the exact minimal bounding boxes of the two cylinders overlap.
2. If they do, check if the capsules overlap.

We depicted special cases in :numref:`cylindercylinder`.

.. _cylindercylinder:
.. figure:: img/accuracy/best_effort/cap-cap.png
   :width: 75 %

   The yellow shape denotes the query shape. All selected elements are shown in
   green, any other element in gray. The bounding box of elements is show in
   light blue.

