import itertools
import os
import pytest
import numpy as np
import tempfile

import spatial_index

from spatial_index import open_index
from spatial_index.index import is_non_string_iterable


CIRCUIT_10_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit-10"
CIRCUIT_1K_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1/circuit-1k"


def assert_valid_dict_result(results, expected_fields):
    assert isinstance(results, dict)
    assert sorted(results.keys()) == sorted(expected_fields)

    for field in expected_fields:
        assert isinstance(results[field], np.ndarray)

    lengths = set(r.shape[0] for r in results.values())
    assert len(lengths) == 1, results
    assert next(iter(lengths)) > 0


def assert_valid_single_result(result):
    assert isinstance(result, (list, np.ndarray)), result
    assert len(result) > 0


def check_query(query, query_shape, query_kwargs, builtin_fields):
    special_fields = ["raw_elements"]
    all_fields = builtin_fields + special_fields

    for field in all_fields:
        result = query(*query_shape, fields=field, **query_kwargs)
        assert_valid_single_result(result)

    for k in range(1, len(builtin_fields) + 1):
        for fields in itertools.combinations(builtin_fields, k):
            result = query(*query_shape, fields=fields, **query_kwargs)
            assert_valid_dict_result(result, fields)

    for field in special_fields:
        with pytest.raises(Exception):
            query(*query_shape, fields=[field], **query_kwargs)

    for field in ["", [], [""]]:
        with pytest.raises(Exception):
            query(*query_shape, fields=field, **query_kwargs)

    results = query(*query_shape, fields=None, **query_kwargs)
    assert_valid_dict_result(results, builtin_fields)


def check_all_regular_query_api(index, window, sphere, accuracy):
    query_kwargs = {"accuracy": accuracy}
    builtin_fields = index._core_index.builtin_fields

    check_query(index.window_query, window, query_kwargs, builtin_fields)
    check_query(index.vicinity_query, sphere, query_kwargs, builtin_fields)


def check_sonata_query(query, query_shape, query_kwargs, builtin_fields):
    sonata_fields = ["afferent_section_id", "afferent_segment_id"]
    all_fields = builtin_fields + sonata_fields

    for field in sonata_fields:
        result = query(*query_shape, fields=field, **query_kwargs)
        assert_valid_single_result(result)

    for k in range(1, len(all_fields) + 1):
        for fields in itertools.combinations(all_fields, k):
            result = query(*query_shape, fields=fields, **query_kwargs)
            assert_valid_dict_result(result, fields)


def circuit_10_config(index_kind, element_kind):
    if "synapse" in element_kind:
        index_path = os.path.join(CIRCUIT_1K_DIR, f"indexes/{element_kind}/{index_kind}")

    elif element_kind == "morphology":
        index_path = os.path.join(CIRCUIT_10_DIR, f"indexes/{element_kind}/{index_kind}")

    else:
        raise ValueError(f"Invalid {element_kind = }.")

    index = open_index(index_path)

    # We know there's a segment at:
    #   [ 116.732, 1957.872, 24.925]
    # and seems to also work for the synapse index.
    window = ([50.0, 1800.0, 10.0], [130.0, 2000.0, 50.0])
    sphere = ([115.0, 1950.0, 25.0], 100.0)

    return index, window, sphere


def spheres_config(index_kind):
    centroids = np.random.uniform(size=(10, 3)).astype(np.float32)
    radii = np.random.uniform(size=10).astype(np.float32)
    index = spatial_index.SphereIndexBuilder.from_numpy(centroids, radii)

    window = [0.0, 0.0, 0.0], [1.0, 1.0, 1.0]
    sphere = [0.5, 0.5, 0.5], 0.5

    return index, window, sphere


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology"],
        ["in_memory", "multi_index"],
        [None, "bounding_box", "best_effort"]
    )
)
def test_index_query_api(element_kind, index_kind, accuracy):
    index, window, sphere = circuit_10_config(index_kind, element_kind)
    check_all_regular_query_api(index, window, sphere, accuracy)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy",
    itertools.product(
        ["synapse"],
        ["in_memory", "multi_index"],
        [None, "bounding_box", "best_effort"]
    )
)
def test_index_sonata_query_api(element_kind, index_kind, accuracy):
    query_kwargs = {"accuracy": accuracy}

    index, window, sphere = circuit_10_config(index_kind, element_kind)
    builtin_fields = index._core_index.builtin_fields

    check_sonata_query(index.window_query, window, query_kwargs, builtin_fields)
    check_sonata_query(index.vicinity_query, sphere, query_kwargs, builtin_fields)


