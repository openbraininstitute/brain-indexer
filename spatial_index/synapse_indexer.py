# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import libsonata
import logging
import numpy

from . import _spatial_index as core
from .util import ChunkedProcessingMixin, DiskMemMapProps, gen_ranges


class PointIndex(core.SphereIndex):
    """
    A spatial index for points. Can be used for synapses without any payload
    """

    def __new__(cls, synapse_centers, synapse_ids):
        """
        Inits a new Synapse indexer.

        Args:
            synapse_centers: The numpy arrays with the points indexes
            synapse_ids: The ids of the synapses
        """
        return core.SphereIndex(synapse_centers, None, synapse_ids)


class SynapseIndexBuilder(ChunkedProcessingMixin):

    DiskMemMapProps = DiskMemMapProps  # shortcut

    # Chunks are 1 Sonata range (of 100k synapses)
    N_ELEMENTS_CHUNK = 1  # override from ChunkedProcessingMixin
    MAX_SYN_COUNT_RANGE = 100_000

    def __init__(self, sonata_edges, selection, mem_map_props: DiskMemMapProps = None):
        if mem_map_props:
            self.index = core.SynapseIndexMemDisk.create(*mem_map_props.args)
        else:
            self.index = core.SynapseIndex()
        self.edges = sonata_edges
        self._selection = self.normalize_selection(selection)
        super().__init__()

    def n_elements_to_import(self):
        return len(self._selection.ranges)

    def process_range(self, range_):
        selection = libsonata.Selection(self._selection.ranges[slice(*range_)])
        syn_ids = selection.flatten()
        post_gids = self.edges.target_nodes(selection)
        pre_gids = self.edges.source_nodes(selection)
        synapse_centers = numpy.dstack((
            self.edges.get_attribute("afferent_center_x", selection),
            self.edges.get_attribute("afferent_center_y", selection),
            self.edges.get_attribute("afferent_center_z", selection)
        ))
        self.index.add_synapses(syn_ids, post_gids, pre_gids, synapse_centers)

    @classmethod
    def load_dump(cls, filename):
        """Load the index from a dump file"""
        return core.SynapseIndex(filename)

    @classmethod
    def from_sonata_selection(cls, sonata_edges, selection,
                              disk_mem_map: DiskMemMapProps = None, **kw):
        """ Builds the synapse index from a generic Sonata selection object.

        Any additional keyword arguments are passed on to `cls.create`.
        """
        return cls.create(sonata_edges, selection, disk_mem_map, **kw)

    @classmethod
    def from_sonata_tgids(cls, sonata_edges, target_gids=None,
                          disk_mem_map: DiskMemMapProps = None, **kw):
        """ Creates a synapse index from an edge file and a set of target gids
        """
        selection = (
            sonata_edges.afferent_edges(target_gids) if target_gids is not None
            else sonata_edges.select_all()
        )
        return cls.from_sonata_selection(sonata_edges, selection, disk_mem_map, **kw)

    @classmethod
    def from_sonata_file(cls, edge_filename, population_name, target_gids=None,
                         disk_mem_map: DiskMemMapProps = None, **kw):
        """ Creates a synapse index from a sonata edge file and population.

        Args:
            edge_filename: The Sonata edges filename
            population_name: The name of the population
            target_gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits
            disk_mem_map: Definitions to use a memory-mapped file instead
            return_indexer: If True, returns the full indexer object
                (inherited from ChunkedProcessingMixin)

        """
        storage = libsonata.EdgeStorage(edge_filename)
        if population_name is None:
            if len(storage.population_names) > 1:
                raise RuntimeError("No population chosen, multiple available")
            population_name = next(iter(storage.population_names), None)
            logging.info("Population not set. Auto-selecting '%s'", population_name)

        edges = storage.open_population(population_name)
        return cls.from_sonata_tgids(edges, target_gids, disk_mem_map, **kw)

    @classmethod
    def normalize_selection(cls, selection):
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

    @classmethod
    def load_disk_mem_map(_cls, filename):
        """Load the index from a memory mapped file"""
        return core.SynapseIndexMemDisk.open(filename)
