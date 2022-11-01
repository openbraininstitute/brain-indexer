import itertools
import os
import pytest
import numpy as np
import tempfile

import spatial_index

from spatial_index import open_index
from spatial_index.util import is_non_string_iterable

from spatial_index import MultiPopulationIndex


DATA_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v4"
CIRCUIT_10_DIR = os.path.join(DATA_DIR, "circuit-10")
CIRCUIT_1K_DIR = os.path.join(DATA_DIR, "circuit-1k")
USECASE_3_DIR = os.path.join(DATA_DIR, "sonata_usecases/usecase3")


def expected_builtin_fields(index):
    if index.element_type == "synapse":
        return ["id", "pre_gid", "post_gid", "position"]

    elif index.element_type == "morphology":
        return ["gid", "section_id", "segment_id",
                "ids", "centroid", "radius",
                "endpoints", "section_type",
                "is_soma"]

    elif index.element_type == "sphere":
        return ["id", "centroid", "radius"]

    else:
        raise RuntimeError(f"Broken test logic. [{type(index)}]")


def _wrap_assert_for_multi_population(func):
    def assert_valid(results, *args, expected_populations=None, **kwargs):
        if expected_populations is None:
            func(results, *args, **kwargs)

        else:
            assert isinstance(results, dict)
            assert sorted(results.keys()) == sorted(expected_populations)

            for single_result in results.values():
                func(single_result, *args, **kwargs)

    return assert_valid


@_wrap_assert_for_multi_population
def assert_valid_dict_result(results, expected_fields):
    assert isinstance(results, dict)
    assert sorted(results.keys()) == sorted(expected_fields)

    for field in expected_fields:
        assert_valid_single_result(results[field], field)

    def length(r, field):
        if field == "endpoints":
            p1, p2 = r
            assert p1.shape == p2.shape
            return p1.shape[0]
        else:
            return r.shape[0]

    lengths = set(length(r, f) for f, r in results.items())
    assert len(lengths) == 1, results
    assert next(iter(lengths)) > 0


@_wrap_assert_for_multi_population
def assert_valid_single_result(result, field):
    if field == "endpoints":
        p1, p2 = result
        assert isinstance(p1, np.ndarray), p1
        assert isinstance(p2, np.ndarray), p1
        assert p1.shape == p2.shape
        assert p1.shape[0] > 0
        assert p1.shape[1] == 3

    elif field == "raw_elements":
        assert isinstance(result, (list, np.ndarray)), result

    else:
        assert isinstance(result, np.ndarray), result
        assert len(result) > 0


def _wrap_check_for_multi_population(check_single_population):
    """Wrap single population check to cover the multi-population case.

    This augments the check with `population_mode` and intersepts it. This
    refers to the `population_mode` of the validation, i.e. we expect queries to
    be formatted according to this population mode. The keyword argument for the
    query itself is contained in `query_kwargs`.

    This further augments with and intersepts `populations`, which are the population
    to try.
    """

    def _wrap_func(*args, query_kwargs=None, populations=None, population_mode=None,
                   **kwargs):

        for pop in populations:
            qkw = {
                **query_kwargs,
                "populations": pop
            }

            if population_mode == "single":
                expected_populations = None
            elif population_mode == "multi":
                expected_populations = [pop]
            else:
                raise ValueError(f"Invalid `{population_mode=}`.")

            check_single_population(
                *args, query_kwargs=qkw, **kwargs,
                expected_populations=expected_populations
            )

        if len(populations) > 1:
            qkw = {
                **query_kwargs,
                "populations": populations
            }

            if population_mode == "single":
                with pytest.raises(ValueError):

                    check_single_population(
                        *args, query_kwargs=qkw, **kwargs,
                        expected_populations=populations
                    )
            else:
                check_single_population(
                    *args, query_kwargs=qkw, **kwargs,
                    expected_populations=populations
                )

    return _wrap_func


