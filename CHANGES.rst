Change Log
==========

Version 0.6.0
-----------
**August 2022**

**Features**
  * Introduced MultiIndex for parallel indexing
  * Queries can now be performed in bounding box or exact mode
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
