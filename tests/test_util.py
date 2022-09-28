import pytest

from spatial_index.util import is_strictly_sensible_filename
from spatial_index.util import strip_singleton_non_string_iterable


def test_strictly_sensible_filename():
    test_cases = [
        ("", False),
        ("foo", True),
        ("fO.o", True),
        ("f_0-", True),
        ("fo o", False),
        ("f/oo", False),
        ("f\\oo", False),
        ("fo?o", False),
        ("fo!o", False),
        ("fo\no", False),
    ]

    for filename, expected in test_cases:
        assert is_strictly_sensible_filename(filename) == expected


def test_strip_singleton_non_string_iterable():
    good_test_cases = [
        ("foo", "foo"),
        (None, None),
        ("", ""),
        (["foo"], "foo"),
        ([None], None)
    ]

    for arg, expected in good_test_cases:
        assert strip_singleton_non_string_iterable(arg) == expected

    bad_test_cases = [
        ["foo", None]
    ]

    for arg in bad_test_cases:
        with pytest.raises(ValueError):
            strip_singleton_non_string_iterable(arg)


@pytest.mark.mpi(min_size=2)
@pytest.mark.xfail
@pytest.mark.parametrize("with_hook", [True, False])
def test_mpi_with_excepthook(with_hook):
    # The purpose of this test is to check if pytest hangs if only a single
    # MPI rank raises an exception.

    from mpi4py import MPI

    if with_hook:
        from spatial_index.util import register_mpi_excepthook
        register_mpi_excepthook()

    if MPI.COMM_WORLD.Get_rank() == 0:
        raise ValueError("Rank == 0")
