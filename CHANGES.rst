Change Log
==========

Version 2.0.0
-------------

We've removed one method. It seems unlikely that users would be affected by
this. Migration instructions are provided in the corresponding bullet point.

**Features**
  * CLI supports `--verbose` which prints log messages of any severity.
  * Infrastructure for creating indexes with randomly placed elements.
  * Support adding spheres to in-memory `SphereIndex`.
  * Support indexes of points `PointIndex`.

**Improvements**
  * Create in-memory indexes "in bulk". The internal structure of an R-tree is
    much better if all elements are known upfront; as opposed to being inserted
    one-by-one. This commit ensures that in-memory indexes are created via bulk
    creation.

    It optionally, old in-memory indexes should be recreated to benefit from
    this optimization. However, old indexes can still be read. Multi-indexes
    aren't affected at all by this change.

  * Use `std::filesystem` instead of `boost::filesystem`.

  * General restructuring of code and CI.

  * Added test to ensure consistency of morphology IDs between SpatialIndex and
    `libsonata`.

**Deprecation**
  * We've deprecated the certain constructors of `core.MorphIndex` and
    `core.SynapseIndex`. Users of SpatialIndex aren't expected to be using
    these classes directly.

**Removal**
  * We've removed a method `add_sphere` from the `SphereBuilder`. Instead
    please create all spheres upfront and then create the index. If upfront
    creation of all spheres doesn't work, the in-memory `SphereIndex` supports
    adding spheres interatively.

**Fixes**
  * Improved error message for missing files.
  * Warn if in-memory indexes are large.
  * Improved documentation.
  * Progress bar is only shown when in a TTY.

Version 1.2.1
-------------
**December 2022**

**Improvements**
  * Fix publishing documentation.

Version 1.2.0
-------------
**December 2022**

**Features**
  * Ability to optimize the order in which queries should be performed to
    improve cache reuse of multi-indexes.

**Improvements**
  * Document SpatialIndex environment variables.
  * Document important performance tricks.
  * Fix morphology builder to return SONATA enabled indexes.
  * Relax the minimum number of synapses in a multi-index.
  * Improvements to the overall code quality.

Version 1.1.0
-------------
**November 2022**

**Features**
  * Integrates ``section_type`` in index. This allows to retrieve information on the
    type of section (i.e. ``soma``, ``axon``, ``basal_dendrite``, ``apical_dendrite``)
    directly from the query.
  * Add ``box_empty`` and ``sphere_empty`` functions to check if a specific area is empty.

**Improvements**
  * Ignore ``virtual`` populations in circuit config files.
  * Populations with unsupported types are not allowed anymore.
  * Relaxed requirement on number of MPI ranks: now can be in the form ``2**n * 3**m * 5**l + 1``.
  * Workaround for ``BOOST_STATIC_CONSTANT`` build issue.
  * Multiple improvements to CI.
  * Removed memory mapped files support entirely.
  * Multiple fixes and improvements to the implementation.

Version 1.0.0
-------------
**October 2022**

No changes since version ``0.9.0``.

Version 0.9.0
-------------
**October 2022**

**Breaking Changes**
  * Implementation details w.r.t ``boost::serialize`` require a breaking change
    in how indexes are stored. Old indexes cannot be loaded anymore and need to
    be recreated.
  * Remove MVD3 support. Only SONATA is supported. If you need MVD3 you must
    convert the circuit to the new format.
  * The keyword argument ``target_gids`` for ``MorphologyBuilder.from_sonata_file`` has been
    renamed ``gids``. Please update your code before version 1.0 as the old name will be removed.
  * The key ``gid`` for ``group_by`` has been renamed ``post_gid`` to reflect that the grouping is
    happening according to the value of ``post_gid``. Please update your code
    before version 1.0 as the old name will be removed.

**Features**
  * ``accuracy="best_effort"`` is now the default.
  * The radius of a segment is equal to the average of the radius at the two
    endpoints.

**Improvements**
  * Improves documentation.
  * Improvements due to QA feedback.
  * Bug fixes.
  * Improve unit-testing with MPI.
  * Improve CI: checking wheels and sdist.
  * Improve integration tests: all usecases1-5

Version 0.8.3
-------------
**September 2022**

**Improvements**
  * Improves documentation.

