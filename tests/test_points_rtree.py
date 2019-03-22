import pytest
import numpy as np

from spatial_index import point_rtree as PointIndex



def test_point_intersection_all():


	p1 = np.array([1.2, 2.3, 4.5], dtype=np.float32)
	p2 = np.array([2.4, 3.3, 5.1], dtype=np.float32)


	points = np.random.uniform(low=p1, high=p2, size=(10, 3)).astype(np.float32)

	t = PointIndex(points)


	idx = t.intersection(p1, p2)

	expected_result = np.arange(10, dtype=np.uintp)

	assert len(np.setdiff1d(idx, expected_result)) == 0, (idx, expected_result)


def test_intersection_packed_random():

	p1 = np.array([-10., -10., -10.], dtype=np.float32)
	p2 = np.array([10., 10., 10.], dtype=np.float32)

	N_spheres = 100

	points = np.random.uniform(low=p1, high=p2, size=(N_spheres, 3)).astype(np.float32)


	q1 =  np.array([-2., -5., -3.], dtype=np.float32)
	q2 =  np.array([8., 2., 4.5], dtype=np.float32)


	expected_result = np.where( (points[:, 0] >= q1[0]) & (points[:, 0] <= q2[0]) & \
							    (points[:, 1] >= q1[1]) & (points[:, 1] <= q2[1]) & \
							    (points[:, 2] >= q1[2]) & (points[:, 2] <= q2[2])
							   )[0]

	t = PointIndex(points)

	idx = t.intersection(q1, q2)

	assert len(np.setdiff1d(idx, expected_result)) == 0, (idx, expected_result)

