# SPATIAL INDEX

## What is it?

Spatial Index is a library for efficient Spatial queries over large sets of geometries.

It provides a high level Python API for indexes of (1) low-level spherical geometries and (2) spherical and cylindrical geometries adapted to neuron morphologies.

## How to install

It is advisable to install Spatial Index in a Python virtualenv.
In order to install Spatial Index on BB5, a script is supplied that performs all the operations necessary to install a Virtual Environment. The script is available in the `scripts` folder.
Soon we will be adding it to an online python index, for `pip install`, please stay tuned.

## How to use the install script on BB5

Download the `install_spatial_index.sh` script and run it as follows:  
`./install_spatial_index.sh <name of the virtualenv>`  
  
(If there's already a virtualenv with the same name, the creation of a new one will be skipped and Spatial Index will be installed in the existing one.)   
  
Activate the virtualenv using:  
`source <name of the virtualenv>/bin/activate`

## How to install from source

Clone the repository:  
`git clone git@bbpgitlab.epfl.ch:hpc/SpatialIndex.git && cd SpatialIndex`

Create the virtualenv:  
`python -m venv <name of the virtualenv>`

Install using pip:  
`pip install -e .`

## Examples

Some examples on how to use Spatial Index are available in the `examples` folder:   
- `example_query.py` shows how to load a set of geometries, index them and query them using the Spatial Index APIs;
- `example_synapses.py` shows how to load synapses from a EDGE file, index them and query them using the Spatial Index APIs.  

Also, the `tests` folder contains some tests that double also as examples on how to use Spatial Index.

## Remarks

Some initial support for multithreading and very large circuits has been added. However such functionality is still
very premature and unsuitable for production jobs. If you are interested please inspect index_grid.hpp.
