# This file is part of SpatialIndex, the new-gen spatial builder for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import libsonata
import numpy

import spatial_index
from . import _spatial_index as core
from .util import gen_ranges
from .index import SynapseIndex
from .chunked_builder import ChunkedProcessingMixin, MultiIndexBuilderMixin


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
        synapse_centers = numpy.dstack((
            self._sonata_edges.get_attribute("afferent_center_x", selection),
            self._sonata_edges.get_attribute("afferent_center_y", selection),
            self._sonata_edges.get_attribute("afferent_center_z", selection)
        ))
        self._core_builder._add_synapses(syn_ids, post_gids, pre_gids, synapse_centers)

    @classmethod
    def from_sonata_file(cls, edge_filename, population_name, target_gids=None,
                         output_dir=None, **kw):
        """Creates a synapse index from a sonata edge file and population.

        Args:
            edge_filename: The Sonata edges filename
            population_name: The name of the population
            target_gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits
            output_dir: If not `None` the index will be stored in the folder `output_dir`.
        """
        edges = spatial_index.io.open_sonata_edges(edge_filename, population_name)
        index = cls.from_sonata_tgids(edges, target_gids, output_dir=output_dir, **kw)

        if output_dir is not None:
            cls._write_extended_meta_data_section(
                output_dir, edge_filename, population_name
            )

        return index

    @classmethod
    def from_sonata_tgids(cls, sonata_edges, target_gids=None, **kw):
        """Creates a synapse index from an edge file and a set of target GIDs."""
        selection = (
            sonata_edges.afferent_edges(target_gids) if target_gids is not None
            else sonata_edges.select_all()
        )
        return cls.from_sonata_selection(sonata_edges, selection, **kw)

    @classmethod
    def from_sonata_selection(cls, sonata_edges, selection, **kw):
        """Builds the synapse index from a generic Sonata selection object.

        Any additional keyword arguments are passed on to `cls.create`.
        """
        return cls.create(sonata_edges, selection, **kw)

    @classmethod
    def _normalize_selection(cls, selection):
        # Some selections may be extremely large. We split them so
        # memory overhead is smaller and progress can be monitored
        new_ranges = []
        for first, last in selection.ranges:
            count = last - first
            if count > cls.MAX_SYN_COUNT_RANGE:
                new_ranges.extend(list(gen_ranges(last, cls.MAX_SYN_COUNT_RANGE, first)))
            else:
                new_ranges.append((first, last))

        return libsonata.Selection(new_ranges)


class SynapseIndexBuilder(SynapseIndexBuilderBase, ChunkedProcessingMixin):
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

    def _write_extended_meta_data_section(*a, **kw):
        spatial_index.io.write_sonata_meta_data_section(*a, **kw)


# Only provide MPI MultiIndex builders if enabled at the core
if hasattr(core, "SynapseMultiIndexBulkBuilder"):

    class SynapseMultiIndexBuilder(MultiIndexBuilderMixin, SynapseIndexBuilderBase):
        """Builder for multi-index synapse indexes.

        Note: this requires MPI support. Guidance on choosing the number of
        MPI ranks can be found in the User Guide.
        """
        def __init__(self, sonata_edges, selection, output_dir=None):
            super().__init__(sonata_edges, selection)

            assert output_dir is not None, f"Invalid `output_dir`. [{output_dir}]"
            self._core_builder = core.SynapseMultiIndexBulkBuilder(output_dir)

        @property
        def _index_if_loaded(self):
            return None

        def _write_index_if_needed(self, output_dir):
            pass

        def _write_extended_meta_data_section(*a, **kw):
            from mpi4py import MPI

            if MPI.COMM_WORLD == 0:
                spatial_index.io.write_sonata_meta_data_section(*a, **kw)
