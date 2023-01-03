Developer Guide
===============

This short guide contains information about the development process
of SpatialIndex.

Design
------

SpatialIndex allows to create indexes and perform volumetric queries with
efficiency in mind. It was developed on top of the Boost Geometry.Index
libraries, renown for their excellent performance.

SpatialIndex specializes to several core geometries, namely spheres and
cylinders, and derivates: indexed spheres/segments and "unions" of them
(variant). These objects are directly stored in the tree structure and have
respective bounding boxes created on the fly. As such some CPU cycles were
traded for substantial memory savings, aiming at near-billion indexed geometries
per cluster node.

Public API: C++
---------------

The core library is implemented as a C++17 Header-Only library and offers a wide
range of possibilities.

By including ``spatial_index/index.hpp`` and ``spatial_index/util.hpp`` the user has
access to the whole API which is highly flexible due to templates. For instance
the IndexTree class can be templated to any of the defined structures or new
user defined geometries, providing the user with a boost index rtree object
which automatically supports serialization and higher level API functions.

Please refer to the unit tests (under ``tests/cpp/unit_tests.cpp``) to see examples
on how to create several kinds of IndexTrees.


Public API: Python
------------------

The Python API gives access to the most common instances of 'IndexTree's, while
adding higher level API and IO helpers.  The user may instantiate a Spatial
Index of Morphology Pieces (both Somas and Segments) or, for maximum efficiency,
Spheres alone.

As so, if it is enough for the user to create a Spatial Index of spherical
geometries, like soma placement, he should rather use
:class:`spatial_index.SphereIndex`. For the cases where Spatial Indexing of
cylindrical or mixed geometries is required, e.g. full neuron geometries, the
user shall use  :class:`spatial_index.MorphIndex`. Furthermore, the latter
features higher level methods to help in the load of entire Neuron morphologies.

I/O Policy
----------
The rules are
  * SpatialIndex will create intermediate directories if required.
  * SpatialIndex will throw a meaningful error message when trying to open a
    missing file. Please use ``spatial_index::util::open_ifstream`` to open an
    ``std::ifstream`` with the appropriate checks.

There's also ``spatial_index::util::open_ofstream`` to open output file streams.

Logging Policy
--------------
There are three levels of logging
  * **info** This level contains information that is meaningful and of interest
    to the user. Typically its purpose is to understand what the library is doing,
    on a coarse level. The size of the log must be kept easily human readable.

  * **warn** This level contains information that might indicate a problem. For example,
    an invalid setting that falls back to a default.

  * **error** Additional information about the error not present in the exception.

Note, logging is distinct from error handling, i.e. calling ``log_error`` will not
throw an exception automatically.

The Python side should use 

.. code-block:: python

    import spatial_index

    spatial_index.logger.info("Hi, let's get started.")
    spatial_index.logger.warning("This is how to log warnings.")

    spatial_index.logger.error(
        "For demonstration purposes we will raise and exception now."
    )
    raise ValueError("Mysterious error.")

Please don't write to the root logger via ``logging.info`` or similar.

The C++ core allows registering a callback to which all logging is forwarded.
When used as a Python library SpatialIndex will register (a wrapper around) the
Python logger ``spatial_index.logger`` as the callback. Therefore, all log
messages from the C++ side are handles by the Python logging library. A different
logger can be registered by

.. code-block:: python

    spatial_index.register_logger(some_other_logger)


This enables, with reasonable effort, to send logs to a specific file (one per
MPI rank), etc.

On the C++ side please use:

.. code-block:: c++

    #include <spatial_index/logging.hpp>

    namespace spatial_index {
       log_info("Hello!");
       log_info(boost::format("Hello %s!") % "Alice");

       log_warn("This might not be as intended.");

       log_error("oops.");
       raise std::runtime_error("tja.");
    }


While nobody admits using ``printf`` debugging, here's a trick:

.. code-block:: c++

    #include <spatial_index/logging.hpp>

    SI_LOG_DEBUG("bla....");
    SI_LOG_DEBUG_IF(
        i == 42,
        boost::format("%d: %e %e) % i % x % y)
    );

This is interesting because you can break up the output by MPI rank; and
therefore get a clean stream of messages from each MPI rank.


.. _`Environment Variables`:

Environment Variables
---------------------

The following environmental variables are used by SpatialIndex:

* ``SI_LOG_SEVERITY``: can be used to control the minimum severity that log message need to have.
  Valid values are ``INFO``, ``WARN``, ``ERROR``, ``DEBUG``.
  Note that DEBUG requires that SI was built with ``SI_ENABLE_LOG_DEBUG``.
  The default is ``INFO``.
* ``SI_REPORT_USAGE_STATS``: if activated by assigning it to ``On`` or ``1``,
  the multi-index cache usage statistics report gets saved to disk.
  By default it is deactivated.

Boost Serialization & Struct Versioning
---------------------------------------

SpatialIndex uses ``boost::serialization`` to write indexes to disk. In order
to be able to at least detect old indexes; and ideally be able to also open
them, we need to version each serialized struct.

There are a few things to respect:

* Boost versions individual classes.
* The base class must be serialized through
  ``boost::serialization::base_object<Base>(*this)``; and must not use
  ``static_cast`` since this will silently work but fails to serialize important
  type information.
* It (effectively) requires that every class has a private `serialize` method,
  even if it only serializes its base class. In toy examples it was easy to modify
  an existing ``serialize`` method. However, adding one to a derived class
  would never work properly. In particular the difficulty was opening classes
  serialized through the old protocol, i.e. without a ``serialize`` method in
  the base.

SpatialIndex defines a constant ``SPATIAL_INDEX_STRUCT_VERSION`` which defines the
version of the structs that are serialized. This is the global version that every
struct will use. Therefore, when creating a new class that needs to be
serialized, e.g., because it's part of something that's being serialized, then
it must set its version to ``SPATIAL_INDEX_STRUCT_VERSION``; and assert that the
version is not ``0``. (This last part is only to check that you haven't
forgotten to set the version.)

Include and Inline Policy
-------------------------

Every header should include everything it needs to be used by itself. There should
be no include ordering, transitive assumptions about what's already included are
permitted.

This also affect the ``inline`` policy, i.e. everything needs to either be a
template or be inline ot avoid violations of the ODR.

In order to check that all headers ``<spatial_index/*.hpp>`` adhere to these rules
we've added one compilation unit per header in ``tests/cpp/check_headers/*.cpp``.
These simply include the header and then the compiler can check if the rules are
adhered. In order to generate the required dummy code, we have a script:

.. code-block:: bash

    bin/update_header_checks.py

(It must be run in the project root.)

History
-------

This section contains summaries of things that have been attempted in the past,
but have been removed without leaving any (obvious) traces behind.

Memory mapping
^^^^^^^^^^^^^^

Memory mapping refers to mapping parts of a file into the virtual address
space. Hence the "memory" can be allocated from the file.

This was used to create an R-Tree which was backed by memory stored on disk.
Thereby one could create "in-memory" indexes even if this would have required
a few TB of RAM.

The approach was reasonably efficient when the backing disk was an NVME disk.
However, for GPFS was way too slow for this to work.

The Git history contains two commits in which this functionality was removed. In
the first step only the Python bindings were removed. In a second step all memory
mapping code was removed. At time of removal this functionality was complete (but
might have bit rotted slightly).
