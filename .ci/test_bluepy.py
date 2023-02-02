"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from Spatial Index are the same as the
    results from bluepy, ran on the same scx-v6-2k dataset.
"""

import os
import numpy as np
import spatial_index

DATADIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v5"
BLUECONFIG_2K = os.path.join(DATADIR, "bluepy_validation/BlueConfig")
CIRCUIT_2K_SI = os.path.join(DATADIR, "bluepy_validation/indexes/morphology/in_memory")


class BluepyCache:
    def __init__(self, circuit):
        self._circuit = circuit
        self._morphs = dict()

    def get(self, gid):
        gid = int(gid)
        if gid not in self._morphs:
            self._morphs[gid] = self._circuit.morph.get(gid, transform=True)

        return self._morphs[gid]


def bluepy_check(bluepy_cache, result):
    for i, gid in enumerate(result['gid']):
        # Skip somas.
        if result['is_soma'][i]:
            continue

        # adding one to the gid since BluePy is 1-indexed
        m = bluepy_cache.get(gid + 1)

        section_id = result['section_id'][i]
        segment_id = result['segment_id'][i]

        # endpoint 1 from BluePy
        p1_b = m.sections[section_id - 1].points[segment_id]
        # endpoint 1 from SpatialIndex
        p1_s = result['endpoints'][0][i]
        # endpoint 2 from BluePy
        p2_b = m.sections[section_id - 1].points[segment_id + 1]
        # endpoint 2 from SpatialIndex
        p2_s = result['endpoints'][1][i]
        # Radius from BluePy
        r1 = m.sections[section_id - 1].diameters[segment_id] / 2
        r2 = m.sections[section_id - 1].diameters[segment_id + 1] / 2
        r_b = (r1 + r2) / 2
        # Radius from SpatialIndex
        r_s = result['radius'][i]
        # Section type from SpatialIndex
        t_s = result['section_type'][i]
        # Section type from BluePy
        t_b = m.sections[section_id - 1].type

        assert np.allclose(p1_b, p1_s, atol=1e-4)
        assert np.allclose(p2_b, p2_s, atol=1e-4)
        assert np.allclose(r_b, r_s, atol=1e-4)
        assert t_b == t_s


def test_bluepy_validation():

    from bluepy import Circuit

    N_QUERIES = 200

    indexer = spatial_index.open_index(CIRCUIT_2K_SI)
    bluepy_cache = BluepyCache(Circuit(BLUECONFIG_2K))

    i = 0
    while i < N_QUERIES:
        min_point = np.random.uniform(low=-500, high=1800, size=3).astype(np.float32)
        for m in range(1, 6):
            max_point = min_point + ((1.1 ** m) * np.random.uniform(
                low=10, high=20, size=1).astype(np.float32))
            result = indexer.box_query(min_point, max_point)
            if not result['gid'].size == 0:
                i += 1
                bluepy_check(bluepy_cache, result)
                break

    print("Success! No differences in bluepy and SpatialIndex morphologies detected.")


if __name__ == "__main__":
    test_bluepy_validation()