@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy",
    itertools.product(
        ["sphere"],
        ["in_memory"],
        [None, "bounding_box", "best_effort"]
    )
)
def test_sphere_index_query_api(element_kind, index_kind, accuracy):

    index, window, sphere = spheres_config(index_kind)
    check_all_regular_query_api(index, window, sphere, accuracy)

    check_index_bounds_api(index)
    check_counts_api(index, window, sphere, accuracy)


def check_counts(counts_method, query_shape, query_kwargs):
    counts = counts_method(*query_shape, group_by=None, **query_kwargs)
    assert counts > 0


def expected_builtin_fields(index):
    synapse_index_classes = (
        spatial_index.core.SynapseIndex,
        spatial_index.core.SynapseMultiIndex
    )

    morph_index_classes = (
        spatial_index.core.MorphIndex,
        spatial_index.core.MorphMultiIndex
    )

    sphere_index_classes = (
        spatial_index.core.SphereIndex,
    )

    if isinstance(index._core_index, synapse_index_classes):
        return ["id", "pre_gid", "post_gid", "position"]

    elif isinstance(index._core_index, morph_index_classes):
        return ["gid", "section_id", "segment_id",
                "ids", "centroid",
                "radius", "endpoint1", "endpoint2",
                "kind"]

    elif isinstance(index._core_index, sphere_index_classes):
        return ["id", "centroid", "radius"]

    else:
        raise RuntimeError(f"Broken test logic. [{type(index._core_index)}]")


def check_builtin_fields(index):
    expected = expected_builtin_fields(index)
    actual = index._core_index.builtin_fields

    assert sorted(expected) == sorted(actual)


def check_counts_api(index, window, sphere, accuracy):
    query_kwargs = {"accuracy": accuracy}
    check_builtin_fields(index)
    check_counts(index.window_counts, window, query_kwargs)
    check_counts(index.vicinity_counts, sphere, query_kwargs)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology"],
        ["in_memory", "multi_index"],
        [None, "bounding_box", "best_effort"]
    )
)
def test_index_counts_api(element_kind, index_kind, accuracy):
    index, window, sphere = circuit_10_config(index_kind, element_kind)
    check_counts_api(index, window, sphere, accuracy)


def check_index_bounds_api(index):
    min_corner, max_corner = index.bounds()
    assert isinstance(min_corner, np.ndarray)
    assert min_corner.dtype == np.float32

    assert isinstance(max_corner, np.ndarray)
    assert max_corner.dtype == np.float32

    assert np.all(min_corner < max_corner)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology"],
        ["in_memory", "multi_index"],
    )
)
def test_index_bounds_api(element_kind, index_kind):
    index, _, _ = circuit_10_config(index_kind, element_kind)
    check_index_bounds_api(index)


def check_index_write_api(index):
    with tempfile.TemporaryDirectory(prefix="api_write_test") as d:
        index_path = os.path.join(d, "foo")

        index.write(index_path)
        loaded_index = spatial_index.open_index(index_path)

        assert isinstance(loaded_index, type(index))


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology"],
        ["in_memory"],
    )
)
def test_index_write_api(element_kind, index_kind):
    index, _, _ = circuit_10_config(index_kind, element_kind)
    check_index_bounds_api(index)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind",
    itertools.product(
        ["synapse"],
        ["in_memory"],
    )
)
def test_index_write_sonata_api(element_kind, index_kind):
    index, _, _ = circuit_10_config(index_kind, element_kind)

    with tempfile.TemporaryDirectory(prefix="api_write_test") as d:

        fake_sonata_filename = "iwoeiweoruowie.h5"
        fake_population = "vhuweoiw"

        index_path = os.path.join(d, "foo")
        index.write(
            index_path, sonata_filename=fake_sonata_filename, population=fake_population
        )

        meta_data = spatial_index.io.MetaData(index_path)

        extended_conf = meta_data.extended

        assert extended_conf is not None
        assert fake_sonata_filename in extended_conf.path("dataset_path")
        assert fake_population in extended_conf.value("population")


def test_is_non_string_iterable():
    assert not is_non_string_iterable("")
    assert not is_non_string_iterable("foo")

    assert is_non_string_iterable([])
    assert is_non_string_iterable(["foo"])
    assert is_non_string_iterable(("foo",))
    assert is_non_string_iterable("foo" for _ in range(3))