Version 0.8.2
--------------
**September 2022**

**Improvements**
  * Improves documentation.
  * Fixes OOM issue for synapse indexes that select by target GID.
  * Fixes assert of `radii` during construction of morphology indexes.

Version 0.8.1
--------------
**September 2022**

**Improvements**
  * Improves help and error messages.

Version 0.8.0
--------------
**September 2022**

**Features**
  * Implemented multi-population support for indexes

**Improvements**
  * Boxes are defined through any two opposing corners, not just the min- and max-corners.
  * "window_query" and "vicinity_query" are now "box_query" and "sphere_query", respectively; and analogously for "{window,vicinity}_counts".
  * "endpoints" are now exported as a tuples and not as two separate objects
  * "kind" field has been replaced by "is_soma"
  * Removed support for memory mapped files from the Python API
  * The multi-index cache usage statistics report has been deactivated by default, available on-demand by setting the environment variable "SI_REPORT_USAGE_STATS" to "1" or "On"
  * Consistency improvements for the code
  * Bug fix for multi-index creation.

Version 0.7.0
-------------
**September 2022**

**Features**
  * Overhaul of the Python APIs: API v2 (more info here: https://bbpteam.epfl.ch/project/spaces/x/MBStBg)
  * Aligns internal identifier packing with TouchDetector. Requires rebuilding of existing indexes.
  * Opening indexes from disk now requires a single command for every kind of index
  * New Python logging infrastructure

**Improvements**
  * Fixed issue in radius calculation
  * Clean-up of the code base from unused code
  * Improved validation using BluePy cross-checks
  * Lots of bug fixes


Version 0.6.0
-------------
**August 2022**

**Features**
  * Introduced MultiIndex for parallel indexing
  * Queries can now be performed in bounding box or best-effort mode
  * Bulk return of values from queries as a dictionary of numpy arrays
  * Support for .json file for CLI tools
  * A full-fledged tutorial written in a Jupyter Notebook

**Improvements**
  * Big improvements to CI
  * Optimizations to collision detection
  * C++ backend now upgraded to C++17
  * Improved documentation
  * Lots of bug fixes


Version 0.5.x
-------------
**April 2022**

**Features**
  * Out-of-core support for node indexing
  * Support for pre and post synaptic gids

**Improvements**
  * Renamed NodeMorphIndexer to MorphIndexBuilder for clarity
  * Introduced free space check for memory mapped files
  * Improved documentation


Version 0.4.x
-------------
**November 2021**

**Features**
  * Support for SONATA Selections for NodeMorphIndexer
  * Add API to support counting elements and aggregate synapses by GID
  * Chunked Synapse indexer feat progress monitor
  * More flexible ranges: python-style (start, end, [step])

**Improvements**
  * New CI (Gitlab): tests, wheels & docs, fix tox, drop custom setup.py docs
  * Building and distributing wheels
  * Added more examples and benchmarking scripts
  * Added new classes to documentation API


Version 0.3.0
-------------
**August 2021**

A major, and long waited, update since the previous release.
This is the first version effectively validated against FLAT index results.
It would take a lot of time to reconstruct everything that has changed from the first release so we'll just give a brief overview of the changes made in this new shiny version.

*Major changes*
  * Morph object Indices are now tuples (gid, section, segment)
  * New High level API/CLI for loading nodes and edges
  * Initial IndexGrid and bindings, for future very large circuits

*Features*
  * Added support for Section IDs
  * Added support for Synapses Indexer
  * Now supports CLI for indexing circuits
  * Easier installation and interoperability with Sonata
  * Gids, Section and Segment IDs are now ensured to be compliant with FLAT (0/1-based)
  * Lots of validation fixes
  * Improved installation experience
  * Introduced IndexGrid/MultiIndex

*Improvements*
  * Refactoring internal index intities, less inheritance
  * Extensive validation against FLAT
  * Many fixes for robustness and stability


Version 0.2.0
-------------

*Features*
  * Point API
  * Support for window queries
  * has_Soma flag (default=true) in add_neuron to allow the API to add segments only.


Version 0.1.0
-------------

*Features*
  * Support saving and loading dumps

*Improvements*
  * Also some refactoring in the way we collect ids, automatic using `id_getter_for*`
  * Docs and tests


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
