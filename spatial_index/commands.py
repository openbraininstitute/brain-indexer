"""
    High level command line commands
"""
import logging
from .node_indexer import MorphIndexBuilder
from .util import check_free_space, docopt_get_args, get_file_path


def spatial_index_nodes(args=None):
    """spatial-index-nodes

    Usage:
        spatial-index-nodes [options] <nodes-file> <morphology-dir>
        spatial-index-nodes --help

    Options:
        -v, --verbose            Increase verbosity level
        -o, --out=<filename>     The index output filename [default:out.spi]
        --use-mem-map=<SIZE_MB>  Whether to use a mapped file instead [experimental]
        --shrink-on-close        Whether to shrink the memory file upon closing the object
    """
    options = docopt_get_args(spatial_index_nodes, args)
    filename = options.get("out") or "out.spi"
    use_mem_map = options.get("use_mem_map")
    mem_map_props = None

    if use_mem_map:
        logging.warning("Using experimental memory-mapped-file")
        fsize = int(use_mem_map)
        if not check_free_space(fsize * 1048576, get_file_path(filename)):
            print("Not enough free space to create a memory-mapped file of size",
                  fsize, "MB!")
            return 1
        shrink = options["shrink_on_close"]
        mem_map_props = MorphIndexBuilder.DiskMemMapProps(filename, fsize, shrink)

    index = MorphIndexBuilder.create(
        options["morphology_dir"],
        options["nodes_file"],
        mem_map_props=mem_map_props,
        progress=True
    )

    if not mem_map_props:
        index.dump(filename)


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

    index = MorphIndexBuilder.create(morphology_dir, nodes_file, cells)
    index.dump(options.get("out") or "out.spi")


def _get_circuit_cells(circuit, target):
    if target is None:
        return circuit.cells.ids()
    return circuit.cells.ids(target)
