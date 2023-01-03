# This file is part of SpatialIndex, the new-gen spatial builder for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import libsonata
import numpy as np

import spatial_index
from . import _spatial_index as core
from .util import chunk_sonata_selection, bcast_sonata_selection
from .index import SynapseIndex
from .builder import _WriteSONATAMetadataMixin, _WriteSONATAMetadataMultiMixin
from .chunked_builder import ChunkedProcessingMixin, MultiIndexBuilderMixin
from .io import open_sonata_edges, validated_sonata_edges_population


class SynapseIndexBuilderBase:
    """Base for building indexes of synapses."""

    N_ELEMENTS_CHUNK = 1
    MAX_SYN_COUNT_RANGE = 100_000

    def __init__(self, sonata_edges, selection):
        self._sonata_edges = sonata_edges
        self._selection = self._normalize_selection(selection)

    @property
    def core_builder(self):
        raise NotImplementedError("Needs to be overloaded.")

    def n_elements_to_import(self):
        return len(self._selection.ranges)

    def process_range(self, range_):
        selection = libsonata.Selection(self._selection.ranges[slice(*range_)])
        syn_ids = selection.flatten()
        post_gids = self._sonata_edges.target_nodes(selection)
        pre_gids = self._sonata_edges.source_nodes(selection)

        synapse_centers = np.empty((syn_ids.shape[0], 3))
        for k, direction in enumerate(["x", "y", "z"]):
            key = f"afferent_center_{direction}"
            synapse_centers[:, k] = self._sonata_edges.get_attribute(key, selection)

        self._core_builder._add_synapses(syn_ids, post_gids, pre_gids, synapse_centers)

    @classmethod
    def from_sonata_file(cls, edge_filename, population_name, target_gids=None,
                         output_dir=None, **kw):
        """Creates a synapse index from a sonata edge file and population.

        Args:
            edge_filename: The Sonata edges filename
            population_name: The name of the population
            target_gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits.
                Note: For multi-indexes, this keyword argument only needs to be
                present on the constructor rank.
            output_dir: If not ``None`` the index will be stored in the folder
                ``output_dir``.
        """
        population_name = validated_sonata_edges_population(
            edge_filename, population_name
        )
        edges = open_sonata_edges(edge_filename, population_name)
        index = cls.from_sonata_tgids(
            edges, target_gids=target_gids, output_dir=output_dir, **kw
        )

        if output_dir is not None:
            cls._write_extended_meta_data_section(
                output_dir, edge_filename, population_name
            )

        return index

    @classmethod
    def _select_afferent_edges(cls, sonata_edges, target_gids, **kw):
        return sonata_edges.afferent_edges(target_gids)

    @classmethod
    def _make_sonata_selection(cls, sonata_edges, target_gids, **kw):
        if target_gids is not None:
            return cls._select_afferent_edges(sonata_edges, target_gids)

        else:
            return sonata_edges.select_all()

    @classmethod
    def from_sonata_tgids(cls, sonata_edges, target_gids, **kw):
        """Creates a synapse index from an edge file and a set of target GIDs."""
        selection = cls._make_sonata_selection(sonata_edges, target_gids)
        return cls.from_sonata_selection(sonata_edges, selection, **kw)

    @classmethod
    def from_sonata_selection(cls, sonata_edges, selection, **kw):
        """Builds the synapse index from a generic Sonata selection object.

        Any additional keyword arguments are passed on to ``cls.create``.
        """
        return cls.create(sonata_edges, selection, **kw)

    @classmethod
    def _normalize_selection(cls, selection):
        return chunk_sonata_selection(selection, cls.MAX_SYN_COUNT_RANGE)


class SynapseIndexBuilder(SynapseIndexBuilderBase,
                          _WriteSONATAMetadataMixin,
                          ChunkedProcessingMixin):
    """Builder for in-memory synapse indexes."""

    # Chunks are 1 Sonata range (of 100k synapses). Pick the value
    # set in `SynapseIndexBuilderBase` not `ChunkedProcessingMixin`.
    N_ELEMENTS_CHUNK = SynapseIndexBuilderBase.N_ELEMENTS_CHUNK

    def __init__(self, sonata_edges, selection):
        super().__init__(sonata_edges, selection)
        self._core_builder = core.SynapseIndex()

    @property
    def index(self):
        return SynapseIndex(self._core_builder, self._sonata_edges)

    @property
    def _index_if_loaded(self):
        return self.index

    def _write_index_if_needed(self, output_dir):
        if output_dir is not None:
            spatial_index.logger.info("Writing index to file: %s", output_dir)
            self._core_builder._dump(output_dir)


# Only provide MPI MultiIndex builders if enabled at the core
if hasattr(core, "SynapseMultiIndexBulkBuilder"):

    class SynapseMultiIndexBuilder(MultiIndexBuilderMixin,
                                   _WriteSONATAMetadataMultiMixin,
                                   SynapseIndexBuilderBase):
        """Builder for multi-index synapse indexes.

        Note: this requires MPI support. Guidance on choosing the number of
        MPI ranks can be found in the User Guide.
        """
        def __init__(self, sonata_edges, selection, output_dir=None):
            super().__init__(sonata_edges, selection)

            assert output_dir is not None, f"Invalid `output_dir`. [{output_dir}]"
            self._core_builder = core.SynapseMultiIndexBulkBuilder(output_dir)

        @classmethod
        def constructor_rank(cls, mpi_comm=None):
            """The MPI rank of the *constructor rank*.

            The *constructor rank* is the rank on which all argument to the
            constructor need to have valid values. Some keyword-arguments
            point to MB of data, e.g. ``target_gids``; those sometimes don't
            need to present on all MPI ranks. When a particular keyword argument
            is optional this is clearly stated in the API documentation stated in
            the API documentation.

            Please consult the User Guide for tips on using SI in an MPI parallel
            setting.
            """

            if mpi_comm is None:
                from mpi4py import MPI
                mpi_comm = MPI.COMM_WORLD

            return mpi_comm.Get_size() - 1

        @property
        def _index_if_loaded(self):
            return None

        def _write_index_if_needed(self, output_dir):
            pass

        @classmethod
        def _mpi_comm(cls):
            from mpi4py import MPI

            return MPI.COMM_WORLD

        @classmethod
        def _make_sonata_selection(cls, sonata_edges, target_gids, **kw):
            comm = cls._mpi_comm()
            mpi_rank = comm.Get_rank()
            root = cls.constructor_rank(mpi_comm=comm)

            if mpi_rank == root:
                selection = SynapseIndexBuilderBase._make_sonata_selection(
                    sonata_edges, target_gids
                )
            else:
                selection = None

            return bcast_sonata_selection(selection, root=root, mpi_comm=comm)

        @classmethod
        def _normalize_selection(cls, selection):
            comm = cls._mpi_comm()
            comm_size = comm.Get_size()

            chunk_size = cls.MAX_SYN_COUNT_RANGE
            while chunk_size >= 1:
                candidate_selection = chunk_sonata_selection(selection, chunk_size)
                if len(candidate_selection.ranges) >= comm_size - 1:
                    return candidate_selection

                chunk_size = chunk_size // 8

            raise ValueError("Unable to create a suitable selection.")
