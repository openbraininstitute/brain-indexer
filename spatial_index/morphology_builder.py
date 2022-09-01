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

from collections import namedtuple
from os import path as ospath

import morphio
import mvdtool
import numpy as np
import quaternion as npq

import spatial_index
from . import _spatial_index as core
from .index_common import DiskMemMapProps  # noqa
from .chunked_builder import ChunkedProcessingMixin, MultiIndexBuilderMixin
from .index import MorphIndex, MorphIndexMemDisk

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
            morph = morphio.Morphology(self._guess_morph_filename(morph_name))
        else:
            raise Exception("Morphology path not found: " + self._pth)

        soma = morph.soma
        morph_infos = MorphInfo(
            soma=(soma.center, soma.max_distance),
            points=morph.points,
            radius=morph.diameters / 2.,
            branch_offsets=morph.section_offsets,
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
    def __init__(self, morphology_dir, nodes_file, population="", gids=None):
        """Initializes a node index builder

        Args:
            morphology_dir (str): The file/directory where morphologies reside
            nodes_file (str): The Sonata/mvd nodes file
            population (str, optional): The nodes population. Defaults to "" (default).
            gids ([type], optional): A selection of gids to index. Defaults to None (All)
        """
        mvd = mvdtool.open(nodes_file, population)

        if gids is None:
            gids = range(0, len(mvd))
        else:
            gids = np.sort(np.array(gids, dtype=int))

        self._mvd = mvd
        self._gids = gids

        self.morph_lib = MorphologyLib(morphology_dir)
        spatial_index.logger.info("Index count: %d cells", len(gids))

    def n_elements_to_import(self):
        return len(self._gids)

    def rototranslate(self, morph, position, rotation):
        morph = self.morph_lib.get(morph)

        # mvd files use (x, y, z, w) representation for quaternions. npq uses (w, x, y, z)
        rotation_vector = np.roll(rotation, 1)

        if rotation is not None:
            points = npq.rotate_vectors(
                npq.quaternion(*rotation_vector).normalized(),
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
        self._core_builder.add_soma(gid, soma_center, soma_rad)
        self._core_builder.add_neuron(
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

        mvd = self._mvd
        morph_names = mvd.morphologies(*index_args)
        positions = mvd.positions(*index_args)
        rotations = mvd.rotations(*index_args) if mvd.rotated else itertools.repeat(None)

        for gid, morph, pos, rot in zip(cur_gids, morph_names, positions, rotations):
            # GIDs in files are zero-based, while they're typically 1-based in application
            gid += 1
            rotopoints = self.rototranslate(morph, pos, rot)
            self.process_cell(gid, morph, rotopoints, pos)

    @classmethod
    def from_sonata_file(cls, morphology_dir, node_filename, pop_name, target_gids=None,
                         index_kind=None, **kw):
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

    @classmethod
    def from_mvd_file(cls, morphology_dir, node_filename, target_gids=None,
                      index_kind=None, **kw):
        """ Build a synpase index from an mvd file"""
        return cls.create(morphology_dir, node_filename, "", target_gids, **kw)

    @classmethod
    def from_sonata_selection(cls, morphology_dir, node_filename, pop_name,
                              selection, index_kind=None, **kw):
        """ Builds the synapse index from a generic Sonata selection object"""
        return cls.create(morphology_dir, node_filename, pop_name,
                          selection.flatten(), **kw)


class MorphIndexBuilder(MorphIndexBuilderBase, ChunkedProcessingMixin):
    """A MorphIndexBuilder is a helper class to create a `MorphIndex`
    from a node file (mvd or Sonata) and a morphology library.
    """
    def __init__(self, morphology_dir, nodes_file, population="", gids=None):
        super().__init__(morphology_dir, nodes_file, population, gids)
        self._core_builder = core.MorphIndex()

    @property
    def _index_if_loaded(self):
        return self.index

    @property
    def index(self):
        return MorphIndex(self._core_builder)

    def _write_index_if_needed(self, output_dir):
        if output_dir is not None:
            spatial_index.logger.info("Writing index to file: %s", output_dir)
            self._core_builder.dump(output_dir)


class MorphIndexMemDiskBuilder(MorphIndexBuilderBase, ChunkedProcessingMixin):
    """A MorphIndexBuilder is a helper class to create a `MorphIndexDiskMemMap`
    from a node file (mvd or Sonata) and a morphology library.
    """
    def __init__(self, morphology_dir, nodes_file, population="", gids=None,
                 disk_mem_map=None):
        super().__init__(morphology_dir, nodes_file, population, gids)

        assert disk_mem_map is not None
        self._core_builder = core.MorphIndexMemDisk.create(*disk_mem_map.args)

    @property
    def _index_if_loaded(self):
        return self.index

    @property
    def index(self):
        return MorphIndexMemDisk(self._core_builder)

    def _write_index_if_needed(self, output_dir):
        pass


# Only provide MPI MultiIndex builders if enabled at the core
if hasattr(core, "MorphMultiIndexBulkBuilder"):

    class MorphMultiIndexBuilder(MultiIndexBuilderMixin, MorphIndexBuilderBase):
        def __init__(self, morphology_dir, nodes_file, population="", gids=None,
                     output_dir=None):
            super().__init__(morphology_dir, nodes_file, population=population, gids=gids)

            assert output_dir is not None, f"Invalid `output_dir`. [{output_dir}]"
            self._core_builder = core.MorphMultiIndexBulkBuilder(output_dir)

        @property
        def _index_if_loaded(self):
            return None

        def _write_index_if_needed(self, output_dir):
            pass
