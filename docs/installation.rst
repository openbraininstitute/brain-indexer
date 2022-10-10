How to install
==============

It is advisable to install SpatialIndex in a Python virtualenv.

You can use ``pip install --index
https://bbpteam.epfl.ch/repository/devpi/simple spatial_index`` to install
SpatialIndex after you created and activated your virtualenv.

Please note that
this method of installation will deactivate any MPI capabilities and therefore
it's necessary to use another installation method if you need to index
large circuits with multi-index support.

Alternatively, to create a Virtual Environment and install SpatialIndex, a
script is supplied that performs all the operations necessary to create a
Virtual Environment and install SpatialIndex. The script is available in the
``scripts`` folder (more info in the following section).

How to use SpatialIndex on BB5
-------------------------------

SpatialIndex is now integrated in Blue Brain Spack, therefore on BB5 you can
load it as a module using the command: ``module load unstable spatial-index``.

Alternatively use the ``install_spatial_index.sh`` script in the ``scripts``
folder and run it as follows:

.. code-block:: bash

    ./install_spatial_index.sh <name of the virtualenv>

(If there's already a virtualenv with the same name, the creation of a new one
will be skipped and SpatialIndex will be installed in the existing one.)

Activate the virtualenv using:

.. code-block:: bash

    source <name of the virtualenv>/bin/activate

How to install from source
---------------------------

Clone the repository:

.. code-block:: bash

    git clone --recursive git@bbpgitlab.epfl.ch:hpc/spatial-index.git

This is the command in case you forgot the ``--recursive``:

.. code-block:: bash

    git submodule update --init --recursive

Remember to ``cd`` into the correct directory and create the virtualenv
and install as editable with pip

.. code-block:: bash

    python -m venv <name of the virtualenv>
    pip install -e .

**Please keep in mind that some dependecies are necessary.** 

On BB5 you can simply load the following modules:

.. code-block:: bash

    module load unstable gcc cmake boost hpe-mpi

On other systems you might need to install them manually if you don't have already `cmake`, `boost`, `gcc` and `mpi` installed.
