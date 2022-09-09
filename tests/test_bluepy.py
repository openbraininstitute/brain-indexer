"""
    Blue Brain Project - Spatial-Index

    A test that checks if the results from Spatial Index are the same as the
    results from bluepy, ran on the same scx-v6-2k dataset.
"""

import os
import sys
import numpy as np
import spatial_index
import pytest
pytest_skipif = pytest.mark.skipif
pytest_long = pytest.mark.long

BLUECONFIG_2K = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/BlueConfig"
CIRCUIT_2K_SI = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit2k_si"


def bluepy_check(circuit, result):

    i = 0
    for gid in result['gid']:
        # Skip somas.
        if result['is_soma'][i]:
            i += 1
            continue

        # adding one to the gid since BluePy is 1-indexed
        m = circuit.morph.get(int(gid + 1), transform=True)

        section_id = result['section_id'][i]
        segment_id = result['segment_id'][i]

        # endpoint 1 from BluePy
        p1_b = m.sections[section_id - 1].points[segment_id]
        # endpoint 1 from SpatialIndex
        p1_s = result['endpoint1'][i]
        # endpoint 2 from BluePy
        p2_b = m.sections[section_id - 1].points[segment_id + 1]
        # endpoint 2 from SpatialIndex
        p2_s = result['endpoint2'][i]
        # Radius from BluePy
        r_b = m.sections[section_id - 1].diameters[segment_id] / 2
        # Radius from SpatialIndex
        r_s = result['radius'][i]

        assert np.allclose(p1_b, p1_s, atol=1e-4)
        assert np.allclose(p2_b, p2_s, atol=1e-4)
        assert np.allclose(r_b, r_s, atol=1e-4)
        i += 1


@pytest_skipif(not os.path.exists(CIRCUIT_2K_SI),
               reason="Circuit file not available")
@pytest_long
def test_bluepy_validation():

    from bluepy import Circuit

    N_QUERIES = int(sys.argv[1]) if len(sys.argv) > 1 else 20

    indexer = spatial_index.open_index(CIRCUIT_2K_SI)
    # Load the circuit
    c = Circuit(BLUECONFIG_2K)

    i = 0
    while i < N_QUERIES:
        min_point = np.random.uniform(low=-500, high=1800, size=3).astype(np.float32)
        for m in range(1, 6):
            max_point = min_point + ((1.1 ** m) * np.random.uniform(
                low=10, high=20, size=1).astype(np.float32))
            result = indexer.window_query(min_point, max_point)
            if not result['gid'].size == 0:
                i += 1
                bluepy_check(c, result)
                break


if __name__ == "__main__":
    test_bluepy_validation()
