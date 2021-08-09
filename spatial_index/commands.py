"""
    High level command line commands
"""

from .node_indexer import NodeMorphIndexer
from .util import docopt_get_args


def spatial_index_nodes(args=None):
    """spatial-index-nodes

    Usage:
        spatial-index-nodes <nodes-file> <morphology-dir> [--out=<filename>]
        spatial-index-nodes --help

    Options:
        -v, --verbose            Increase verbosity level
        -o, --out=<filename>     The index output filename [default:out.spi]
    """
    options = docopt_get_args(spatial_index_nodes, args)
    index = NodeMorphIndexer.create(options["morphology_dir"], options["nodes_file"])
    index.dump(options.get("out") or "out.spi")


def spatial_index_circuit(args=None):
    """spatial-index-circuit

    Indexes all cells from a circuit/target given a BlueConfig.
    Note: requires BluePy

    Usage:
        spatial-index-circuit <circuit-file> [<target>] [--out=<out_file>]
        spatial-index-circuit --help

    Options:
        -o --out=<out_file>     The index output filename [default:out.spi]
    """
    import logging
    try:
        from bluepy import Circuit
    except ImportError:
        print("Error: indexing a circuit requires bluepy")
        return 1

    options = docopt_get_args(spatial_index_circuit, args)
    logging.basicConfig(level=logging.INFO)
    circuit = Circuit(options["circuit_file"])

    nodes_file = circuit.config["cells"]
    morphology_dir = circuit.config["morphologies"] + "/ascii"
    target = options.get("target") or circuit.config.get("CircuitTarget")
    if not target:
        print("Target not specified, will index the whole circuit")
        cells = None
    else:
        cells = _get_circuit_cells(circuit, target)
        print("Indexing", target, "containing", len(cells), "cells")

    index = NodeMorphIndexer.create(morphology_dir, nodes_file, cells)
    index.dump(options.get("out") or "out.spi")


def _get_circuit_cells(circuit, target):
    if target is None:
        return circuit.cells.ids()
    return circuit.cells.ids(target)
