import logging
import os
import sys
from tqdm import tqdm
from abc import ABCMeta, abstractmethod

import numpy as np


class ChunkedProcessingMixin(metaclass=ABCMeta):

    N_ELEMENTS_CHUNK = 100

    @abstractmethod
    def n_elements_to_import(self):
        return NotImplemented

    @abstractmethod
    def process_range(self, range_):
        return NotImplemented

    def process_all(self, progress=False):
        n_elements = self.n_elements_to_import()
        make_ranges = ranges_with_progress if progress else gen_ranges
        for range_ in make_ranges(n_elements, self.N_ELEMENTS_CHUNK):
            self.process_range(range_)

    @classmethod
    def create(cls, *args, progress=False, **kw):
        """Interactively create, with some progress"""
        index_builder = cls(*args, **kw)
        index_builder.process_all(progress)
        return index_builder.get_object()


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
        logging.error(
            "Exception raised, calling MPI_Abort",
            exc_info=(exc_type, exc_value, exc_traceback)
        )

        from mpi4py import MPI
        MPI.COMM_WORLD.Abort(-1)

    sys.excepthook = handle_exception


class MultiIndexBuilderMixin:
    def finalize(self):
        self.index.finalize()

    def local_size(self):
        return self.index.local_size()

    @classmethod
    def create(cls, *args, output_dir=None, progress=False, **kw):
        """Interactively create, with some progress"""
        from mpi4py import MPI
        register_mpi_excepthook()

        indexer = cls(*args, output_dir=output_dir, **kw)

        comm = MPI.COMM_WORLD
        mpi_rank = comm.Get_rank()
        comm_size = comm.Get_size()

        def is_power_of_two(n):
            # Credit: https://stackoverflow.com/a/57025941
            return (n & (n - 1) == 0) and n != 0

        assert is_power_of_two(comm_size - 1)

        work_queue = MultiIndexWorkQueue(comm)

        if mpi_rank == comm_size - 1:
            work_queue.distribute_work(indexer.n_elements_to_import())
        else:
            while (chunk := work_queue.request_work(indexer.local_size())) is not None:
                indexer.process_range(chunk)

        print(f"local_size = {indexer.local_size()}", flush=True)
        comm.Barrier()
        indexer.finalize()

        comm.Barrier()

        if mpi_rank == 0:
            # The generated index has to be re-opened as a whole
            indexer.index = cls.IndexClass.open_core_index(output_dir, mem=10**9)
            print(f"index elements: {len(indexer.index)}")

        return indexer.get_object()

    @classmethod
    def load_dir(cls, output_dir, data_file, mem=10**9):
        """Creates an extended multi-index from a raw multi index directory
        with `mem` bytes of memory allowance
        """
        return cls.IndexClass.from_dir(output_dir, mem, data_file, mem)


