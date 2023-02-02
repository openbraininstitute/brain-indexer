"""
    Blue Brain Project - Spatial-Index

    For morphology indexes, the SONATA nodes file can be queries for
    additional node properties not directly stored in the morphology
    index. This tests ensures consistency, i.e. that the node properties
    returned by the query really belong to the correct neuron.

    This is done by loading the morphology name and transformation parameters
    (all node properties) via extended fields. It then loads the reference
    morphology from disk via MorphIO, performs the transformation; and compares
    the retrieved segments (builtin fields) against the computed segments.
"""

import morphio
import os
import spatial_index
import numpy as np
import quaternion as npq
CIRCUIT_2K = "tests/data/tiny_circuits/circuit-10/"
INDEX_DIR = CIRCUIT_2K + "indexes/morphology/in_memory"
MORPHOLOGY_DIR = CIRCUIT_2K + "morphologies/ascii/"


def remove_somas(d):
    i = d['section_id'] != 0

    return {
        key: (value[0][i], value[1][i]) if key == "endpoints" else value[i]
        for key, value in d.items()
    }


def rototranslate(xyz, model):
    # npq requries quaternion in the order: (w, x, y, z)
    position, rotation = model
    points = npq.rotate_vectors(
        npq.quaternion(*rotation).normalized(),
        xyz
    )
    return points + position


def get_spatial_indices(index_path, min, max):
    index = spatial_index.open_index(index_path, max_cache_size_mb=1000)
    pos_keys = ["x", "y", "z"]
    rot_keys = [f"orientation_{key}" for key in ["w", "x", "y", "z"]]
    matches = index.box_query(
        min, max,
        fields=[
            'gid', 'section_id', 'segment_id', 'section_type',
            'morphology', 'endpoints', *pos_keys, *rot_keys,
        ]
    )

    # Delete entries with section_id == 0 (soma)
    matches = remove_somas(matches)

    morphos = []
    for i, morpho_name in enumerate(matches["morphology"]):
        morpho_path = os.path.join(MORPHOLOGY_DIR, morpho_name + ".asc")
        rot = np.array([matches[key][i] for key in rot_keys])
        pos = np.array([matches[key][i] for key in pos_keys])
        morphos.append((morpho_path, (pos, rot)))
    return matches, morphos


def test_sonata_sanity():
    min_corner = [70.0, 1950.0, 50.0]
    max_corner = [85.0, 2000.0, 70.0]
    si_results, neuron_params = get_spatial_indices(INDEX_DIR, min_corner, max_corner)
    for k, (path, model) in enumerate(neuron_params):
        morphology = morphio.Morphology(path)
        points = rototranslate(morphology.points, model)
        section_id = si_results['section_id'][k]
        segment_id = si_results['segment_id'][k]
        # Minus one because SI section IDs reserve `0` for the soma.
        # In MorphIO `section_offsets` doesn't reserve space for the soma.
        section_start = morphology.section_offsets[section_id - 1]

        assert np.allclose(
            points[section_start + segment_id],
            si_results['endpoints'][0][k],
            rtol=1e-5
        )
        assert np.allclose(
            points[section_start + segment_id + 1],
            si_results['endpoints'][1][k],
            rtol=1e-5
        )

    print("Success! SONATA fields and builtin fields are consistent.")


if __name__ == "__main__":
    test_sonata_sanity()
