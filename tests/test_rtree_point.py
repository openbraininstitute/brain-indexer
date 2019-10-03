import numpy as np

from spatial_index import SphereIndex as PointIndex

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
    print("Running tests with class " + PointIndex.__name__)
    t = PointIndex()
    t.add_points(points, ids)
    min_corner = [-1, -1, -1]
    max_corner = [1, 1, 1]
    idx = t.find_intersecting_window(min_corner, max_corner)
    expected_result = np.array([0, 1, 2, 6], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def test_init_points():
    p = PointIndex(points, is_point=True)  # or p = PointIndex(points, True)
    idx = p.find_nearest([5, 0, 0], 3)
    expected_result = np.array([0, 2, 6], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def test_init_points_with_ids():
    p = PointIndex(points, None, ids, True)
    # or p = PointIndex(points, None, is_point=True)
    idx = p.find_nearest([5, 0, 0], 3)
    expected_result = np.array([0, 2, 6], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def run_tests():
    test_add_points()
    print("[PASSED] Test test_add_points()")

    test_init_points()
    print("[PASSED] Test test_init_points()")

    test_init_points_with_ids()
    print("[PASSED] Test test_init_points_with_ids()")


if __name__ == "__main__":
    run_tests()
