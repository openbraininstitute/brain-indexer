# SPATIAL INDEX

## What is it?

Spatial Index is a library for efficient Spatial queries over large sets of geometries.

It provides a high level Python API for indexes of (1) low-level spherical geometries and (2) spherical and cylindrical geometries adapted to neuron morphologies.

## How to install

It is advisable to install Spatial Index in a Python virtualenv.

You use `pip install --index https://bbpteam.epfl.ch/repository/devpi/simple spatial_index` to install Spatial Index after you created and activated your virtualenv.

Alternatively, to create a Virtual Environment and install Spatial Index, a script is supplied that performs all the operations necessary to create a Virtual Environment and install Spatial Index. The script is available in the `scripts` folder (more info in the following section).

## How to use Spatial Index on BB5

Spatial Index is now integrated in Blue Brain Spack, therefore on BB5 you can load it as a module using the command: `module load unstable spatial-index`.

Alternatively use the `install_spatial_index.sh` script in the `scripts` folder and run it as follows:  
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
- `segment_index.py` shows how to load a set of geometries, index them and query them using the Spatial Index APIs;
- `segment_index_sonata.py` shows how to load a set of geometries, perform a selection using SONATA, index and query them using the Spatial Index APIs;
- `synapse_index.py` shows how to load synapses from a EDGE file, index them and query them using the Spatial Index APIs.  

Also, the `tests` folder contains some tests that double also as examples on how to use Spatial Index.

## Remarks

Some initial support for multithreading and very large circuits has been added. However such functionality is still
very premature and unsuitable for production jobs. If you are interested please inspect index_grid.hpp.
