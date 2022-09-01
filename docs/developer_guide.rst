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
which automamtically supports serialization and higher level API functions.

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

.. code-block: python

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

.. code-block: python

    spatial_index.register_logger(some_other_logger)



This enables, with reasonable effort, to send logs to a specific file (one per
MPI rank), etc.

On the C++ side please use:

.. code-block: c++

    #include <spatial_index/logging.hpp>

    namespace spatial_index {
       log_info("Hello!");
       log_info(boost::format("Hello %s!") % "Alice");

       log_warn("This might not be as intended.");

       log_error("oops.");
       raise std::runtime_error("tja.");
    }


While nobody admits using ``printf`` debugging, here's a trick:

.. code-block: c++

    #include <spatial_index/logging.hpp>

    SI_LOG_DEBUG("bla....");
    SI_LOG_DEBUG_IF(
        i == 42,
        boost::format("%d: %e %e) % i % x % y)
    );

This is interesting because you can break up the output by MPI rank; and
therefore get a clean stream of messages from each MPI rank.
