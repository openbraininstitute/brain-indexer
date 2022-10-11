How to install
==============

How to use SpatialIndex on BB5
-------------------------------

SpatialIndex is now integrated in Blue Brain Spack, therefore on BB5 you can
load it as a module using the command: ``module load unstable spatial-index``.

This is the only recommended option on BB5.

How to use SpatialIndex locally
-------------------------------

Since SpatialIndex is an HPC application, the regular means of distribution
don't work well, i.e. wheels don't play nicely with MPI or other critical HPC
libraries.

Instead Spack has been invented. It works much like `pip` together with virtual
environments, just that it isn't limited to Python-only software. The advantage
of `spack` is that it aware of the requirements of HPC applications and solves
the dependecy issues correctly by selecting, and if required compiling, a
compatible set of libraries.

Therefore, the preferred way of installing SpatialIndex locally, such as a
laptop is

.. code-block:: bash

    spack install spatial-index


How to install from source
---------------------------

As a developer of SpatialIndex you'll need to build from source. The steps are
to first install the required dependecies. On BB5 you can simply load the
following modules:

.. code-block:: bash

    module load unstable gcc cmake boost hpe-mpi

On other systems you might need to install them manually. The dependecies are
modern versions of `cmake`, `gcc` and `mpi`; and `boost==1.79.0`.

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


But why can't I 'pip install'?
------------------------------

This question has two parts. Installing wheels is only partially possible
simply because wheels don't support MPI. (There also seems to be no interest in
changing this. Hence, HPC uses Spack or ``conda``.)

If all you need is a read only version of SpatialIndex then you can obtain a
version without MPI from from the BBP devpi

.. code-block:: bash

    pip install --index https://bbpteam.epfl.ch/repository/devpi/simple spatial_index

this will allow you to open and query any index, including multi-indexes.
However, you'll not be able to create multi-indexes.

If you don't need to install from wheel, but are willing to install from a
source distribution. You can obtain a fully functioning version of SpatialIndex
as follows

.. code-block:: bash

    pip install --index https://bbpteam.epfl.ch/repository/devpi/simple spatial_index --no-binary spatial_index

However, it's important that you have the required dependecies installed. Please
check the section on installing from source.
