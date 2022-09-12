import numpy as np

from spatial_index import core

from test_rtree_sphere import _box_query_id

points = np.array(
    [
        [0.0, 1.0, 0],
        [-0.5, -0.5, 0],
        [0.5, -0.5, 0],
        [-2.1, 0.0, 0],
        [0.0, 2.1, 0],
        [0.0, 0.0, 2.1],
        [1., 1., 1.]
    ], dtype=np.float32)

ids = np.arange(len(points), dtype=np.intp)


def test_add_points():
    print("Running tests with class " + core.SphereIndex.__name__)
    t = core.SphereIndex()
    t._add_points(points, ids)
    min_corner = [-1, -1, -1]
    max_corner = [1, 1, 1]
    idx = _box_query_id(t, min_corner, max_corner)
    expected_result = np.array([0, 1, 2, 6], dtype=np.uintp)

    assert sorted(idx) == sorted(expected_result)


def test_init_points():
    p = core.SphereIndex(points, radii=None)  # or p = core.SphereIndex(points, None)
    idx = p._find_nearest([5, 0, 0], 3)
    expected_result = np.array([0, 2, 6], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def test_init_points_with_ids():
    p = core.SphereIndex(points, radii=None, py_ids=ids)
    # or p = core.SphereIndex(points, None, ids)
    idx = p._find_nearest([5, 0, 0], 3)
    expected_result = np.array([0, 2, 6], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def test_print_rtree():
    points = np.array(
        [
            [0.00001, 1.0, 0],
            [-0.5, -0.5456, 0],
            [0.5, -0.5, 0],
        ], dtype=np.float32)
    p = core.SphereIndex(points[0:3], radii=None)
    str_expect = (
        'IndexTree([\n'
        '  IShape(id=0, Sphere(centroid=[1e-05 1 0], radius=0))\n'
        '  IShape(id=1, Sphere(centroid=[-0.5 -0.546 0], radius=0))\n'
        '  IShape(id=2, Sphere(centroid=[0.5 -0.5 0], radius=0))\n'
        '])')
    str_result = str(p)
    assert str_result == str_expect