@_wrap_check_for_multi_population
def check_query(query, query_shape, *, query_kwargs=None, available_fields=None,
                builtin_fields=None, expected_populations=None):
    special_fields = ["raw_elements"]

    all_fields = available_fields + special_fields

    for field in all_fields:
        result = query(*query_shape, fields=field, **query_kwargs)
        assert_valid_single_result(
            result, field, expected_populations=expected_populations
        )

    for k in range(1, len(builtin_fields) + 1):
        for fields in itertools.combinations(builtin_fields, k):
            result = query(*query_shape, fields=fields, **query_kwargs)
            assert_valid_dict_result(
                result, fields, expected_populations=expected_populations
            )

    result = query(*query_shape, fields=available_fields, **query_kwargs)
    assert_valid_dict_result(
        result, available_fields, expected_populations=expected_populations
    )

    for field in special_fields:
        with pytest.raises(Exception):
            query(*query_shape, fields=[field], **query_kwargs)

    for field in ["", [], [""]]:
        with pytest.raises(Exception):
            query(*query_shape, fields=field, **query_kwargs)

    results = query(*query_shape, fields=None, **query_kwargs)
    assert_valid_dict_result(
        results, builtin_fields, expected_populations=expected_populations
    )


@_wrap_assert_for_multi_population
def assert_valid_counts(counts):
    assert counts > 0


@_wrap_assert_for_multi_population
def assert_valid_empty(result):
    assert result is False


@_wrap_check_for_multi_population
def check_counts(counts_method, query_shape, query_kwargs=None,
                 expected_populations=None):

    counts = counts_method(*query_shape, group_by=None, **query_kwargs)
    assert_valid_counts(counts, expected_populations=expected_populations)


@_wrap_check_for_multi_population
def check_empty(empty_method, query_shape, query_kwargs=None,
                expected_populations=None):

    result = empty_method(*query_shape, **query_kwargs)
    assert_valid_empty(result, expected_populations=expected_populations)


def check_element_type(index):
    isinstance(index.element_type, str)


def check_generic_api(index):
    check_builtin_fields(index)
    check_element_type(index)


def check_builtin_fields(index):
    expected = expected_builtin_fields(index)
    actual = index.builtin_fields

    assert sorted(actual) == sorted(expected)


@_wrap_assert_for_multi_population
def assert_valid_bounds(bounds, expected_populations=None):
    min_corner, max_corner = bounds
    assert isinstance(min_corner, np.ndarray)
    assert min_corner.dtype == np.float32

    assert isinstance(max_corner, np.ndarray)
    assert max_corner.dtype == np.float32

    assert np.all(min_corner < max_corner)


@_wrap_check_for_multi_population
def check_index_bounds(bounds_method, query_kwargs=None, expected_populations=None):
    bounds = bounds_method(**query_kwargs)
    assert_valid_bounds(bounds, expected_populations=expected_populations)


def check_index_bounds_api(index, population_mode):
    populations = index.populations
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_index_bounds(
        index.bounds,
        query_kwargs={"population_mode": population_mode},
        populations=populations,
        population_mode=expected_population_mode
    )


def deduce_expected_population_mode(index, population_mode):
    if population_mode is None:
        return "multi" if isinstance(index, MultiPopulationIndex) else "single"
    else:
        return population_mode


