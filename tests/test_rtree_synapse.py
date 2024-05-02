import numpy as np

from brain_indexer import core
from brain_indexer.index import SynapseIndex

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
post_gids = [1, 1, 2, 3, 3, 3, 4]
pre_gids = np.array([0, 0, 1, 2, 2, 2, 3], dtype=np.intp)


def _test_rtree(index):
    objs = index.box_query(
        [-1., -1., -1.], [1., 1., 1.],
        fields="raw_elements"
    )
    objs.sort(key=lambda x: x.id)
    assert len(objs) == 4

    assert (list(sorted(n for n in dir(objs[0]) if not n.startswith('_')))
            == ['centroid', 'id', 'ids', 'post_gid', 'pre_gid', ]), \
        'New property added, make sure a test is added below'

    expected_ids = (0, 1, 2, 6)
    expected_post_gids = (1, 1, 2, 4)
    expected_pre_gids = (0, 0, 1, 3)

    for obj, id_, post_gid, pre_gid in zip(objs,
                                           expected_ids,
                                           expected_post_gids,
                                           expected_pre_gids):

        assert obj.id == id_ and obj.post_gid == post_gid and obj.pre_gid == pre_gid, \
            (obj.id, obj.post_gid, obj.pre_gid, "!=", id_, post_gid, pre_gid)

    n_elems_within = index.box_counts([-1., -1., -1.], [1., 1., 1.])
    assert n_elems_within == 4

    aggregated_per_gid = index.box_counts(
        [-1., -1., -1.], [1., 1., 1.], group_by="post_gid"
    )
    assert aggregated_per_gid[1] == 2
    assert aggregated_per_gid[2] == 1
    assert 3 not in aggregated_per_gid
    assert aggregated_per_gid[4] == 1

    aggregated_2 = index.box_counts(
        [-5., -5., -5.], [5., 5., 5.], group_by="post_gid"
    )
    assert aggregated_2[1] == 2
    assert aggregated_2[2] == 1
    assert aggregated_2[3] == 3
    assert aggregated_2[4] == 1

    # This is a backward compatibility check.
    aggregated_old = index.box_counts(
        [-5., -5., -5.], [5., 5., 5.], group_by="gid"
    )

    assert aggregated_2.keys() == aggregated_old.keys()
    for a, b in zip(aggregated_2.values(), aggregated_old.values()):
        assert a == b


def test_synapse_query_aggregate():
    rtree = core.SynapseIndex()
    rtree._add_synapses(ids, post_gids, pre_gids, points)
    _test_rtree(SynapseIndex(rtree))
