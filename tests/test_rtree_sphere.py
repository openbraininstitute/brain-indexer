import tempfile

import numpy as np

import spatial_index
from spatial_index import core, SphereIndexBuilder

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


def _box_query_id(core_index, *box):
    return _any_query_id(core_index, box, "box_query")


def _sphere_query_id(core_index, *sphere):
    return _any_query_id(core_index, sphere, "sphere_query")


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
        t._insert(i, centroids[i], radii[i])

    for i, c in enumerate(centroids):
        idx = t._find_nearest(c, 1)[0]
        if len(idx.dtype) > 1:
            idx = idx['gid']  # Records
        assert idx == i, "{} != {}".format(idx, i)

    with tempfile.TemporaryDirectory(prefix="mytree.save", dir=".") as index_path:
        t._dump(index_path)
        t2 = IndexClass(index_path)
        for i, c in enumerate(centroids):
            idx = t2._find_nearest(c, 1)[0]
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
    rtree._add_spheres(centroids, radius, ids)

    idx = rtree._find_nearest([5, 0, 0], 3)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert sorted(idx) == [9, 10, 11]


def test_non_overlap_place():
    N = 3
    rtree = IndexClass()
    region = np.array([[0, 0, 0], [4, 1, 1]], dtype=np.float32)

    for i in range(N):
        assert rtree._place(region, i, [.0, .0, .0], 0.8) is True

    idx = rtree._find_nearest([5, 0, 0], N)
    if len(idx.dtype) > 1:
        idx = idx['gid']  # Records
    assert sorted(idx) == list(range(N))


def arange_sphere_index(n_spheres, radius):
    centroids = arange_centroids(n_spheres)
    radii = np.full(n_spheres, radius, dtype=np.float32)
    ids = np.arange(n_spheres)

    return SphereIndexBuilder.from_numpy(centroids, radii, ids)


def check_intersection(make_query_shape, empty_name, query_name):
    index = arange_sphere_index(n_spheres=3, radius=0.2)
    empty = getattr(index, empty_name)
    query = getattr(index, query_name)

    for xpos in [-1, 0.5, 1.5, 2.5]:
        query_shape = make_query_shape(xpos)

        assert empty(*query_shape) is True

        objs = query(*query_shape, fields="raw_elements", accuracy="best_effort")
        assert len(objs) == 0, f"Should be empty. Found: {objs}"

        results = query(*query_shape, accuracy="best_effort")
        for k, x in results.items():
            if k == "endpoints":
                p1, p2 = x
                assert p1.size == 0, f"Should be empty. Found: {p1}"
                assert p2.size == 0, f"Should be empty. Found: {p2}"
            else:
                assert len(x) == 0, f"Should be empty. Found: {x}"

    expected_ids = [0, 0, 0, 0, 1, 2]
    for xpos, expected_id in zip([-0.2, -0.1, .0, 0.1, 1.2, 2.2], expected_ids):
        query_shape = make_query_shape(xpos)

        assert empty(*query_shape) is False

        objs = query(*query_shape, fields="raw_elements", accuracy="best_effort")
        assert len(objs) == 1, f"Expected one element. Found: {objs}"

        actual_ids = query(*query_shape, fields="id", accuracy="best_effort")
        assert actual_ids.size == 1, f"Expected one id found: {actual_ids}"
        assert actual_ids[0] == expected_id


def test_intersection_sphere():
    def make_query_shape(x):
        return [x, 0.0, 0.0], 0.1

    check_intersection(make_query_shape, "sphere_empty", "sphere_query")


def test_intersection_box():
    def make_query_shape(x):
        return [x - 0.1, -0.1, -0.1], [x + 0.1, 0.1, 0.1]

    check_intersection(make_query_shape, "box_empty", "box_query")


def test_intersection_random():
    p1 = np.array([-10., -10., -10.], dtype=np.float32)
    p2 = np.array([10., 10., 10.], dtype=np.float32)

    N_spheres = 100

    centroids = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)
    radii = np.random.uniform(low=0.01, high=10., size=N_spheres).astype(np.float32)
    ids = np.arange(centroids.shape[0])

    index = SphereIndexBuilder.from_numpy(centroids, radii, ids)

    n_rep = 10
    for _ in range(n_rep):
        q_centroid = np.random.uniform(low=p1, high=p2, size=3).astype(np.float32)
        q_radius = np.float32(np.random.uniform(low=0.01, high=10.))

        distances = np.linalg.norm(centroids - q_centroid, axis=1)
        expected_result = np.where(distances <= radii + q_radius)[0]

        idx = index.sphere_query(q_centroid, q_radius, fields="id")
        objs = index.sphere_query(q_centroid, q_radius, fields="raw_elements")

        assert len(idx) == len(objs)
        assert sorted(idx) == sorted(expected_result)
        assert sorted([o.id for o in objs]) == sorted(expected_result)


def test_intersection_window():
    n_spheres, n_points = 7, 5
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
            [1.2,  1.2, 1.2],

            # Degenerate spheres, aka points
            [0.5, -0.5,  1],
            [-1.0, 2.0, -1.1],
            [1.0,  1.0,  1.0],
            [0.0,  0.0,  0.0],
            [-1.0, -0.1, 1.1]
        ],
        dtype=np.float32
    )

    radii = np.random.uniform(low=0.5, high=1.0, size=n_spheres).astype(np.float32)
    radii = np.hstack((radii, np.array(n_points * [0.0])))

    ids = np.hstack((np.arange(n_spheres), 10 + np.arange(n_points)))

    min_corner = np.array([-1, -1, -1], dtype=np.float32)
    max_corner = np.array([1, 1, 1], dtype=np.float32)

    expected_result = [0, 1, 2, 6, 10, 12, 13]

    assert centroids.shape[0] == n_spheres + n_points
    assert radii.shape[0] == n_spheres + n_points
    assert ids.shape[0] == n_spheres + n_points

    index = SphereIndexBuilder.from_numpy(centroids, radii, ids)
    idx = index.box_query(min_corner, max_corner, fields="id")

    assert sorted(idx) == sorted(expected_result), (idx, expected_result)


def test_nearest_all():
    centroids = arange_centroids()
    radii = np.ones(len(centroids), dtype=np.float32) * 0.01
    ids = np.arange(centroids.shape[0])

    index = SphereIndexBuilder.from_numpy(centroids, radii, ids)
    core_index = index._core_index

    center = np.array([0., 0., 0.], dtype=np.float32)
    idx = core_index._find_nearest(center, 10)
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
    ids = np.arange(centroids.shape[0])

    center = np.array([0., 0., 0.], dtype=np.float32)

    closest_point = np.minimum(
        np.maximum(center, centroids - radii[:, np.newaxis]),
        centroids + radii[:, np.newaxis]
    )
    distances = np.linalg.norm(closest_point - center, axis=1)
    expected_result = np.argsort(distances)[:K]

    index = SphereIndexBuilder.from_numpy(centroids, radii, ids)
    core_index = index._core_index

    idx = core_index._find_nearest(center, K)
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
