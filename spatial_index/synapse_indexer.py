# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

from . import _spatial_index
import numpy


class SynapseIndexer(_spatial_index.SphereIndex):
    """
    A Synapse spatial index
    Synapses are loaded from Sonata edge files as instantiated in a SphereIndex
    """

    def __init__(self, synapse_centers, synapse_ids, edges_object=None):
        """
        Inits a new Synapse indexer.

        For Sonata files consider using any of from_sonata_* helpers.

        Args:
            synapse_centers: The numpy arrays with the points indexes
            synapse_ids: The ids of the synapses
            edges_object: (optional) The edges object to query for extended data
        """
        self.edges = edges_object
        super().__init__(synapse_centers, None, synapse_ids)

    @classmethod
    def from_sonata_selection(cls, edges, selection):
        """ Builds the synapse index from a generic Sonata selection object
        """
        synapse_ids = selection.flatten()
        synapse_centers = numpy.dstack((
            edges.get_attribute("afferent_center_x", selection),
            edges.get_attribute("afferent_center_y", selection),
            edges.get_attribute("afferent_center_z", selection)
        ))
        return cls(synapse_centers, synapse_ids, edges)

    @classmethod
    def from_sonata_tgids(cls, edges, target_gids=None):
        """ Creates a synapse index from an edge file and a set of target gids
        """
        selection = (
            edges.afferent_edges(target_gids) if target_gids is not None
            else edges.select_all()
        )
        return cls.from_sonata_selection(edges, selection)

    @classmethod
    def from_sonata_file(cls, edge_filename,
                         population_name="default",
                         target_gids=None):
        """ Creates a synapse index from a sonata edge file and population.

        Args:
            edge_filename: The Sonata edges filename
            population_name: The name of the population. Default: 'default'
            target_gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits

        """
        import libsonata  # local, since this is just a helper, avoid cyclec dep
        storage = libsonata.EdgeStorage(edge_filename)
        edges = storage.open_population(population_name)
        return cls.from_sonata_tgids(edges, target_gids)