def check_all_regular_query_api(index, window, sphere, accuracy, population_mode):
    query_kwargs = {"accuracy": accuracy, "population_mode": population_mode}

    populations = index.populations
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_query(
        index.box_query, window, query_kwargs=query_kwargs,
        available_fields=index.available_fields,
        builtin_fields=index.builtin_fields,
        populations=populations,
        population_mode=expected_population_mode,
    )

    reversed_window = (window[1], window[0])
    check_query(
        index.box_query, reversed_window, query_kwargs=query_kwargs,
        available_fields=index.available_fields,
        builtin_fields=index.builtin_fields,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_query(
        index.sphere_query, sphere, query_kwargs=query_kwargs,
        available_fields=index.available_fields,
        builtin_fields=index.builtin_fields,
        populations=populations,
        population_mode=expected_population_mode,
    )


def check_all_counts_api(index, window, sphere, accuracy, population_mode):
    query_kwargs = {"accuracy": accuracy, "population_mode": population_mode}

    populations = index.populations
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_counts(
        index.box_counts, window, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )

    reversed_window = (window[1], window[0])
    check_counts(
        index.box_counts, reversed_window, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_counts(
        index.sphere_counts, sphere, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_empty(
        index.box_empty, window, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_empty(
        index.box_empty, reversed_window, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_empty(
        index.sphere_empty, sphere, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )


def check_all_index_api(index, window, sphere, accuracy, population_mode):
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_all_regular_query_api(
        index, window, sphere, accuracy,
        population_mode=expected_population_mode
    )
    check_all_counts_api(index, window, sphere, accuracy, population_mode)
    check_index_bounds_api(index, population_mode)
    check_generic_api(index)


def circuit_10_config(index_variant, element_type):
    if "synapse" in element_type:
        index_path = os.path.join(
            CIRCUIT_1K_DIR, f"indexes/{element_type}/{index_variant}"
        )

    elif "morphology" in element_type:
        index_path = os.path.join(
            CIRCUIT_10_DIR, f"indexes/{element_type}/{index_variant}"
        )

    else:
        raise ValueError(f"Invalid {element_type = }.")

    index = open_index(index_path)

    # We know there's a segment at:
    #   [ 116.732, 1957.872, 24.925]
    # and seems to also work for the synapse index.
    window = ([50.0, 1800.0, 10.0], [130.0, 2000.0, 50.0])
    sphere = ([115.0, 1950.0, 25.0], 100.0)

    return index, window, sphere


def spheres_config(index_variant):
    centroids = np.random.uniform(size=(10, 3)).astype(np.float32)
    radii = np.random.uniform(size=10).astype(np.float32)
    index = spatial_index.SphereIndexBuilder.from_numpy(centroids, radii)

    window = [0.0, 0.0, 0.0], [1.0, 1.0, 1.0]
    sphere = [0.5, 0.5, 0.5], 0.5

    return index, window, sphere


def usecase_3_config(index_variant, element_type):
    index_path = os.path.join(USECASE_3_DIR, f"indexes/{element_type}/{index_variant}")
    index = open_index(index_path)

    # We know there's only very few elements. Hence oversize boxes are fine.
    window = ([-1000.0, -1000.0, -1000.0], [1000.0, 1000.0, 1000.0])
    sphere = ([0.0, 0.0, 0.0], 1000.0)

    return index, window, sphere


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_type,index_variant,accuracy,population_mode",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology", "morphology_no_sonata"],
        ["in_memory", "multi_index"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_index_api(element_type, index_variant, accuracy, population_mode):
    index, window, sphere = circuit_10_config(index_variant, element_type)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


@pytest.mark.parametrize(
    "element_type,index_variant,accuracy,population_mode",
    itertools.product(
        ["sphere"],
        ["in_memory"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_sphere_index_query_api(element_type, index_variant, accuracy, population_mode):
    index, window, sphere = spheres_config(index_variant)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


@pytest.mark.skipif(not os.path.exists(USECASE_3_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_type,index_variant,accuracy,population_mode",
    itertools.product(
        ["synapse", "morphology"],
        ["in_memory"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_multi_population_index_api(element_type, index_variant, accuracy,
                                    population_mode):

    index, window, sphere = usecase_3_config(index_variant, element_type)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_type,index_variant",
    itertools.product(
        ["synapse", "morphology"],
        ["in_memory"],
    )
)
def test_index_write_api(element_type, index_variant):
    index, _, _ = circuit_10_config(index_variant, element_type)

    with tempfile.TemporaryDirectory(prefix="api_write_test") as d:
        index_path = os.path.join(d, "foo")

        index.write(index_path)
        loaded_index = spatial_index.open_index(index_path)

        assert isinstance(loaded_index, type(index))


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_type,index_variant",
    itertools.product(
        ["synapse", "morphology"],
        ["in_memory"],
    )
)
def test_index_write_sonata_api(element_type, index_variant):
    index, _, _ = circuit_10_config(index_variant, element_type)

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
