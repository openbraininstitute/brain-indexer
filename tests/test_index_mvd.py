import subprocess
import os.path

from spatial_index import circuit_indexer


MORPHOLOGIES = "TestData/circuitBuilding_1000neurons/morphologies/ascii"
FILENAME = "TestData/circuitBuilding_1000neurons/circuits/circuit.mvd3"


def setup():
    if not os.path.isdir("TestData"):
        subprocess.run(("git", "clone", "bbpcode.epfl.ch/common/TestData"))


def test_serial_exec():
    circuit_indexer.main_serial(MORPHOLOGIES)
