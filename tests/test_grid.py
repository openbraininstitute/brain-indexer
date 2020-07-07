"""
Python tests to the SpatialGrid

"""
import numpy as np
from spatial_index import SphereGrid


def test_insert_check_size():
    c1 = SphereGrid()
    # Spheres take 4 floats: (x,y,z) and radius
    c1.insert(np.array([1, 1, 1, 0.5], dtype='float'), [1])
    print(c1)
    assert len(c1) == 1, "{} != {}".format(len(c1), 1)


def test_insert_array():
    c1 = SphereGrid()
    c1.insert(np.array([[1, 1, 1, 0.5],
                        [2, 2, 2, 0.5],
                        [3, 3, 3, 0.5]], dtype='float'),
              [1, 2, 3])
    print(c1)
    assert len(c1) == 3, "{} != {}".format(len(c1), 3)
    return c1


def test_serialization():
    import pickle  # only py3
    c1 = test_insert_array()
    buf = pickle.dumps(c1)
    c2 = pickle.loads(buf)
    assert len(c2) == 3, "{} != {}".format(len(c2), 3)
    assert c1 == c2


def test_iadd():
    c1 = SphereGrid()
    c1.insert(np.array([[1, 1, 1, 0.5],
                        [6, 6, 6, 0.5]], dtype='float'),
              [1, 2])
    c2 = SphereGrid()
    c2.insert(np.array([[2, 2, 2, 0.5],
                        [11, 11, 11, 0.5]], dtype='float'),
              [3, 4])
    c1 += c2
    print(c1)


def run_tests():
    test_insert_check_size()
    test_insert_array()
    test_serialization()
    test_iadd()


if __name__ == "__main__":
    run_tests()
