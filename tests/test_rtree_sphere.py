import numpy as np
from spatial_index import SphereIndex as IndexClass


def arange_centroids(N=10):
    centroids = np.zeros([N, 3], dtype=np.float32)
    centroids[:, 0] = np.arange(N, dtype=np.float32)
    return centroids


def test_insert_save_restore():
    print("Running tests with class " + IndexClass.__name__)
    centroids = arange_centroids(10)
    radii = np.full(10, 0.5, dtype=np.float32)

    t = IndexClass()
    for i in range(len(centroids)):
        t.insert(i, centroids[i], radii[i])

    for i, c in enumerate(centroids):
        idx = t.find_nearest(c, 1)[0]
        if len(idx.dtype) > 1:
            idx = idx['gid']  # Records
        assert idx == i, "{} != {}".format(idx, i)

    t.dump("mytree.save")
    t2 = IndexClass("mytree.save")
    for i, c in enumerate(centroids):
        idx = t2.find_nearest(c, 1)[0]
        if len(idx.dtype) > 1:
            idx = idx['gid']  # Records
        assert idx == i, "{} != {}".format(idx, i)


def test_bulk_spheres_add():
    N = 10
    ids = np.arange(5, 15, dtype=np.intp)
    centroids = np.zeros([N, 3], dtype=np.float32)
    centroids[:, 0] = np.arange(N)
    radius = np.ones(N, dtype=np.float32)

    rtree = IndexClass()
    rtree.add_spheres(centroids, radius, ids)

    idx = rtree.find_nearest([5, 0, 0], 3)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert sorted(idx) == [9, 10, 11]


def test_non_overlap_place():
    N = 3
    rtree = IndexClass()
    region = np.array([[0, 0, 0], [4, 1, 1]], dtype=np.float32)

    for i in range(N):
        assert rtree.place(region, i, [.0, .0, .0], 0.8) is True

    idx = rtree.find_nearest([5, 0, 0], N)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert sorted(idx) == list(range(N))


def test_is_intersecting():
    centroids = arange_centroids(3)
    radii = np.full(3, 0.2, dtype=np.float32)
    t = IndexClass(centroids, radii)
    for xpos in [-1, 0.5, 1.5, 2.5]:
        assert t.is_intersecting([xpos, 0, 0], 0.1) is False
    for xpos in [-0.2, -0.1, .0, 0.1, 1.2, 2.2]:
        assert t.is_intersecting([xpos, 0, 0], 0.1)


def test_intersection_none():
    centroids = np.array([[.0, 1., 0.],
                          [-0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.],
                          [0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.]], dtype=np.float32)
    radii = np.random.uniform(low=0.0, high=0.5, size=len(centroids)).astype(np.float32)

    t = IndexClass(centroids, radii)

    centroid = np.array([0., 0., 0.], dtype=np.float32)
    radius = np.random.uniform(low=0.0, high=0.49)

    idx = t.find_intersecting(centroid, radius)
    assert len(idx) == 0, "Should be empty, but {} were found instead.".format(idx)


def test_intersection_all():

    centroids = np.array([[.0, 1., 0.],
                          [-0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.],
                          [0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.]], dtype=np.float32)
    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)

    t = IndexClass(centroids, radii)

    centroid = np.array([0., 0., 0.], dtype=np.float32)
    radius = np.random.uniform(low=0.5, high=1.0)

    idx = t.find_intersecting(centroid, radius)
    objs = t.find_intersecting_objs(centroid, radius)

    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records

    expected_result = np.array([0, 1, 2], dtype=np.uintp)

    # New API: retrieve object references
    assert np.all(idx == expected_result), (idx, expected_result, centroids, radii)
    for obj, exp_gid in zip(objs, expected_result):
        assert obj.gid == exp_gid


def test_intersection_random():
    p1 = np.array([-10., -10., -10.], dtype=np.float32)
    p2 = np.array([10., 10., 10.], dtype=np.float32)

    N_spheres = 100

    centroids = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)
    radii = np.random.uniform(low=0.01, high=10., size=N_spheres).astype(np.float32)

    q_centroid = np.random.uniform(low=p1, high=p2).astype(np.float32)
    q_radius = np.float32(np.random.uniform(low=0.01, high=10.))

    distances = np.linalg.norm(centroids - q_centroid, axis=1)
    expected_result = np.where(distances <= radii + q_radius)[0]

    t = IndexClass(centroids, radii)

    idx = t.find_intersecting(q_centroid, q_radius)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert len(np.setdiff1d(idx, expected_result)) == 0, (idx, expected_result)


