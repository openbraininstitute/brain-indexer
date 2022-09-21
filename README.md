# SpatialIndex

## What is it?

Spatial Index is a library for efficient spatial queries on large datasets (TBs
and more). It is currently based on `boost::rtree`.

It provides a high level Python API for indexes of simple geometric shapes
(boxes, spheres and cylinders) and the required functionalities to
create indexes of synapses and morphologies.

## Where to start

We included plenty of examples in the `examples` folder and there's also a
`basic_tutorial.ipynb` and an `advanced_tutorial.ipynb` Jupyter Notebook which
which provide a hands-on introduction to SI.

There is also a [User Guide][1], which provides detailed descriptions of all
concepts and includes an API documentation.

[1]: https://bbpteam.epfl.ch/documentation/projects/spatial-index/latest/index.html

## How to install

Please see [Installation][2].

[2]: https://bbpteam.epfl.ch/documentation/projects/spatial-index/latest/installation.html


## Examples

Some examples on how to use Spatial Index are available in the `examples` folder:
- `segment_index.py`: simple indexing and querying of a segment index
- `synapses_index.py`: simple indexing and querying of a synapse index
- `segment_index_sonata.py`: indexing and querying of a segment index using SONATA files
- `segment_multi_index_sonata.py`: indexing and querying of a segment multi-index using SONATA files
- `synapse_multi_index_sonata.py`: indexing and querying of a synapse multi-index using SONATA files

Also, the `tests` folder contains some tests that double also as examples on how to use Spatial Index.
