# SPATIAL INDEX

## What is it?

Spatial Index is a library for efficient Spatial queries over large sets of geometries.

It provides a high level Python API for indexes of (1) low-level spherical geometries and (2) spherical and cylindrical geometries adapted to neuron morphologies.

Spatial Index allows the user to create huge indexes via multi-indexes and is extremely flexible in that regard.

## Where to start

We included plenty of examples in the `examples` folder and there's also a `basic_tutorial.ipynb` and an `advanced_tutorial.ipynb` Jupyter Notebook which are the **recommended** way to get started with this project.

## How to install

It is advisable to install Spatial Index in a Python virtualenv.

You can use `pip install --index https://bbpteam.epfl.ch/repository/devpi/simple spatial_index` to install Spatial Index after you created and activated your virtualenv.

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

    git clone --recursive git@bbpgitlab.epfl.ch:hpc/spatial-index.git

This is the command in case you forgot the `--recursive`:

    git submodule update --init --recursive

Remember to `cd` into the correct directory and create the virtualenv
and install as editable with pip

    python -m venv <name of the virtualenv>
    pip install -e .

## Examples

Some examples on how to use Spatial Index are available in the `examples` folder:   
- `segment_index.py`: simple indexing and querying of a segment index 
- `synapses_index.py`: simple indexing and querying of a synapse index
- `segment_index_sonata.py`: indexing and querying of a segment index using SONATA files
- `segment_multi_index_sonata.py`: indexing and querying of a segment multi-index using SONATA files
- `synapse_multi_index_sonata.py`: indexing and querying of a synapse multi-index using SONATA files

Also, the `tests` folder contains some tests that double also as examples on how to use Spatial Index.
