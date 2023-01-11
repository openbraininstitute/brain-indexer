import pytest

from spatial_index.util import is_strictly_sensible_filename
from spatial_index.util import strip_singleton_non_string_iterable
from spatial_index.util import factor


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


def test_factor():
    # Test if the function correctly returns the two factors of a perfect square
    assert factor(4, dims=2) == (2, 2)
    assert factor(9, dims=2) == (3, 3)
    assert factor(25, dims=2) == (5, 5)
    # Test if the function correctly returns two factors that are close to each other
    assert factor(10, dims=2) == (2, 5)
    assert factor(21, dims=2) == (3, 7)
    # Test if the function correctly returns any two factors for an arbitrary number
    assert factor(8, dims=2) == (2, 4)
    assert factor(7 * 661, dims=2) == (7, 661)
    # Test if the function correctly handles prime numbers
    assert factor(2, dims=2) == (1, 2)
    assert factor(7, dims=2) == (1, 7)
    assert factor(17, dims=2) == (1, 17)
    # Test if the function correctly handles the number 1
    assert factor(1, dims=2) == (1, 1)
    # Test if the function correctly handles the number 0
    assert factor(0, dims=2) == (0, 1)


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
