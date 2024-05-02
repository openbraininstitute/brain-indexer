"""
Python tests to the morphology spatial index.

It reuses tests from test_rthree_sphere
"""
import numpy as np
import os.path
import sys
import pytest

from brain_indexer import core
from brain_indexer import MorphIndex, SectionType

# Add this dir to path so we can import the other tests
sys.path.append(os.path.abspath(os.path.dirname(__file__)))
import test_rtree_sphere  # NOQA


def test_morphos_insert():
    c1 = test_rtree_sphere.arange_centroids(10)
    c2 = test_rtree_sphere.arange_centroids(10)
    c2[:, 1] += 2
    radii = np.full(10, 0.5, dtype=np.float32)

    t = core.MorphIndex()
    for i in range(len(c1)):
        t._insert(i // 4, i % 4, i % 4, c1[i], c2[i], radii[i],
                  SectionType.undefined)

    for i, c in enumerate(c1):
        idx = t._find_nearest(c, 1)[0]
        assert idx[0] == (i // 4) and idx[1] == (i % 4), "i={}, idx={} ".format(i, idx)


def test_bulk_single_segments_no_soma():
    """
    Adding a neuron with one segment per section
    .=
    .=
    .=
    """

    n_sections = 3
    points = np.array([
        3 * [1.0], 3 * [1.1],
        3 * [2.0], 3 * [2.1],
        3 * [3.0], 3 * [3.1],
    ])

    n_points = points.shape[0]
    assert n_points == 2 * n_sections, "Broken internal test logic."

    offsets = np.array(range(0, n_points, 2))
    radii = np.arange(n_points)  # Fake piecewise linear radii
    types = np.full(len(points), SectionType.undefined)

    rtree = core.MorphIndex()
    rtree._add_neuron(0, points, radii, offsets, types, has_soma=False)

    assert len(rtree) == n_sections


def test_bulk_single_segments_with_soma():
    """
    Adding a neuron with one segment per section
    (S).=
       .=
       .=
    """

    n_sections = 3
    points = np.array([
        3 * [9.0],
        3 * [1.0], 3 * [1.1],
        3 * [2.0], 3 * [2.1],
        3 * [3.0], 3 * [3.1],
    ])

    n_points = points.shape[0]
    n_elements = n_sections + 1

    assert n_points == 2 * n_sections + 1, "Broken internal test logic."

    offsets = np.array([0] + list(range(1, n_points, 2)))
    radii = np.arange(n_points)  # Fake piecewise linear radii.
    types = np.full(n_points, SectionType.undefined)

    rtree = core.MorphIndex()
    rtree._add_neuron(0, points, radii, offsets, types, has_soma=True)

    assert len(rtree) == n_elements


def test_bulk_neuron_add():
    """
    Adding a neuron alike:
    ( S ).=.=.=+=.=.=.=.=.
               +=.=.=.=.=.
      0  1 2 3 4 5 6 7 8 9  <- x coord
      0   1 2 3 4 5 6 7 8   <- segment_id (section 1)
                1 2 3 4 5   <- segment_id (section 2)
    """
    N = 10 + 6
    nrn_id = 1
    points = np.zeros([N, 3], dtype=np.float32)
    points[:, 0] = np.concatenate((np.arange(10), np.arange(4, 10)))
    points[10:, 1] = 1
    radius = np.ones(N, dtype=np.float32)
    types = np.full(N, SectionType.undefined)

    rtree = core.MorphIndex()
    rtree._add_neuron(nrn_id, points, radius, [1, 10], types)

    # around a point, expect 4 segments, 2 from branch 1, 2 from branch 2
    COORD_SEARCH = [5, 0, 0]
    EXPECTED_IDS = {(1, 1, 3), (1, 1, 4), (1, 2, 0), (1, 2, 1)}
    idx = rtree._find_nearest(COORD_SEARCH, 4)

    assert len(idx) == len(EXPECTED_IDS)
    for out_id in idx:
        assert out_id in idx

    # New API
    index = MorphIndex(rtree)
    objs = index.box_query(
        [COORD_SEARCH[0] - 0.9, -.1, -.1],
        [COORD_SEARCH[0] + 0.9, .1, .1],
        fields="raw_elements"
    )
    assert len(objs) == len(EXPECTED_IDS)
    for obj in objs:
        assert (obj.gid, obj.section_id, obj.segment_id) in EXPECTED_IDS


def test_add_neuron_with_soma_and_toString():
    points = [
        [1, 3, 5],
        [2, 4, 6],
        [2, 4, 6],
        [10, 10, 10]
    ]
    radius = [3, 2, 2, 1]
    offsets = [0, 2]
    n_points = len(points)
    types = np.full(n_points, SectionType.undefined)
    rtree = core.MorphIndex()
    rtree._add_neuron(1, points, radius, offsets, types)
    str_expect = (
        'IndexTree([\n'
        '  Soma(id=(1, 0, 0), Sphere(centroid=[1 3 5], radius=3))\n'
        '  Segment(id=(1, 1, 0), Cylinder(centroids=([1 3 5], [2 4 6]), radius=2.5))\n'
        '  Segment(id=(1, 2, 0), Cylinder(centroids=([2 4 6], [10 10 10]), radius=1.5))\n'
        '])')
    str_result = str(rtree)
    assert str_result == str_expect


def test_add_neuron_without_soma_and_toString():
    points = [
        [1.123, 3, 5],
        [2, 4, 6],
        [2, 4, 6],
        [10, 10, 10]
    ]
    radius = [3, 2, 2, 1]
    offsets = [0, 2]
    n_points = len(points)
    types = np.full(n_points, SectionType.undefined)
    rtree = core.MorphIndex()
    # add segments
    rtree._add_neuron(1, points, radius, offsets, types, has_soma=False)
    str_expect = (
        'IndexTree([\n'
        '  Segment(id=(1, 1, 0), Cylinder(centroids=([1.12 3 5], [2 4 6]), radius=2.5))\n'
        '  Segment(id=(1, 2, 0), Cylinder(centroids=([2 4 6], [10 10 10]), radius=1.5))\n'
        '])')
    str_result = str(rtree)
    assert str_result == str_expect

    # add soma
    s_p = [1, 3, 5.135]
    s_r = 3136
    s_id = 1
    rtree._add_soma(s_id, s_p, s_r)
    str_expect = (
        'IndexTree([\n'
        '  Segment(id=(1, 1, 0), Cylinder(centroids=([1.12 3 5], [2 4 6]), radius=2.5))\n'
        '  Segment(id=(1, 2, 0), Cylinder(centroids=([2 4 6], [10 10 10]), radius=1.5))\n'
        '  Soma(id=(1, 0, 0), Sphere(centroid=[1 3 5.14], radius=3.14e+03))\n'
        '])')
    str_result = str(rtree)
    assert str_result == str_expect


def test_endpoints_retrieval():
    points = [
        [1, 3, 5],
        [2, 4, 6],
        [2, 4, 6],
        [10, 10, 10]
    ]
    radius = [3, 2, 2, 1]
    offsets = [0, 2]
    min_corner = [-50, 0, 0]
    max_corner = [0, 50, 50]
    n_points = len(points)
    types = np.full(n_points, SectionType.undefined)
    rtree = core.MorphIndex()
    rtree._add_neuron(1, points, radius, offsets, types)
    array_expect = np.array([[1, 3, 5], [2, 4, 6]])

    index = MorphIndex(rtree)
    idx = index.box_query(min_corner, max_corner, fields="raw_elements")
    assert idx[0].endpoints is None
    np.testing.assert_allclose(idx[1].endpoints, array_expect)


def test_section_type_retrieval():
    # This test creates a small artificial circuit
    # with one soma and three sections with
    # one segment each.
    # Every segment is a different type.
    # The test checks if the section type is
    # correctly retrieved.
    # Here's a sketch of the circuit:
    # (S) . = (axon)
    #     . = (basal dendrite)
    #     . = (apical dendrite)

    points = [
        [1, 3, 5],
        [2, 4, 6],
        [3, 7, 8],
        [10, 10, 10],
        [2, 3, 5],
        [7, 6, 8]
    ]
    radius = [3, 2, 2, 1, 1, 1]
    offsets = [0, 2, 4]
    min_corner = [-10, -10, -10]
    max_corner = [15, 15, 15]
    types = [SectionType.axon,
             SectionType.basal_dendrite,
             SectionType.apical_dendrite]
    rtree = core.MorphIndex()
    rtree._add_neuron(1, points, radius, offsets, types, True)

    index = MorphIndex(rtree)
    results = index.box_query(min_corner, max_corner, fields="section_type")
    assert results[0] == core.SectionType.soma
    assert results[1] == core.SectionType.axon
    assert results[2] == core.SectionType.basal_dendrite
    assert results[3] == core.SectionType.apical_dendrite

    all_types = [SectionType.soma, SectionType.axon,
                 SectionType.basal_dendrite, SectionType.apical_dendrite]
    for k, type in enumerate(all_types):
        expected = np.full(results.size, False)
        expected[k] = True

        actual = results == type
        assert np.all(expected == actual)


@pytest.mark.parametrize(
    "points, radius, offsets",
    [
        ([], [1], [0]),
        ([[0, 0, 0]], [], [0]),
    ]
)
def test_add_neuron_exc_with_soma(points, radius, offsets):
    types = np.full(len(points), SectionType.undefined)
    rtree = core.MorphIndex()
    with pytest.raises(ValueError):
        rtree._add_neuron(1, points, radius, offsets, types, has_soma=True)


@pytest.mark.parametrize(
    "points, radius, offsets",
    [
        ([[0., 0., 0.], [0., 10., 0.]], [1, 1, ], [1, 1, 2, ]),
        ([[0., 0., 0.], [0., 10., 0.]], [1, 1, ], [4, ]),
        ([[0., 0., 0.], [0., 10., 0.]], [], [0, 0, ]),
        ([[0., 0., 0.], [0., 10., 0.]], [1, 1, ], []),
        ([[0., 0., 0.]], [1, ], [0, ])
    ]
)
def test_add_neuron_exc_without_soma(points, radius, offsets):
    types = np.full(len(points), [SectionType.undefined])
    rtree = core.MorphIndex()
    with pytest.raises(ValueError):
        rtree._add_neuron(1, points, radius, offsets, types, has_soma=False)
