import numpy as np
from libsonata import Selection


def check_create_sonata_selection(ranges):
    expected = [(i, j) for i, j in ranges]

    selection = Selection(ranges)
    assert selection.ranges == expected


def test_create_sonata_selection():
    ranges = [(1, 4), (6, 9)]
    check_create_sonata_selection(ranges)
    check_create_sonata_selection(np.array(ranges))
