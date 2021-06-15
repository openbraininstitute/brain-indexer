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
from numpy import roll
from spatial_index import MorphIndex

morphio.set_ignored_warning(morphio.Warning.only_child)
logging.basicConfig(level=logging.INFO)


N_CELLS_RANGE = 100
MorphInfo = namedtuple("MorphInfo", "soma, points, radius, branch_offsets")

class MorphologyLib:

    def __init__(self, pth):
        self._pth = pth
        self._morphologies = {}

    def _load(self, morph_name):
        if ospath.isfile(self._pth):
            morph = morphio.Morphology(self._pth) #h5
        elif ospath.isdir(self._pth):
            morph = morphio.Morphology(ospath.join(self._pth, morph_name) + ".asc") #asc
        else:
            raise Exception("Morphology file not found")

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


class MvdMorphIndexer:
    def __init__(self, morphology_dir, mvd_file):
        self.morph_lib = MorphologyLib(morphology_dir)
        self.spatialindex = MorphIndex()
        self.mvd = mvdtool.open(mvd_file)  # type: mvdtool.MVD3.File

    def rototranslate(self, morph, position, rotation):
        morph = self.morph_lib.get(morph)

        # mvd files use (x, y, z, w) representation for quaternions. npq uses (w, x, y, z)
        rotation_vector = roll(rotation, 1)
        points = (npq.rotate_vectors(npq.quaternion(*rotation_vector).normalized(), morph.points)
                  if rotation is not None
                  else morph.points)

        points += position
        return points

    def process_cell(self, gid, morph, points, position):
        morph = self.morph_lib.get(morph)
        soma_center, soma_rad = morph.soma
        soma_center += position
        self.spatialindex.add_soma(gid, soma_center, soma_rad)
        self.spatialindex.add_neuron(
            gid, points, morph.radius, morph.branch_offsets[:-1], False
        )

    def process_range(self, range_=()):
        """Process a range of cells.

        :param: range_ (offset, count), or () [all]
        """
        gids = itertools.count(range_[0] if range_ else 0)
        mvd = self.mvd
        morph_names = mvd.morphologies(*range_)
        positions = mvd.positions(*range_)
        rotations = mvd.rotations(*range_) if mvd.rotated else itertools.repeat(None)
        for gid, morph, pos, rot in zip(gids, morph_names, positions, rotations):
            # GIDs in files are zero-based, while they're typically 1-based in application.
            gid += 1
            rotopoints = self.rototranslate(morph, pos, rot)
            self.process_cell(gid, morph, rotopoints, pos)


def gen_ranges(limit, blocklen):
    low_i = 0
    for high_i in range(blocklen, limit, blocklen):
        yield low_i, high_i - low_i
        low_i = high_i
    if low_i < limit:
        yield low_i, limit - low_i


def _range_process(morphology_dir, mvd_file, part):
    # Instantiate indexer just once per process
    indexer = globals().get("indexer")
    if not indexer:
        indexer = globals()["indexer"] = MvdMorphIndexer(morphology_dir, mvd_file)
    indexer.process_range(part)


def main_serial(morphology_dir, mvd_file):
    indexer = MvdMorphIndexer(morphology_dir, mvd_file)
    n_cells = len(indexer.mvd)
    nranges = int(n_cells / N_CELLS_RANGE)

    for i, range_ in enumerate(gen_ranges(n_cells, N_CELLS_RANGE)):
        indexer.process_range(range_)
        logging.info("Processed %d%%", float(i+1) * 100 / nranges )
    return indexer


def main_parallel(morphology_dir, mvd_file):
    import multiprocessing
    pool = multiprocessing.Pool()

    indexer = globals()["indexer"] = MvdMorphIndexer(morphology_dir, mvd_file)
    n_cells = len(indexer.mvd)
    nranges = int(n_cells) / N_CELLS_RANGE
    build_index = functools.partial(_range_process, morphology_dir, mvd_file)
    ranges = gen_ranges(n_cells, N_CELLS_RANGE)

    for i, _ in enumerate(pool.imap_unordered(build_index, ranges)):
        logging.info("Processed %d%%", float(i+1) * 100 / nranges)
