import numpy as np
from _spatial_index import MorphIndex
import importlib
import os.path

test_rtree_soma = importlib.import_module('test_rtree_soma',
                                          os.path.dirname(__file__))


def test_bulk_somas_add():
    N = 10
    ids = np.arange(5, 15, dtype=np.intp)
    centroids = np.zeros([N, 3], dtype=np.float32)
    centroids[:, 0] = np.arange(N)
    radius = np.ones(N, dtype=np.float32)

    rtree = MorphIndex()
    rtree.add_somas(ids, centroids, radius)

    idx = rtree.find_nearest(5, 0, 0, 3)
    assert sorted(idx['gid']) == [9, 10, 11]
    assert sorted(idx['segment_i']) == [0, 0, 0]


def test_bulk_neuron_add():
    """
    Adding a neuron alike:
    ( S ).=.=.=.=.=.=.=.=.
      0  1 2 3 4 5 6 7 8 9  <- x coord
      0   1 2 3 4 5 6 7 8   <- segment_i
    """
    N = 10
    nrn_id = 1
    points = np.zeros([N, 3], dtype=np.float32)
    points[:, 0] = np.arange(N)
    radius = np.ones(N, dtype=np.float32)

    rtree = MorphIndex()
    rtree.add_neuron(nrn_id, points, radius)

    idx = rtree.find_nearest(5, 0, 0, 4)
    assert sorted(idx['gid']) == [1, 1, 1, 1]
    assert sorted(idx['segment_i']) == [3, 4, 5, 6]



if __name__ == "__main__":
    test_rtree_soma.IndexClass = MorphIndex
    test_rtree_soma.run_tests()

    test_bulk_somas_add()
    print("[PASSED] test_bulk_somas_add")

    test_bulk_neuron_add()
    print("[PASSED] test_bulk_neuron_add")
