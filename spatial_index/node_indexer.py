#!/bin/env python
#
# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

"""
    Implementation of MvdMorphIndexer which, builds a MorphologyIndex
    and indexes all cells contained in an MVD/SONATA file
"""

import itertools
import warnings; warnings.simplefilter("ignore")  # NOQA
import logging

from collections import namedtuple
from os import path as ospath

import morphio
import mvdtool
import numpy as np
import quaternion as npq

from ._spatial_index import MorphIndex, MorphIndexMemDisk
from ._spatial_index import MorphMultiIndex

try:
    from ._spatial_index import MorphMultiIndexBulkBuilder
except ImportError:
    logging.warning("SpatialIndex was built without MPI support.")

from .util import ChunkedProcessingMixin, DiskMemMapProps

morphio.set_ignored_warning(morphio.Warning.only_child)
MorphInfo = namedtuple("MorphInfo", "soma, points, radius, branch_offsets")


class MorphologyLib:

    def __init__(self, pth):
        self._pth = pth
        self._morphologies = {}

    def _load(self, morph_name):
        if ospath.isfile(self._pth):
            morph = morphio.Morphology(self._pth)
        elif ospath.isdir(self._pth):
            morph = morphio.Morphology(ospath.join(self._pth, morph_name) + ".asc")
        else:
            raise Exception("Morphology path not found: " + self._pth)

        soma = morph.soma
        morph_infos = MorphInfo(
            (soma.center, soma.max_distance),
            morph.points,
            morph.diameters,
            morph.section_offsets,
        )
        self._morphologies[morph_name] = morph_infos
        return morph_infos

    def get(self, morph_name):
        return self._morphologies.get(morph_name) or self._load(morph_name)


class MorphIndexBuilderBase:
    def __init__(self, morphology_dir, nodes_file, population="", gids=None):
        """Initializes a node index builder

        Args:
            morphology_dir (str): The file/directory where morphologies reside
            nodes_file (str): The Sonata/mvd nodes file
            population (str, optional): The nodes population. Defaults to "" (default).
            gids ([type], optional): A selection of gids to index. Defaults to None (All)
        """

        self.morph_lib = MorphologyLib(morphology_dir)
        self.mvd = mvdtool.open(nodes_file, population)
        self._gids = range(0, len(self.mvd)) if gids is None else \
            np.sort(np.array(gids, dtype=int))
        logging.info("Index count: %d cells", len(self._gids))

    def n_elements_to_import(self):
        return len(self._gids)

    def rototranslate(self, morph, position, rotation):
        morph = self.morph_lib.get(morph)

        # mvd files use (x, y, z, w) representation for quaternions. npq uses (w, x, y, z)
        rotation_vector = np.roll(rotation, 1)
        points = (npq.rotate_vectors(npq.quaternion(*rotation_vector).normalized(),
                                     morph.points)
                  if rotation is not None
                  else morph.points)

        points += position
        return points

    def process_cell(self, gid, morph, points, position):
        """ Process (index) a single cell
        """
        morph = self.morph_lib.get(morph)
        soma_center, soma_rad = morph.soma
        soma_center += position
        self.index.add_soma(gid, soma_center, soma_rad)
        self.index.add_neuron(
            gid, points, morph.radius, morph.branch_offsets[:-1], False
        )

    def process_range(self, range_=(None,)):
        """ Process a range of cells.

        :param: range_ (start, end, [step]), or (None,) [all]
        """
        slice_ = slice(*range_)
        cur_gids = self._gids[slice_]
        actual_indices = slice_.indices(len(self._gids))
        assert actual_indices[2] > 0, "Step cannot be negative"
        # gid vec is sorted. check if range is contiguous
        if len(cur_gids) and cur_gids[0] + len(cur_gids) == cur_gids[-1] + 1:
            index_args = (cur_gids[0], len(cur_gids))
        else:
            index_args = (np.array(cur_gids),)  # numpy can init from a range obj

        mvd = self.mvd
        morph_names = mvd.morphologies(*index_args)
        positions = mvd.positions(*index_args)
        rotations = mvd.rotations(*index_args) if mvd.rotated else itertools.repeat(None)

        for gid, morph, pos, rot in zip(cur_gids, morph_names, positions, rotations):
            # GIDs in files are zero-based, while they're typically 1-based in application
            gid += 1
            rotopoints = self.rototranslate(morph, pos, rot)
            self.process_cell(gid, morph, rotopoints, pos)

    @classmethod
    def from_mvd_file(cls, morphology_dir, node_filename, target_gids=None, **kw):
        """ Build a synpase index from an mvd file"""
        return cls.create(morphology_dir, node_filename, "", target_gids, **kw)

    @classmethod
    def from_sonata_selection(cls, morphology_dir, node_filename, pop_name,
                              selection, **kw):
        """ Builds the synapse index from a generic Sonata selection object"""
        return cls.create(morphology_dir, node_filename, pop_name,
                          selection.flatten(), **kw)

    @classmethod
    def from_sonata_file(cls, morphology_dir, node_filename, pop_name, target_gids=None,
                         **kw):
        """ Creates a node index from a sonata node file.

        Args:
            node_filename: The SONATA node filename
            morphology_dir: The directory containing the morphology files
            pop_name: The name of the population
            target_gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits

        """
        return cls.create(morphology_dir, node_filename, pop_name, target_gids,
                          **kw)


