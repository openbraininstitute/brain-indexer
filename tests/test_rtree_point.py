import pytest

import numpy as np

from spatial_index import core
from spatial_index import SphereIndexBuilder


@pytest.fixture
def points():
    return np.array(
        [
            [1e-5, 1.0, 0],
            [-0.5, -0.5456, 0],
            [0.5, -0.5, 0],
            [-2.1, 0.0, 0],
            [0.0, 2.1, 0],
            [0.0, 0.0, 2.1],
            [1., 1., 1.],
        ],
        dtype=np.float32,
    )


@pytest.fixture
def ids(points):
    return np.arange(len(points), dtype=np.intp)


@pytest.fixture
def zero_radii(points):
    return np.zeros(points.shape[0], dtype=np.float32)


def test_init_points_with_ids(points, zero_radii, ids):
    index = SphereIndexBuilder.from_numpy(points, zero_radii, ids)

    box = [-1, -1, -1], [1, 1, 1]
    actual_result = index.box_query(*box, fields="id")
    expected_result = np.array([0, 1, 2, 6], dtype=np.uintp)

    assert sorted(actual_result) == sorted(expected_result)


def test_print_rtree(points, zero_radii, ids):
    p = core.SphereIndex(points[:3, :], zero_radii[:3], ids[:3])
    str_expect = (
        'IndexTree([\n'
        '  IShape(id=0, Sphere(centroid=[1e-05 1 0], radius=0))\n'
        '  IShape(id=1, Sphere(centroid=[-0.5 -0.546 0], radius=0))\n'
        '  IShape(id=2, Sphere(centroid=[0.5 -0.5 0], radius=0))\n'
        '])')
    str_result = str(p)
    assert str_result == str_expect
