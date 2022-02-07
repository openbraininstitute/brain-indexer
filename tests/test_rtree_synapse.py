import numpy as np

from spatial_index._spatial_index import SynapseIndex

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


def test_synapse_query_aggregate():
    rtree = SynapseIndex()
    rtree.add_synapses(ids, post_gids, pre_gids, points)

    objs = rtree.find_intersecting_window_objs([-1., -1., -1.], [1., 1., 1.])
    objs.sort(key=lambda x: x.id)
    assert len(objs) == 4
    expected_ids, expected_post_gids, expected_pre_gids = (0, 1, 2, 6), \
        (1, 1, 2, 4), (0, 0, 1, 3)
    for obj, id_, post_gid, pre_gid in zip(objs,
                                           expected_ids,
                                           expected_post_gids,
                                           expected_pre_gids):
        assert obj.id == id_ and obj.post_gid == post_gid and obj.pre_gid == pre_gid, \
            (obj.id, obj.post_gid, obj.pre_gid, "!=", id_, post_gid, pre_gid)

    # New way
    n_elems_within = rtree.count_intersecting([-1., -1., -1.], [1., 1., 1.])
    assert n_elems_within == 4

    aggregated_per_gid = rtree.count_intersecting_agg_gid([-1., -1., -1.], [1., 1., 1.])
    assert aggregated_per_gid[1] == 2
    assert aggregated_per_gid[2] == 1
    assert 3 not in aggregated_per_gid
    assert aggregated_per_gid[4] == 1

    aggregated_2 = rtree.count_intersecting_agg_gid([-5., -5., -5.], [5., 5., 5.])
    assert aggregated_2[1] == 2
    assert aggregated_2[2] == 1
    assert aggregated_2[3] == 3
    assert aggregated_2[4] == 1


if __name__ == "__main__":
    test_synapse_query_aggregate()
    print("Test aggregated_per_gid PASSED!")
