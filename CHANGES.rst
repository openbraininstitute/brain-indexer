Change Log
==========

Version 0.0.1
-------------

*Features*
  * Initial Spatial-Index based on boost.geometry.index.

  * | IndexTree handling both generic geometries and boost variants implementing the protocol:
    | - Base Geometries: Spheres and Cylinders.
    | - Extended types: IndexedSphere, Soma and Segment.
    | - Variant types: variant<Soma, Segment>

  * | Created Python API for the two possibly most useful trees:
    | - SphereIndex: IndexTree<IndexedSphere> - memory and cpu efficient.
    | - MorphIndex: IndexTree<variant<Soma, Segment>> - capable of handling entire morphologies.
