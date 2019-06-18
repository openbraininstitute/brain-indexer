import pytest
import numpy as np
from spatial_index import sphere_rtree as SphereIndex


def arange_values():

	radii = np.arange(10).astype(np.float32)

	centroids = np.column_stack((radii, radii, radii)).astype(np.float32)

	return centroids, radii

@pytest.fixture
def arange_rtree():

	centroids, radii = arange_values()
	return SphereIndex(centroids, radii)


def test_packed_constructor(arange_rtree):

	data = arange_rtree.data()
	idx = data[:, 0].argsort()
	data = data[idx]

	r_data = np.column_stack(arange_values())

	assert r_data.size == data.size

	assert np.all(r_data == data), "\nExpected: {} \nResult: {}".format(r_data, data)

def test_nearest():

	centroids, radii = arange_values()

	centroids = centroids[::-1]

	radii = (np.ones(len(centroids)) * 0.01).astype(np.float32)

	t = SphereIndex(centroids, radii)

	center = np.array([0., 0., 0.]).astype(np.float32)
	radius = 0.001

	idx = t.nearest(center[0], center[1], center[2], 10)

	assert np.all(idx == np.array([9, 8, 7, 6, 5, 4, 3, 2, 1, 0], dtype=np.uintp))

def test_insert():

	centroids, radii = arange_values()

	t = SphereIndex()

	for i in range(len(centroids)):
		t.insert(centroids[i, 0], centroids[i, 1], centroids[i, 2], radii[i])

	data = t.data()
	idx = data[:, 0].argsort()

	data = data[idx]

	r_data = np.column_stack((centroids, radii))

	assert r_data.size == data.size

	assert np.all(r_data == data), "\nExpected: {} \nResult: {}".format(r_data, data)


def test_intersection_none():
	"""
	"""

	centroids = np.array([[.0,    1., 0.],
						 [- 0.5 * np.sqrt(2),   - 0.5 * np.sqrt(2), 0.],
						 [0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.]]).astype(np.float32)


	radii = np.random.uniform(low=0.0, high=0.5, size=len(centroids)).astype(np.float32)

	t = SphereIndex(centroids, radii)

	centroid = np.array([0., 0., 0.]).astype(np.float32)

	radius = np.random.uniform(low=0.0, high=0.49)

	idx = t.intersection(centroid[0], centroid[1], centroid[2], radius)

	assert len(idx) == 0, "Should be empty, but {} were found instead.".format(idx)


def test_intersection_all():
	"""
	"""

	centroids = np.array([[.0,    1., 0.],
						 [- 0.5 * np.sqrt(2),   - 0.5 * np.sqrt(2), 0.],
						 [0.5 * np.sqrt(2), - 0.5 * np.sqrt(2), 0.]]).astype(np.float32)


	radii = np.random.uniform(low=0.5, high=1.0, size=len(centroids)).astype(np.float32)

	t = SphereIndex(centroids, radii)

	centroid = np.array([0., 0., 0.]).astype(np.float32)

	radius = np.random.uniform(low=0.5, high=1.0)

	idx = t.intersection(centroid[0], centroid[1], centroid[2], radius)

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

	t = SphereIndex(centroids, radii)

	idx = t.intersection(q_centroid[0], q_centroid[1], q_centroid[2], q_radius)

	assert len(np.setdiff1d(idx, expected_result)) == 0, (idx, expected_result)


def test_data():

	col = np.arange(5).astype(np.float32)
	centroids = np.column_stack((col, col, col))

	t = SphereIndex(centroids, col)

	r_data = np.column_stack((centroids, col))

	data = t.data()

	idx = np.argsort(data[:, 0])

	data = data[idx]

	assert np.allclose(r_data, data), (r_data, data)






