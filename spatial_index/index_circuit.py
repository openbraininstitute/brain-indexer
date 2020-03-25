#!/bin/env python

import multiprocessing
import warnings; warnings.simplefilter("ignore")
from collections import namedtuple
from os import path as ospath

import itertools
import morphio
import mvdtool
import quaternion as npq
from mpi4py import MPI
from spatial_index import MorphIndex

morphio.set_ignored_warning(morphio.Warning.only_child)
comm = MPI.COMM_WORLD
rank = comm.Get_rank()

MORPHOLOGIES = "/Users/leite/dev/TestData/circuitBuilding_1000neurons/morphologies/ascii"
FILENAME = "/Users/leite/dev/TestData/circuitBuilding_1000neurons/circuits/circuit.mvd3"
N_CELLS_RANGE = 100


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
        self._morph_lib = MorphologyLib(morphology_dir)
        self._spatialindex = MorphIndex()
        self._mvd = mvdtool.open(FILENAME)  # type: mvdtool.MVD3.File

    def process_cell(self, gid, morph, position, rotation):
        morph = self._morph_lib.get(morph)
        points = (npq.rotate_vectors(npq.quaternion(*rotation).normalized(), morph.points)
                  if rotation is not None
                  else morph.points)
        points += position
        self._spatialindex.add_soma(gid, *morph.soma)
        self._spatialindex.add_neuron(
            gid, points, morph.radius, morph.branch_offsets[:-1], False
        )

    def process_range(self, low, count):
        gids = itertools.count(low)
        mvd = self._mvd
        morph_names = mvd.morphologies(low, count)
        positions = mvd.positions(low, count)
        rotations = mvd.rotations(low, count) if mvd.rotated else itertools.repeat(None)

        for gid, morph, pos, rot in zip(gids, morph_names, positions, rotations):
            self.process_cell(gid, morph, pos, rot)


def gen_ranges(limit, blocklen):
    low_i = 0
    for high_i in range(blocklen, limit, blocklen):
        yield low_i, high_i - low_i
        low_i = high_i
    if low_i < limit:
        yield low_i, limit - low_i


def build_index(_range):
    indexer = MvdMorphIndexer(MORPHOLOGIES)
    indexer.process_range(*_range)


def main():
    pool = multiprocessing.Pool()
    n_cells = len(mvdtool.open(FILENAME))
    nranges = int(n_cells / N_CELLS_RANGE)
    cur_i = 0
    for _ in pool.imap_unordered(build_index, gen_ranges(n_cells, N_CELLS_RANGE)):
        cur_i += 1
        print("Processed range {} / {}".format(cur_i, nranges))


if __name__ == "__main__":
    main()
