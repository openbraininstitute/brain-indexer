"""
Python tests to the SpatialGrid

"""
import numpy as np
from spatial_index import SphereGrid
import os.path
import sys


def test_insert_check_size():
    c1 = SphereGrid()
    c1.insert(np.array([1, 1, 1], dtype='float'))
    c1.print()
    assert len(c1) == 1, "{} != {}".format(len(c1), 1)


def test_insert_array():
    c1 = SphereGrid()
    c1.insert(np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]], dtype='float'))
    c1.print()
    assert len(c1) == 3, "{} != {}".format(len(c1), 3)


def run_tests():
    test_insert_check_size()
    test_insert_array()


if __name__ == "__main__":
    run_tests()
