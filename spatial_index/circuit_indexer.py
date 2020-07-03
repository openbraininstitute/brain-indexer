#!/bin/env python
# ------------------------------------------------------
# copyright Blue Brain Project 2020. All rights reserved
# ------------------------------------------------------
"""
Implementation of MvdMorphIndexer which, builds a MorphologyIndex
and indexes all cells contained in an MVD/SONATA file
"""
import functools
import itertools
import logging
import morphio
import mvdtool
import warnings; warnings.simplefilter("ignore")
from collections import namedtuple
from os import path as ospath

import quaternion as npq
from spatial_index import MorphIndex

morphio.set_ignored_warning(morphio.Warning.only_child)


class MorphologyLib:
    MorphInfo = namedtuple("MorphInfo", "soma, points, radius, branch_offsets")

    def __init__(self, pth):
        self._pth = pth
        self._morphologies = {}

    def _load(self, morph_name):
        morph = morphio.Morphology(ospath.join(self._pth, morph_name) + ".asc")
        soma = morph.soma
        morph_infos = self.MorphInfo(
            (soma.center, soma.max_distance),
            morph.points,
            morph.diameters,
            morph.section_offsets,
        )
        self._morphologies[morph_name] = morph_infos
        return morph_infos

    def get(self, morph_name):
        return self._morphologies.get(morph_name) or self._load(morph_name)


class MvdMorphIndexer:
    def __init__(self, morphology_dir):
        self.morph_lib = MorphologyLib(morphology_dir)
        self.spatialindex = MorphIndex()
        self.mvd = mvdtool.open(FILENAME)  # type: mvdtool.MVD3.File

    def process_cell(self, gid, morph, position, rotation):
        morph = self.morph_lib.get(morph)
        points = (npq.rotate_vectors(npq.quaternion(*rotation).normalized(), morph.points)
                  if rotation is not None
                  else morph.points)
        points += position
        self.spatialindex.add_soma(gid, *morph.soma)
        self.spatialindex.add_neuron(
            gid, points, morph.radius, morph.branch_offsets[:-1], False
        )

    def process_range(self, range_):
        """Process a range of cells.

        :param: range_ (offset, count), or () [all]
        """
        gids = itertools.count(range_[0] if range_ else 0)
        mvd = self._mvd
        morph_names = mvd.morphologies(*range_)
        positions = mvd.positions(*range_)
        rotations = mvd.rotations(*range_) if mvd.rotated else itertools.repeat(None)

        for gid, morph, pos, rot in zip(gids, morph_names, positions, rotations):
            self.process_cell(gid, morph, pos, rot)


def gen_ranges(limit, blocklen):
    low_i = 0
    for high_i in range(blocklen, limit, blocklen):
        yield low_i, high_i - low_i
        low_i = high_i
    if low_i < limit:
        yield low_i, limit - low_i


# The unitary processing unit to run independently in each process
def build_index(morphology_dir, range_=()):
    indexer = MvdMorphIndexer(morphology_dir)
    indexer.process_range(range_)
    return indexer


def main_serial(morphology_dir):
    return build_index(morphology_dir)


def main_parallel(morphology_dir):
    import multiprocessing
    pool = multiprocessing.Pool()
    N_CELLS_RANGE = 100

    n_cells = len(mvdtool.open(FILENAME))
    nranges = int(n_cells / N_CELLS_RANGE)
    cur_i = 0
    build_index = functools.partial(build_index, morphology_dir)

    for _ in pool.imap_unordered(build_index, gen_ranges(n_cells, N_CELLS_RANGE)):
        cur_i += 1
        logging.info("Processed range %d / %d", cur_i, nranges)
