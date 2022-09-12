.. _`Multiple Populations`:

Multiple Populations
====================

Multiple populations are handled by indexing each index separately, and writing
a meta-data file that facilitates loading of the multi population index. Similarly,
queries are simply performed by querying each index separately.

Multi-Population Queries
------------------------

In SI, this has been implemented through ``MultiPopulationIndex``. Which
supports the same API as regular indexes, with a few additions. See
:ref:`Queries`_ for more details.

The difference between single-population indexes and multi-population indexes is
that multi-population indexes the results of queries is a dictionary of
single-population results. For example:

.. code-block: python

    >>> index.box_query(*window, fields="gid")
    {
      "NodeA__NodeA__chemical": np.array([12, 3290, ..., ]),
      "NodeA__NodeB__chemical": np.array([22, 2309, ..., ]),
      ...
    }

    >>> index.box_query(*window, fields=["gid", "radius"])
    {
      "NodeA__NodeA__chemical": {
        "gid": np.array([12, 3290, ..., ]),
        "radius": np.array([0.23, 0.34, ...])
      },
      "NodeA__NodeB__chemical": {
        "gid": ...
        "radius": ...
      }
    }

Keyword Argument: ``populations``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The query will be restricted to only these populations. The default it to return
the results for all indexes. Example:

.. code-block: python

    >>> index.box_query(*window, fields="gid", populations="NodeA__NodeA__chemical")
    {
      "NodeA__NodeA__chemical": np.array([12, 3290, ..., ])
    }

    >>> all_but_one = index.populations[:-1]
    >>> index.box_query(*window, fields="gid", populations=all_but_one)
    {
      "NodeA__NodeA__chemical": np.array([12, 3290, ..., ]),
      "NodeA__NodeB__chemical": np.array([22, 2309, ..., ]),
      ...
    }


Keyword Argument: ``population_mode``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This option is slightly advanced and only interesting if you're writing code
that should be generic accross both single- and multi-popultion indexes. It
controls the return type of queries, i.e., it controls if the dict over
populations is present or not.

There are three options:
  * ``None`` This is the default. Under this mode the query result for
  multi-population indexes is a dictionary of single-population query results.
  While single-population indexes simply return a single-population result.

  * ``"single"`` This can be used to force a multi-population index to behave
  like a single population index, i.e., by not wrapping the single-population
  query result in a dict. Clearly, this requires that the query involves exactly
  one population.

  * ``"multi"`` This forces a single-population index to wrap their result as if
  it were a multi-population index. Note that the name of the population is
  unspecified.


.. _`Writing Generic Code`:

Writing Generic Code
--------------------

This section contains tips on how to use the API to write code that behaves
nicely if one doesn't know if the index is a single-population index or a
multi-population index.

The potential traps:

.. code-block: python

    def special_query(index, window):
        """Query the index suitable for scientific Usecase A."

        results = index.box_query(*window, fields="gid")

        # Bad: fails for multi-population indexes
        largest_gid = np.max(results["gid"]) > 1000

        # Bad: fails for single-population indexes
        largest_gid = np.max(results["NodeA__NodeA__chemical"]["gid"])

        if largest_gid > 1000: 
            print("Large GID spotted.")


Usecase 1: Single Population Queries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The piece of code knows it's only dealing with a single population. In this
case, we can coerce the result into the single-population format:

.. code-block: python

    def special_query(index, window, population=None):
        """Query the index suitable for scientific Usecase A."

        results = index.box_query(
            *window, fields="gid", population=population,
            population_mode="single"
        )

        # Good: works for both single- and multi-population indexes,
        # because of `population_mode="single"`.
        largest_gid = np.max(results["gid"]) > 1000


Usecase 2: Multiple Population Queries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this usecase the code know how to handle multiple population if present.
Then, one can choose to always use the multi-population return type:

.. code-block: python

    def special_query(index, window, populations=None):
        """Print larges GID."

        results = index.box_query(
            *window, fields="gid", population=population,
            population_mode="multiple"
        )

        for pop, result in results.items():
            largest_gid = np.max(result["gid"])
            print(f"{pop=}: {largest_gid}")
