import logging
import os
import sys
from abc import abstractmethod

import numpy as np


class ChunkedProcessingMixin:

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
    def create(cls, *args, progress=False, return_indexer=False, **kw):
        """Interactively create, with some progress"""
        indexer = cls(*args, **kw)
        indexer.process_all(progress)
        return indexer if return_indexer else indexer.index

    @classmethod
    def create_parallel(cls, *ctor_args, num_cpus=None, progress=False):
        import functools
        import multiprocessing
        if num_cpus is None:
            num_cpus_env = os.environ.get("SLURM_TASKS_PER_NODE")
            num_cpus = int(num_cpus_env) if num_cpus_env else multiprocessing.cpu_count()

        # make indexer global, so that in each runner process, among processing chunks,
        # morphologies dont get deleted
        indexer = globals()["indexer"] = cls(*ctor_args)
        n_elements = indexer.n_elements_to_import()
        nchunks = int(n_elements) / cls.N_ELEMENTS_CHUNK
        ranges = gen_ranges(n_elements, cls.N_ELEMENTS_CHUNK)
        # use functools as lambdas are not serializable
        build_index = functools.partial(_process_range_increment, cls, ctor_args)

        logging.info("Running in parallel. CPUs=" + str(num_cpus))
        with multiprocessing.Pool(num_cpus) as pool:
            for i, _ in enumerate(pool.imap_unordered(build_index, ranges)):
                if progress:
                    show_progress(i + 1, nchunks)
        return indexer  # the indexer on rank0


def _process_range_increment(cls, ctor_args, part):
    # Instantiate indexer just once per process
    indexer = globals().get("indexer")
    if not indexer:
        logging.debug("No cached indexer. Building new... %s", ctor_args)
        indexer = globals()["indexer"] = cls(*ctor_args)
    indexer.process_range(part)


def gen_ranges(limit, blocklen, low=0):
    for high in range(low + blocklen, limit, blocklen):
        yield low, high
        low = high
    if low < limit:
        yield low, limit


def ranges_with_progress(limit, blocklen, low=0):
    nchunks = (limit - low - 1) // blocklen + 1
    show_progress(0, nchunks)
    for i, range_ in enumerate(gen_ranges(limit, blocklen, low)):
        yield range_
        show_progress(i + 1, nchunks)


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


def show_progress(iteration, total, prefix='Progress:', decimals=1, length=80, fill='█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% ', end="   ", flush=True)
    # Print New Line on Complete
    if iteration == total:
        print()


def check_free_space(size, path):
    """
    Check if there's enough free space on the drive where the
    memory mapped file is allocated. Size is in bytes.
    """
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize >= size


def get_dirname(path):
    return os.path.dirname(path) or "."


class DiskMemMapProps:
    """A class to configure memory-mapped files as the backend for spatial indices."""

    def __init__(self, map_file, file_size=1024, close_shrink=False):
        self.memdisk_file = map_file
        self.file_size = file_size
        self.shrink = close_shrink

    @property
    def args(self):
        return self.memdisk_file, self.file_size, self.shrink


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
    def create(cls, *args, output_dir=None, progress=False, return_indexer=False, **kw):
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
            indexer.index = cls.open_index(output_dir, mem=10**9)
            print(f"index elements: {len(indexer.index)}")

        if return_indexer:
            return indexer
        else:
            return indexer.index if mpi_rank == 0 else None


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
