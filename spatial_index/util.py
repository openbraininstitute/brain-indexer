import os
import sys
from tqdm import tqdm

import numpy as np

import spatial_index


def gen_ranges(limit, blocklen, low=0):
    for high in range(low + blocklen, limit, blocklen):
        yield low, high
        low = high
    if low < limit:
        yield low, limit


def ranges_with_progress(limit, blocklen, low=0):
    nchunks = (limit - low - 1) // blocklen + 1
    for i, range_ in tqdm(enumerate(gen_ranges(limit, blocklen, low)),
                          total=nchunks,
                          desc="Indexing in progress",
                          unit="chunk(s)",
                          colour="#FF33FB"):
        yield range_


def docopt_get_args(func, extra_args=None):
    """Get all CLI  args via docopt
    """
    from docopt import docopt
    docopt_opts = docopt(func.__doc__, extra_args)
    opts = {}
    for key, val in docopt_opts.items():
        key = key.strip("<>-").replace("-", "_")
        if isinstance(val, str):
            if val.lower() in ("off", "false"):
                val = False
            elif val.lower() in ("on", "true"):
                val = True
        opts[key] = val
    return opts


def check_free_space(size, path):
    """
    Check if there's enough free space on the drive where the
    memory mapped file is allocated. Size is in bytes.
    """
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize >= size


def get_dirname(path):
    return os.path.dirname(path) or "."


def balanced_chunk(n_elements, n_chunks, k_chunk):
    chunk_size = n_elements // n_chunks
    n_large_chunks = n_elements % n_chunks

    low = k_chunk * chunk_size + min(k_chunk, n_large_chunks)
    high = (k_chunk + 1) * chunk_size + min(k_chunk + 1, n_large_chunks)

    return min(low, n_elements), min(high, n_elements)


def register_mpi_excepthook():
    # Credit: https://stackoverflow.com/a/16993115

    def handle_exception(exc_type, exc_value, exc_traceback):
        spatial_index.logger.error(
            "Exception raised, calling MPI_Abort",
            exc_info=(exc_type, exc_value, exc_traceback)
        )

        from mpi4py import MPI
        MPI.COMM_WORLD.Abort(-1)

    sys.excepthook = handle_exception


def is_likely_same_index(lhs, rhs, confidence=0.99, error_rate=0.001, rtol=1e-5):
    """Are the two indexes `lhs` and `rhs` likely the same?

    This will first perform a few fast, deterministic checks to check if the two
    indexes are impossibly the same. Then follow it up with a sampling based
    check.

    Assuming that the elements in the index are uniformly distributed, then
    `confidence` is the frequency which which this test succeeds, if
    ratio of different elements is `error_rate`.
    """
    if confidence == 1.0 or error_rate == 0.0:
        raise NotImplementedError(
            "The deterministic edge cases haven't been implemented."
        )

    n_elements = len(lhs)
    if n_elements != len(rhs):
        spatial_index.logger.info("The number of elements in the two indexes differ.")
        return False

    lhs_box = lhs.bounds()
    rhs_box = rhs.bounds()

    lhs_extent = lhs_box[1] - lhs_box[0]
    rhs_extent = rhs_box[1] - rhs_box[0]

    atol = rtol * np.maximum(lhs_extent, rhs_extent)

    for lhs_xyz, rhs_xyz in zip(lhs_box, rhs_box):
        if not np.all(np.abs(lhs_xyz - rhs_xyz) < atol):
            spatial_index.logger.info("The bounding boxes of the two indexes differ.")
            return False

    box = lhs_box
    extent = lhs_extent

    # For uniformly distributed small elements we'd expect
    # on average one element in a box of this size.
    n_elements_per_dim = n_elements ** (1.0 / 3.0)
    sampling_extent = extent / n_elements_per_dim

    # Assuming we're picking elements randomly from two large sets, what's
    # the probability of drawing `n` equal elements if the error rate is
    # `1.0 - p`:
    #
    #    p ** n =: alpha
    #
    # Therefore,
    #    n = ceil(log_p(alpha)) = ceil(log(alpha)) / log(p))
    # is the number of sample we need to draw. Note that,
    #
    #    p == 1.0 - error_rate
    #    alpha == 1.0 - confidence.
    max_elements_checked = int(np.ceil(
        np.log(1.0 - confidence) / np.log(1.0 - error_rate)
    ))

    # A bit loose, but we'll assume that we get one element per
    # query (on average).
    n_queries = max_elements_checked

    for _ in range(n_queries):
        xyz = np.random.uniform(box[0], box[1])
        min_corner = xyz - 0.5 * sampling_extent
        max_corner = xyz + 0.5 * sampling_extent

        if not is_window_query_equal(lhs, rhs, min_corner, max_corner, atol):
            return False

    return True


def is_window_query_equal(lhs, rhs, min_corner, max_corner, atol):
    """Does the window queryreturn the same result?

    Given the two indexes `rhs` and `lhs`, will performing the same
    window query on both indexes return the same result? Here "same"
    is stable to small differences due to floating point arithmetic.

    The `atol` is the absolute tolerance for comparing coordinates. The value
    can be different for each dimension.
    """
    def is_contained(a, b):
        return is_window_query_contained(a, b, min_corner, max_corner, atol)

    return is_contained(lhs, rhs) and is_contained(rhs, lhs)


def is_window_query_contained(lhs, rhs, min_corner, max_corner, atol):
    """Are results from one query contained in the other?

    Given the two indexes `rhs` and `lhs`, will performing the window
    query on `lhs` be contained in the a slightly inflated window
    query on `rhs`?

    The `atol` is the size by which the window is inflated.
    """
    lhs_results = lhs.window_query(min_corner, max_corner)
    rhs_results = rhs.window_query(min_corner - atol, max_corner + atol)

    if all(key in lhs_results for key in ["gid", "section_id", "segment_id"]):
        # Morphology indexes
        keys = ["gid", "section_id", "segment_id"]
        dtype = "i,i,i"

    elif "id" in lhs_results:
        # Synapse indexes (and more).
        keys = ["id"]
        dtype = "i"

    else:
        raise NotImplementedError(
            "This isn't implemented yet for {} and {}".format(
                type(lhs),
                type(rhs)
            )
        )

    if lhs_results[keys[0]].size == 0:
        return True

    def pack_ids(r):
        return np.array(sorted([ijk for ijk in zip(*[r[k] for k in keys])]), dtype=dtype)

    lhs_ids = pack_ids(lhs_results)
    rhs_ids = pack_ids(rhs_results)

    is_equal = np.all(np.isin(lhs_ids, rhs_ids))

    if not is_equal:
        spatial_index.logger.info(f"The two indexes diff:\n{lhs_ids}\n{rhs_ids}")

    return is_equal
