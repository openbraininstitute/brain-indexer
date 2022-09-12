import itertools
import os
import pytest
import numpy as np
import tempfile

import spatial_index

from spatial_index import open_index
from spatial_index.util import is_non_string_iterable

from spatial_index import MultiPopulationIndex


DATA_DIR = "/gpfs/bbp.cscs.ch/project/proj12/spatial_index/v1"
CIRCUIT_10_DIR = os.path.join(DATA_DIR, "circuit-10")
CIRCUIT_1K_DIR = os.path.join(DATA_DIR, "circuit-1k")
USECASE_3_DIR = os.path.join(DATA_DIR, "sonata_usecases/usecase3")


def expected_builtin_fields(index):
    if index._element_type == "synapse":
        return ["id", "pre_gid", "post_gid", "position"]

    elif index._element_type == "morphology":
        return ["gid", "section_id", "segment_id",
                "ids", "centroid",
                "radius", "endpoint1", "endpoint2",
                "is_soma"]

    elif index._element_type == "sphere":
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
        assert isinstance(results[field], np.ndarray)

    lengths = set(r.shape[0] for r in results.values())
    assert len(lengths) == 1, results
    assert next(iter(lengths)) > 0


@_wrap_assert_for_multi_population
def assert_valid_single_result(result):
    assert isinstance(result, (list, np.ndarray)), result
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
def check_query(query, query_shape, *, query_kwargs=None, builtin_fields=None,
                expected_populations=None):
    special_fields = ["raw_elements"]
    all_fields = builtin_fields + special_fields

    for field in all_fields:
        result = query(*query_shape, fields=field, **query_kwargs)
        assert_valid_single_result(result, expected_populations=expected_populations)

    for k in [1, len(builtin_fields) + 1]:
        for fields in itertools.combinations(builtin_fields, k):
            result = query(*query_shape, fields=fields, **query_kwargs)
            assert_valid_dict_result(
                result, fields, expected_populations=expected_populations
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


@_wrap_check_for_multi_population
def check_counts(counts_method, query_shape, query_kwargs=None,
                 expected_populations=None):

    counts = counts_method(*query_shape, group_by=None, **query_kwargs)
    assert_valid_counts(counts, expected_populations=expected_populations)


def check_generic_api(index):
    check_builtin_fields(index)


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
    builtin_fields = index.builtin_fields

    populations = index.populations
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_query(
        index.window_query, window, query_kwargs=query_kwargs,
        builtin_fields=builtin_fields,
        populations=populations,
        population_mode=expected_population_mode,
    )

    check_query(
        index.vicinity_query, sphere, query_kwargs=query_kwargs,
        builtin_fields=builtin_fields,
        populations=populations,
        population_mode=expected_population_mode,
    )


def check_all_counts_api(index, window, sphere, accuracy, population_mode):
    query_kwargs = {"accuracy": accuracy, "population_mode": population_mode}

    populations = index.populations
    expected_population_mode = deduce_expected_population_mode(index, population_mode)

    check_counts(
        index.window_counts, window, query_kwargs=query_kwargs,
        populations=populations,
        population_mode=expected_population_mode,
    )
    check_counts(
        index.vicinity_counts, sphere, query_kwargs=query_kwargs,
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


def usecase_3_config(index_kind, element_kind):
    index_path = os.path.join(USECASE_3_DIR, f"indexes/{element_kind}/{index_kind}")
    index = open_index(index_path)

    # We know there's only very few elements. Hence oversize boxes are fine.
    window = ([-1000.0, -1000.0, -1000.0], [1000.0, 1000.0, 1000.0])
    sphere = ([0.0, 0.0, 0.0], 1000.0)

    return index, window, sphere


@pytest.mark.skipif(not os.path.exists(CIRCUIT_10_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy,population_mode",
    itertools.product(
        ["synapse", "synapse_no_sonata", "morphology"],
        ["in_memory", "multi_index"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_index_api(element_kind, index_kind, accuracy, population_mode):
    index, window, sphere = circuit_10_config(index_kind, element_kind)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy,population_mode",
    itertools.product(
        ["sphere"],
        ["in_memory"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_sphere_index_query_api(element_kind, index_kind, accuracy, population_mode):
    index, window, sphere = spheres_config(index_kind)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


@pytest.mark.skipif(not os.path.exists(USECASE_3_DIR),
                    reason="Circuit directory not available")
@pytest.mark.parametrize(
    "element_kind,index_kind,accuracy,population_mode",
    itertools.product(
        ["synapse", "morphology"],
        ["in_memory"],
        [None, "bounding_box", "best_effort"],
        [None, "single", "multi"]
    )
)
def test_multi_population_index_api(element_kind, index_kind, accuracy, population_mode):
    index, window, sphere = usecase_3_config(index_kind, element_kind)
    check_all_index_api(index, window, sphere, accuracy, population_mode)


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