class MorphIndexBuilder(MorphIndexBuilderBase, ChunkedProcessingMixin):
    """A MorphIndexBuilder is a helper class to create Spatial Indices (RTree)
    from a node file (mvd or Sonata) and a morphology library.
    When the index is expected to NOT FIT IN MEMORY it can alternatively be set up
    to use MorphIndexMemDisk, according to `disk_mem_map` ctor argument.

    After indexing (ranges of) cells, the user can access the current spatial
    index at the `index` property

    Factories are provided to create & retrieve an index directly from mvd or Sonata
    """

    DiskMemMapProps = DiskMemMapProps  # shortcut

    def __init__(self, morphology_dir, nodes_file, population="", gids=None,
                 disk_mem_map: DiskMemMapProps = None):
        """Initializes a node index builder

        Args:
            morphology_dir (str): The file/directory where morphologies reside
            nodes_file (str): The Sonata/mvd nodes file
            population (str, optional): The nodes population. Defaults to "" (default).
            gids ([type], optional): A selection of gids to index. Defaults to None (All)
            disk_mem_map (DiskMemMapProps, optional): In provided, specifies properties
                of the memory-mapped-file backing this struct [experimental!]
        """
        if disk_mem_map:
            self.index = MorphIndexMemDisk.create(*disk_mem_map.args)
        else:
            self.index = MorphIndex()

        super().__init__(morphology_dir, nodes_file, population, gids)

    @classmethod
    def load_dump(cls, filename):
        """Load the index from a dump file"""
        return MorphIndex(filename)

    @classmethod
    def load_disk_mem_map(cls, filename):
        """Load the index from a dump file"""
        return MorphIndexMemDisk.open(filename)


def balanced_chunk(n_elements, n_chunks, k_chunk):
    chunk_size = n_elements // n_chunks
    n_large_chunks = n_elements % n_chunks

    low = k_chunk * chunk_size + min(k_chunk, n_large_chunks)
    high = (k_chunk + 1) * chunk_size + min(k_chunk + 1, n_large_chunks)

    return min(low, n_elements), min(high, n_elements)


class MorphMultiIndexBuilder(MorphIndexBuilderBase):
    def __init__(self, morphology_dir, nodes_file, population="", gids=None,
                 output_dir=None):

        assert output_dir is not None, f"Invalid `output_dir`. [{output_dir}]"
        self.index = MorphMultiIndexBulkBuilder(output_dir)
        super().__init__(morphology_dir, nodes_file, population=population, gids=gids)

    def finalize(self):
        self.index.finalize()

    def local_size(self):
        return self.index.local_size()

    @classmethod
    def create(cls, *args, output_dir=None, progress=False, return_indexer=False, **kw):
        """Interactively create, with some progress"""
        from mpi4py import MPI

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

    @classmethod
    def open_index(cls, output_dir, mem):
        """Open a multi index with `mem` bytes of memory allowance."""
        return MorphMultiIndex(output_dir, mem)


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

        self._current_sizes = np.zeros(self.comm_size, dtype=np.int64)
        self._is_waiting = np.full(self.comm_size, False)

        self._request_tag = 2388
        self._chunk_tag = 2930

        self._distributor_rank = self.comm_size - 1

    def distribute_work(self, n_elements):
        """This is the entry-point for the distributor rank."""
        assert self.comm_rank == self._distributor_rank, \
               "Wrong rank is attempting to distribute work."

        n_chunks = 100 * (self.comm_size - 1)
        chunks = [
            balanced_chunk(n_elements, n_chunks, k_chunk)
            for k_chunk in range(n_chunks)
        ]

        k_chunk = 0
        while k_chunk < n_chunks:
            # 1. Listen for anyone that needs more work.
            self._receive_request()

            # 2. Compute the eligible ranks.
            avg_size = np.sum(self._current_sizes) / (self.comm_size - 1)

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
        for rank in range(self.comm_size - 1):
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
