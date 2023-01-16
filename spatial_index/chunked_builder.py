from abc import ABCMeta, abstractmethod

import numpy as np

import spatial_index

from . import core
from .util import ranges_with_progress, gen_ranges
from .util import register_mpi_excepthook, balanced_chunk


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
    def create(cls, *args, progress=False, output_dir=None, **kw):
        """Interactively create, with some progress"""
        index_builder = cls(*args, **kw)
        index_builder.process_all(progress)
        index_builder._core_builder._finalize()

        index_builder._write_index_if_needed(output_dir)
        return index_builder._index_if_loaded


class MultiIndexBuilderMixin:
    def _finalize(self):
        self._core_builder._finalize()

    def local_size(self):
        return self._core_builder.local_size()

    @classmethod
    def create(cls, *args, output_dir=None, progress=False, **kw):
        """Interactively create, with some progress"""
        from mpi4py import MPI
        register_mpi_excepthook()

        builder = cls(*args, output_dir=output_dir, **kw)

        comm = MPI.COMM_WORLD
        mpi_rank = comm.Get_rank()
        comm_size = comm.Get_size()

        def is_valid_comm_size(comm_size):
            return core.is_valid_comm_size(comm_size - 1)

        if not is_valid_comm_size(comm_size):
            spatial_index.logger.error(
                "SpatialIndex requires that the number of MPI "
                "ranks is equal to `N = 2**n * 3**m * 5**l + 1`, "
                "e.g., 2 + 1, 4*3 + 1, 72 + 1 ... Please ensure you're using "
                "`mpirun -np N` or `srun -n N` with a valid number "
                "of MPI ranks N."
            )
            raise ValueError(f"Invalid communicator size, comm_size={comm_size}.")

        work_queue = MultiIndexWorkQueue(comm)

        if mpi_rank == work_queue.distributor_rank:
            work_queue.distribute_work(builder.n_elements_to_import())
        else:
            while (chunk := work_queue.request_work(builder.local_size())) is not None:
                builder.process_range(chunk)

        comm.Barrier()
        if mpi_rank == 0:
            spatial_index.logger.info("Starting to build distributed index.")

        builder._finalize()
        comm.Barrier()


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

    @property
    def distributor_rank(self):
        return self._distributor_rank

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
