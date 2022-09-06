import tempfile

import numpy as np

import spatial_index
from spatial_index import core

IndexClass = core.SphereIndex


def _any_query_id(core_index, query_shape, query_name):
    if isinstance(core_index, core.MorphIndex):
        index = spatial_index.MorphIndex(core_index)
        field = "gid"
    elif isinstance(core_index, core.SphereIndex):
        index = spatial_index.SphereIndex(core_index)
        field = "id"
    else:
        raise RuntimeError("Broken test logic.")

    return getattr(index, query_name)(
        *query_shape,
        accuracy="best_effort",
        fields=field
    )


def _window_query_id(core_index, *box):
    return _any_query_id(core_index, box, "window_query")


def _vicinity_query_id(core_index, *sphere):
    return _any_query_id(core_index, sphere, "vicinity_query")


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

    with tempfile.TemporaryDirectory(prefix="mytree.save", dir=".") as index_path:
        t.dump(index_path)
        t2 = IndexClass(index_path)
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

    idx = t.find_intersecting_objs(centroid, radius, geometry="best_effort")
    assert len(idx) == 0, "Should be empty, but {} were found instead.".format(idx)

    d = t.find_intersecting_np(centroid, radius, geometry="best_effort")
    for x in d.values():
        assert len(x) == 0, "Should be empty, but {} were found instead.".format(x)


def test_intersection_all():

    centroids = np.array([[.0, 1., 0.],
                          [-0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.],
                          [0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.]], dtype=np.float32)
    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)

    # When IndexClass is a MorphIndex then init with centroids will create somas
    t = IndexClass(centroids, radii)

    centroid = np.array([0., 0., 0.], dtype=np.float32)
    radius = np.random.uniform(low=0.5, high=1.0)

    ids = t.find_intersecting_np(centroid, radius, geometry="best_effort")
    objs = t.find_intersecting_objs(centroid, radius, geometry="best_effort")
    expected_result = list(range(3))

    # New API: retrieve object references
    use_gid_field = IndexClass is core.MorphIndex  # MorphIndex raw ids are meaningless
    assert sorted(obj.gid if use_gid_field else obj.id for obj in objs) == expected_result

    field = "gid" if use_gid_field else "id"
    assert sorted(ids[field]) == expected_result


def test_intersection_random():
    p1 = np.array([-10., -10., -10.], dtype=np.float32)
    p2 = np.array([10., 10., 10.], dtype=np.float32)

    N_spheres = 100

    centroids = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)
    radii = np.random.uniform(low=0.01, high=10., size=N_spheres).astype(np.float32)

    q_centroid = np.random.uniform(low=p1, high=p2, size=3).astype(np.float32)
    q_radius = np.float32(np.random.uniform(low=0.01, high=10.))

    distances = np.linalg.norm(centroids - q_centroid, axis=1)
    expected_result = np.where(distances <= radii + q_radius)[0]

    t = IndexClass(centroids, radii)

    idx = _vicinity_query_id(t, q_centroid, q_radius)
    objs = t.find_intersecting_objs(q_centroid, q_radius, geometry="best_effort")

    assert len(idx) == len(objs)
    assert sorted(idx) == sorted(expected_result)


def test_intersection_window():
    centroids = np.array(
        [
            # Some inside / partially inside
            [0.0,  1.0, 0],
            [-0.5, -0.5, 0],
            [0.5, -0.5, 0],
            # Some outside
            [-2.1, 0.0, 0],
            [0.0,  2.1, 0],
            [0.0,  0.0, 2.1],
            # Another partially inside (double check)
            [1.2,  1.2, 1.2]
        ], dtype=np.float32)
    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)
    t = IndexClass(centroids, radii)

    min_corner = np.array([-1, -1, -1], dtype=np.float32)
    max_corner = np.array([1, 1, 1], dtype=np.float32)
    idx = _window_query_id(t, min_corner, max_corner)

    expected_result = [0, 1, 2, 6]
    assert sorted(idx) == sorted(expected_result), (idx, expected_result)


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
            [-1.0, 2.0, -1.1],
            [1.0,  1.0,  1.0],
            [0.0,  0.0,  0.0],
            [-1.0, -0.1, 1.1]
        ], dtype=np.float32)
    ids = np.arange(10, 10 + len(points), dtype=np.intp)
    rtree.add_points(points, ids)

    # Query
    min_corner = [-1, -1, -1]
    max_corner = [1, 1, 1]

    idx = _window_query_id(rtree, min_corner, max_corner)
    expected_result = np.array([0, 1, 2, 6, 10, 12, 13], dtype=np.uintp)

    assert sorted(idx) == sorted(expected_result)


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
    bounding box. Due to floating point arithmetic this test can fail, rarely.
    """
    p1 = np.array([-10., -10., -10.], dtype=np.float32)
    p2 = np.array([10., 10., 10.], dtype=np.float32)

    N_spheres = 100
    K = 10

    centroids = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)
    radii = np.random.uniform(low=0.01, high=0.5, size=N_spheres).astype(np.float32)

    center = np.array([0., 0., 0.], dtype=np.float32)

    closest_point = np.minimum(
        np.maximum(center, centroids - radii[:, np.newaxis]),
        centroids + radii[:, np.newaxis]
    )
    distances = np.linalg.norm(closest_point - center, axis=1)
    expected_result = np.argsort(distances)[:K]

    t = IndexClass(centroids, radii)

    idx = t.find_nearest(center, K)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert np.all(np.sort(idx) == np.sort(expected_result)), (idx, expected_result)


def test_nearest_random():
    # The test `_nearest_random` can fail due to floating point
    # issue. This should happen very rarely. Hence 2 out of 3
    # should always succeed.
    n_success = 0

    for _ in range(3):
        try:
            _nearest_random()
            n_success += 1
        except AssertionError:
            pass

    assert n_success >= 2
