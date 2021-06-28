# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

from . import _spatial_index
import libsonata
import numpy


class SynapseIndexer(_spatial_index.SphereIndex):
    """
    A Synapse spatial index
    Synapses are loaded from Sonata edge files as instantiated in a SphereIndex
    """

    def __init__(self, sonata_edges, target_gids, edge_population="default"):
        """
        Inits a new Synapse indexer.

        Args:
            sonata_edges: The Sonata edge file
            target_gids: the gids of the cells whose synapses are to be indexed
                Note: To force indexing all synapses pass 'None'
            edge_population: The name of the edge population (default: "default")
        """
        storage = libsonata.EdgeStorage(sonata_edges)
        self.edges = edges = storage.open_population(edge_population)
        selection = (edges.afferent_edges(target_gids)
                     if target_gids is not None
                     else edges.select_all())
        synapse_ids = selection.flatten()
        synapse_centers = numpy.dstack((
            edges.get_attribute("afferent_center_x", selection),
            edges.get_attribute("afferent_center_y", selection),
            edges.get_attribute("afferent_center_z", selection)
        ))
        super().__init__(synapse_centers, None, synapse_ids)
