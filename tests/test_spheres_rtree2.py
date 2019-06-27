import pytest
import numpy as np
from _spatial_index import SomaTree


def arange_values():

    radii = np.arange(10).astype(np.float32)
    centroids = np.column_stack((radii, radii, radii)).astype(np.float32)
    return centroids, radii


@pytest.fixture
def arange_rtree():

    centroids, radii = arange_values()
    return SomaTree(centroids, radii)


def test_insert():

    centroids, radii = arange_values()

    t = SomaTree()

    for i in range(len(centroids)):
        t.insert(i, centroids[i, 0], centroids[i, 1], centroids[i, 2], radii[i])


def test_intersection_none():

    centroids = np.array([[.0,  1.,  0.],
                         [-0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.],
                         [ 0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.]]).astype(np.float32)


    radii = np.random.uniform(low=0.0, high=0.5, size=len(centroids)).astype(np.float32)

    t = SomaTree(centroids, radii)

    centroid = np.array([0., 0., 0.]).astype(np.float32)

    radius = np.random.uniform(low=0.0, high=0.49)

    idx = t.find_intersecting(centroid[0], centroid[1], centroid[2], radius)

    assert len(idx) == 0, "Should be empty, but {} were found instead.".format(idx)


def test_intersection_all():

    centroids = np.array([[.0,  1.,  0.],
                         [-0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.],
                         [ 0.5 * np.sqrt(2), -0.5 * np.sqrt(2), 0.]]).astype(np.float32)


    radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)

    t = SomaTree(centroids, radii)

    centroid = np.array([0., 0., 0.]).astype(np.float32)

    radius = np.random.uniform(low=0.5, high=1.0)

    idx = t.find_intersecting(centroid[0], centroid[1], centroid[2], radius)

    expected_result = np.array([0, 1, 2], dtype=np.uintp)
    assert np.all(idx == expected_result), (idx, expected_result, centroids, radii)


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

    t = SomaTree(centroids, radii)

    idx = t.find_intersecting(q_centroid[0], q_centroid[1], q_centroid[2], q_radius)

    assert len(np.setdiff1d(idx, expected_result)) == 0, (idx, expected_result)


def test_nearest_all():

    centroids, radii = arange_values()

    centroids = centroids[::-1]
    radii = (np.ones(len(centroids)) * 0.01).astype(np.float32)

    t = SomaTree(centroids, radii)

    center = np.array([0., 0., 0.]).astype(np.float32)
    radius = 0.001

    idx = t.find_nearest(center[0], center[1], center[2], 10)
    assert np.all(np.sort(idx) == np.sort(np.arange(10, dtype=np.uintp))), idx


def test_nearest_random():
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

    center = np.array([0., 0., 0.]).astype(np.float32)

    distances = np.linalg.norm(centroids, axis=1) - radii
    distances[distances < .0] = .0
    expected_result = np.argsort(distances)[:K]

    t = SomaTree(centroids, radii)

    idx = t.find_nearest(center[0], center[1], center[2], K)

    assert np.all(np.sort(idx) == np.sort(expected_result)), (idx, expected_result)


if __name__ == "__main__":
    test_intersection_none()
    print("[PASSED] Test test_intersection_none()")

    test_intersection_all()
    print("[PASSED] Test test_intersection_all()")

    test_intersection_random()
    print("[PASSED] Test test_intersection_random()")

    test_nearest_all()
    print("[PASSED] Test test_nearest_all()")

    # At east 1 in 3 must exactly match
    for i in range(3):
        try:
            test_nearest_random()
            print("[PASSED] Test test_nearest_random()")
            break
        except:
            if i == 2: raise
