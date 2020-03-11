import itertools
import mvdtool
import warnings; warnings.simplefilter("ignore")
import quaternion as npq
from morphio import Morphology
from os import path as ospath
from mpi4py import MPI
from spatial_index import MorphIndex
from collections import namedtuple

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

MORPHOLOGIES = "/Users/leite/dev/TestData/circuitBuilding_1000neurons/morphologies/ascii"
FILENAME = "/Users/leite/dev/TestData/circuitBuilding_1000neurons/circuits/circuit.mvd3"
mvd = mvdtool.open(FILENAME)  # type: mvdtool.MVD3.File


class MorphologyLib:
    MorphInfo = namedtuple("MorphInfo", "soma, points, radius, branch_offsets")

    def __init__(self, pth):
        self._pth = pth
        self._morphologies = {}

    def _load(self, morph_name):
        print("Loading morphology")
        morph = Morphology(ospath.join(self._pth, morph_name) + ".asc")
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

    def process_cell(self, gid, morph, position, rotation):
        # print("Processing gid", gid)
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
        print("Processing neurons in range", low, "->", low + count)
        gids = itertools.count(low)
        morph_names = mvd.morphologies(low, count)
        positions = mvd.positions(low, count)
        rotations = mvd.rotations(low, count) if mvd.rotated else itertools.repeat(None)
        for gid, morph, pos, rot in zip(gids, morph_names, positions, rotations):
            self.process_cell(gid, morph, pos, rot)


def gen_ranges(size, blocklen):
    low_i = 0
    for high_i in range(blocklen, size, blocklen):
        yield low_i, high_i
        low_i = high_i
    if low_i < size:
        yield low_i, size


def main():
    n_cells = len(mvdtool.open(FILENAME))
    for low_i, high_i in gen_ranges(n_cells, 100):
        indexer = MvdMorphIndexer(MORPHOLOGIES)
        indexer.process_range(low_i, high_i - low_i)


if __name__ == "__main__":
    main()
