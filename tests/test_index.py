# This file covers correctness of indexes contained in `index.py`.

import numpy as np
import brain_indexer


def check_point_index_boxes(index, positions):
    eps = 1e-5

    n_queries = 20

    boxes = zip(
        np.random.uniform(size=(n_queries, 3)),
        np.random.uniform(size=(n_queries, 3)),
    )

    ids = np.arange(len(index))

    for p1, p2 in boxes:
        print((p1, p2))
        found = index.box_query(p1, p2, fields="id")
        mask = ids == ids
        mask[found] = False

        p1, p2 = np.minimum(p1, p2), np.maximum(p1, p2)

        if np.size(found) > 0:
            # If found the points must be inside a slightly larger box.
            assert np.all(
                np.logical_and(
                    p1 - eps <= positions[np.logical_not(mask)],
                    positions[np.logical_not(mask)] <= p2 + eps,
                )
            )

            # If not found the points must be outside of a slightly smaller box.
            assert np.all(
                np.any(
                    np.logical_or(
                        positions[mask] <= p1 + eps,
                        p2 - eps <= positions[mask],
                    ),
                    axis=1,
                )
            )


def check_point_index_spheres(index, positions):
    eps = 1e-5
    n_queries = 20

    spheres = zip(
        np.random.uniform(size=(n_queries, 3)),
        np.random.uniform(size=(n_queries)),
    )

    ids = np.arange(len(index))

    for c, r in spheres:
        found = index.sphere_query(c, r, fields="id")

        mask = ids == ids
        mask[found] = False

        # If found the points must be inside a slightly larger sphere.
        assert np.all(
            np.linalg.norm(positions[np.logical_not(mask)] - c, axis=1) < r + eps
        )

        # If not found the points must be outside of a slightly smaller box.
        assert np.all(
            np.linalg.norm(positions[mask] - c, axis=1) > r - eps
        )


def test_point_index():
    n_elements = 1000
    centroids = np.random.uniform(size=(n_elements, 3))
    ids = np.arange(centroids.shape[0])

    index = brain_indexer.PointIndexBuilder.from_numpy(centroids, ids)

    assert len(index) == n_elements
    assert np.size(index.box_query(3 * [-1.0], 3 * [2.0], fields="id")) == n_elements

    check_point_index_boxes(index, centroids)
    check_point_index_spheres(index, centroids)