def test_intersection_window():

    centroids = np.array(
        [
            # Some inside / partially inside
            [0.0,  1.0, 0],
            [-0.5, -0.5, 0],
            [0.5, -0.5, 0],
            # Some outside
            [-2.1,  0.0, 0],
            [0.0,  2.1, 0],
            [0.0,  0.0, 2.1],
            # Another partially inside (double check)
            [1.2,  1.2, 1.2]
        ], dtype=np.float32)
    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)
    t = IndexClass(centroids, radii)

    min_corner = np.array([-1, -1, -1], dtype=np.float32)
    max_corner = np.array([1, 1, 1], dtype=np.float32)
    idx = t.find_intersecting_window(min_corner, max_corner)
    pos = t.find_intersecting_window_pos(min_corner, max_corner)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    expected_result = [0, 1, 2, 6]
    expected_pos = [
        [0.,    1.,  0.],
        [-0.5, -0.5, 0.],
        [0.5,  -0.5, 0.],
        [1.2,   1.2, 1.2]
    ]
    assert np.all(idx == expected_result), (idx, expected_result)
    assert np.allclose(pos, expected_pos)

    # NEW API
    for sphere in t.find_intersecting_window_objs(min_corner, max_corner):
        i = expected_result.index(sphere.gid)  # asserts gid is in the list
        assert np.allclose(sphere.centroid, expected_pos[i])


def test_bulk_spheres_points_add():
    rtree = IndexClass()
    # add Spheres
    centroids = np.array(
        [
            # Some inside / partially inside
            [0.0,  1.0, 0],
            [-0.5, -0.5, 0],
            [0.5, -0.5, 0],
            # Some outside
            [-2.1,  0.0, 0],
            [0.0,  2.1, 0],
            [0.0,  0.0, 2.1],
            # Another partially inside (double check)
            [1.2,  1.2, 1.2]
        ], dtype=np.float32)
    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)
    ids = np.arange(len(centroids), dtype=np.intp)
    rtree.add_spheres(centroids, radii, ids)

    # add Points
    points = np.array(
        [
            [0.5, -0.5,  1],
            [-1.0,  2.0, -1.1],
            [1.0,  1.0,  1.0],
            [0.0,  0.0,  0.0],
            [-1.0, -0.1,  1.1]
        ], dtype=np.float32)
    ids = np.arange(10, 10+len(points), dtype=np.intp)
    rtree.add_points(points, ids)

    # Query
    min_corner = [-1, -1, -1]
    max_corner = [1, 1, 1]
    idx = rtree.find_intersecting_window(min_corner, max_corner)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    expected_result = np.array([0, 1, 2, 6, 10, 12, 13], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result)


def test_nearest_all():
    centroids = arange_centroids()
    radii = np.ones(len(centroids), dtype=np.float32) * 0.01

    t = IndexClass(centroids, radii)

    center = np.array([0., 0., 0.], dtype=np.float32)
    idx = t.find_nearest(center, 10)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert np.all(np.sort(idx) == np.sort(np.arange(10, dtype=np.uintp))), idx


def _nearest_random():
    """
    We use the internal nearest() predicate, which uses the distance to the
    bouding box. As so results might not completely match but are good enough.
    In this test we get 100% match 80-90% of the times.
    """
    p1 = np.array([-10., -10., -10.], dtype=np.float32)
    p2 = np.array([10., 10., 10.], dtype=np.float32)

    N_spheres = 100
    K = 10

    centroids = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)
    radii = np.random.uniform(low=0.01, high=0.5, size=N_spheres).astype(np.float32)

    center = np.array([0., 0., 0.], dtype=np.float32)

    distances = np.linalg.norm(centroids, axis=1) - radii
    distances[distances < .0] = .0
    expected_result = np.argsort(distances)[:K]

    t = IndexClass(centroids, radii)

    idx = t.find_nearest(center, K)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert np.all(np.sort(idx) == np.sort(expected_result)), (idx, expected_result)


def test_nearest_random():
    # At east 1 in 3 must exactly match. See previous comment
    for i in range(3):
        try:
            _nearest_random()
            break
        except AssertionError:
            if i == 2:
                raise


def run_tests():
    test_insert_save_restore()
    print("[PASSED] Test test_insert_save_restore()")

    test_bulk_spheres_add()
    print("[PASSED] Test test_bulk_spheres_add()")

    test_bulk_spheres_points_add()
    print("[PASSED] Test test_bulk_spheres_points_add()")

    test_non_overlap_place()
    print("[PASSED] Test test_non_overlap_place()")

    test_is_intersecting()
    print("[PASSED] Test test_is_intersecting()")

    test_intersection_none()
    print("[PASSED] Test test_intersection_none()")

    test_intersection_all()
    print("[PASSED] Test test_intersection_all()")

    test_intersection_random()
    print("[PASSED] Test test_intersection_random()")

    test_intersection_window()
    print("[PASSED] Test test_intersection_window()")

    test_nearest_all()
    print("[PASSED] Test test_nearest_all()")

    test_nearest_random()
    print("[PASSED] Test test_nearest_random()")


if __name__ == "__main__":
    run_tests()
