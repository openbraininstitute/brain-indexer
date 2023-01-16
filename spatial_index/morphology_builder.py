# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved

import warnings; warnings.simplefilter("ignore")  # NOQA

from collections import namedtuple
from os import path as ospath

import morphio
import numpy as np
import quaternion as npq

import spatial_index
from . import _spatial_index as core
from . import logger

from .builder import _WriteSONATAMetadataMixin, _WriteSONATAMetadataMultiMixin
from .chunked_builder import ChunkedProcessingMixin, MultiIndexBuilderMixin
from .index import MorphIndex
from .io import open_sonata_nodes, validated_sonata_nodes_population_name


morphio.set_ignored_warning(morphio.Warning.only_child)
MorphInfo = namedtuple("MorphInfo", "soma, points, radius, branch_offsets, section_type")


class MorphologyLib:
    def __init__(self, pth):
        self._pth = pth
        self._morphologies = {}

    def _load(self, morph_name):
        if ospath.isfile(self._pth):
            morph = morphio.Morphology(self._pth)
        elif ospath.isdir(self._pth):
            morph = morphio.Morphology(self._guess_morph_filename(morph_name))
        else:
            raise Exception("Morphology path not found: " + self._pth)

        soma = morph.soma
        morph_infos = MorphInfo(
            soma=(soma.center, soma.max_distance),
            points=morph.points,
            radius=morph.diameters / 2.,
            branch_offsets=morph.section_offsets,
            section_type=morph.section_types,
        )
        self._morphologies[morph_name] = morph_infos
        return morph_infos

    def _guess_morph_filename(self, morph_name):
        extensions = [".asc", ".swc", ".h5"]

        for ext in extensions:
            filename = ospath.join(self._pth, morph_name) + ext
            if ospath.isfile(filename):
                return filename

        raise RuntimeError(
            f"Unable to guess morphology filename. {self._pth} {morph_name}"
        )

    def get(self, morph_name):
        return self._morphologies.get(morph_name) or self._load(morph_name)