class MultiIndexWorkQueue:
    """Dynamic work queue for loading even number of elements.

    The task is to distribute jobs with IDs `[0, ..., n_jobs)` to
    the MPI ranks in such a manner that the total weight of the assigned
    jobs is reasonably even between MPI ranks.

    Example: Assign neurons to each MPI ranks such that the total number of
    segments is balanced across MPI ranks.

    Note: The rank performing the distribution task is the last rank, i.e.
    `comm_size - 1`.
    """
    def __init__(self, comm):
        self.comm = comm
        self.comm_rank = comm.Get_rank()
        self.comm_size = comm.Get_size()
        self._n_workers = self.comm_size - 1

        self._current_sizes = np.zeros(self.comm_size, dtype=np.int64)
        self._is_waiting = np.full(self.comm_size, False)

        self._request_tag = 2388
        self._chunk_tag = 2930

        self._distributor_rank = self.comm_size - 1

    def distribute_work(self, n_elements):
        """This is the entry-point for the distributor rank."""
        assert self.comm_rank == self._distributor_rank, \
            "Wrong rank is attempting to distribute work."

        assert self._n_workers <= n_elements, \
            "More worker ranks than elements to process."

        n_chunks = min(n_elements, 100 * self._n_workers)
        chunks = [
            balanced_chunk(n_elements, n_chunks, k_chunk)
            for k_chunk in range(n_chunks)
        ]

        k_chunk = 0
        while k_chunk < n_chunks:
            # 1. Listen for anyone that needs more work.
            self._receive_request()

            # 2. Compute the eligible ranks.
            avg_size = np.sum(self._current_sizes) / self._n_workers

            is_eligible = np.logical_and(
                self._current_sizes <= 1.05 * avg_size,
                self._is_waiting
            )
            eligible_ranks = np.argwhere(is_eligible)[:, 0]

            # 3. Send work to all eligible ranks.
            for rank in eligible_ranks:
                if k_chunk < n_chunks:
                    self._send_chunk(chunks[k_chunk], rank)
                    self._is_waiting[rank] = False
                    k_chunk += 1

        # 4. Send everyone an empty chunk to signal that there's no more work.
        for rank in range(self._n_workers):
            if not self._is_waiting[rank]:
                self._receive_local_count()

            self._send_chunk((0, 0), rank)

    def request_work(self, current_size):
        """Request more work from the distributor.

        If there is more work, two integers are turned, the assigned work
        is the range `[low, high)`. If there is nomore work `None` is returned.

        The `current_size` must be the current total weight of all jobs that
        have been assigned to this MPI rank.
        """
        assert self.comm_rank != self._distributor_rank, \
            "The distributor rank is attempting to receive work."

        self._send_local_count(current_size)
        return self._receive_chunk()

    def _send_chunk(self, raw_chunk, dest):
        chunk = np.empty(2, dtype=np.int64)
        chunk[0] = raw_chunk[0]
        chunk[1] = raw_chunk[1]

        self.comm.Send(chunk, dest=dest, tag=self._chunk_tag)

    def _receive_chunk(self):
        chunk = np.empty(2, dtype=np.int64)
        self.comm.Recv(chunk, source=self._distributor_rank, tag=self._chunk_tag)

        return chunk if chunk[0] < chunk[1] else None

    def _send_local_count(self, raw_local_count):
        local_count = np.empty(1, dtype=np.int64)
        local_count[0] = raw_local_count
        self.comm.Send(local_count, dest=self._distributor_rank, tag=self._request_tag)

    def _receive_local_count(self):
        from mpi4py import MPI

        local_count = np.empty(1, dtype=np.int64)
        status = MPI.Status()
        self.comm.Recv(
            local_count,
            source=MPI.ANY_SOURCE,
            tag=self._request_tag,
            status=status,
        )
        source = status.Get_source()

        return local_count[0], source

    def _receive_request(self):
        local_count, source = self._receive_local_count()

        # This rank is now waiting for work.
        self._is_waiting[source] = True
        self._current_sizes[source] = local_count


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
        logging.info("The number of elements in the two indexes differ.")
        return False

    lhs_box = lhs.bounds()
    rhs_box = rhs.bounds()

    lhs_extent = lhs_box[1] - lhs_box[0]
    rhs_extent = rhs_box[1] - rhs_box[0]

    atol = rtol * np.maximum(lhs_extent, rhs_extent)

    for lhs_xyz, rhs_xyz in zip(lhs_box, rhs_box):
        if not np.all(np.abs(lhs_xyz - rhs_xyz) < atol):
            logging.info("The bounding boxes of the two indexes differ.")
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
    lhs_results = lhs.find_intersecting_window_np(min_corner, max_corner)
    rhs_results = rhs.find_intersecting_window_np(min_corner - atol, max_corner + atol)

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
        print(min_corner)
        print(max_corner)

        print(lhs_results)
        print(rhs_results)
        logging.info(f"The two indexes diff:\n{lhs_ids}\n{rhs_ids}")

    return is_equal