class MorphIndexBuilderBase:
    def __init__(self, morphology_dir, nodes_file, population=None, gids=None):
        """Initializes a node index builder

        Args:
            morphology_dir (str): The file/directory where morphologies reside
            nodes_file (str): The SONATA nodes file
            population (str, optional): The nodes population. Defaults to the only
               population.
            gids ([type], optional): A selection of gids to index. Defaults to all GIDs.
        """
        population = validated_sonata_nodes_population_name(nodes_file, population)
        self._sonata_nodes = open_sonata_nodes(nodes_file, population)

        if gids is None:
            gids = range(0, self._sonata_nodes.size)
        else:
            # TODO maybe enforce 64bit unsigned ints.
            gids = np.sort(np.array(gids, dtype=int))

        self._gids = gids

        self.morph_lib = MorphologyLib(morphology_dir)
        spatial_index.logger.info("Number of neurons to index: %d", len(gids))

    def n_elements_to_import(self):
        return len(self._gids)

    def rototranslate(self, morph, position, rotation):
        # npq requires quaternion in the order: (w, x, y, z)

        morph = self.morph_lib.get(morph)
        if rotation is not None:
            points = npq.rotate_vectors(
                npq.quaternion(*rotation).normalized(),
                morph.points
            )

            points += position

        else:
            # Don't modify morphology-db points inplace, i.e. never `+=`.
            points = morph.points + position

        return points

    def process_cell(self, gid, morph, points, position):
        """ Process (index) a single cell
        """
        morph = self.morph_lib.get(morph)
        soma_center, soma_rad = morph.soma
        soma_center = soma_center + position  # Avoid +=
        self._core_builder._add_soma(gid, soma_center, soma_rad)
        self._core_builder._add_neuron(
            gid, points, morph.radius, morph.branch_offsets[:-1],
            morph.section_type, False
        )

    def process_range(self, sub_range=(None,)):
        """ Process a range of cells.

        :param: sub_range (start, end, [step]), or (None,) [all]
        """
        slice_ = slice(*sub_range)
        cur_gids = self._gids[slice_]

        sonata_nodes = self._sonata_nodes
        for gid in cur_gids:
            morph_name = sonata_nodes.get_attribute("morphology", gid)

            pos_keys = ["x", "y", "z"]
            pos = np.array(
                [sonata_nodes.get_attribute(key, gid) for key in pos_keys]
            )

            orientation_keys = [f"orientation_{key}" for key in ["w", "x", "y", "z"]]
            rot = np.array(
                [sonata_nodes.get_attribute(key, gid) for key in orientation_keys]
            )

            rotopoints = self.rototranslate(morph_name, pos, rot)
            self.process_cell(gid, morph_name, rotopoints, pos)

    @classmethod
    def from_sonata_file(cls, morphology_dir, node_filename, pop_name, gids=None,
                         output_dir=None, **kw):
        """ Creates a node index from a sonata node file.

        Args:
            node_filename: The SONATA node filename
            morphology_dir: The directory containing the morphology files
            pop_name: The name of the population
            gids: A list/array of target gids to index. Default: None
                Warn: None will index all synapses, please mind memory limits
            output_dir: If not ``None`` the index will be stored in the folder
                ``output_dir``.
        """
        if "target_gids" in kw:
            logger.warning(
                "The keyword argument 'target_gids' has been renamed to"
                " 'gids' and will be removed before 1.0."
            )

            if gids is not None:
                raise ValueError("Incompatible values for target_gids and gids.")

            gids = kw["target_gids"]
            del kw["target_gids"]

        index = cls.create(morphology_dir, node_filename, pop_name, gids,
                           output_dir=output_dir, **kw)

        if output_dir is not None:
            cls._write_extended_meta_data_section(
                output_dir, node_filename, pop_name
            )

        return index

    @classmethod
    def from_sonata_selection(cls, morphology_dir, node_filename, pop_name,
                              selection, output_dir=None, **kw):
        """ Builds the synapse index from a generic Sonata selection object"""
        index = cls.create(morphology_dir, node_filename, pop_name,
                           selection.flatten(), output_dir=output_dir, **kw)

        if output_dir is not None:
            cls._write_extended_meta_data_section(
                output_dir, node_filename, pop_name
            )

        return index


class MorphIndexBuilder(MorphIndexBuilderBase,
                        _WriteSONATAMetadataMixin,
                        ChunkedProcessingMixin):
    """A MorphIndexBuilder is a helper class to create a `MorphIndex`
    from a SONATA nodes file and a morphology library.
    """
    def __init__(self, morphology_dir, nodes_file, population=None, gids=None):
        super().__init__(morphology_dir, nodes_file, population, gids)
        self._core_builder = core.MorphIndexBulkBuilder()
        self._warn_when_too_large()

    def _warn_when_too_large(self):
        n_gids = len(self._gids)
        if n_gids > 100_000:
            logger.warning(
                f"Attempting to create an in-memory index with {n_gids} GIDs. Likely,"
                " it would be better to use a multi-index."
            )

    @property
    def _index_if_loaded(self):
        return self.index

    @property
    def index(self):
        return MorphIndex(self._core_index, self._sonata_nodes)

    @property
    def _core_index(self):
        return self._core_builder._index()

    def _write_index_if_needed(self, output_dir):
        if output_dir is not None:
            spatial_index.logger.info("Writing index to file: %s", output_dir)
            self._core_index._dump(output_dir)


# Only provide MPI MultiIndex builders if enabled at the core
if hasattr(core, "MorphMultiIndexBulkBuilder"):

    class MorphMultiIndexBuilder(MultiIndexBuilderMixin,
                                 _WriteSONATAMetadataMultiMixin,
                                 MorphIndexBuilderBase):

        def __init__(self, morphology_dir, nodes_file, population=None, gids=None,
                     output_dir=None):
            super().__init__(morphology_dir, nodes_file, population=population, gids=gids)

            assert output_dir is not None, f"Invalid `output_dir`. [{output_dir}]"
            self._core_builder = core.MorphMultiIndexBulkBuilder(output_dir)

        @property
        def _index_if_loaded(self):
            return None

        def _write_index_if_needed(self, output_dir):
            pass
