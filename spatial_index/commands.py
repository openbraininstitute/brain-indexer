"""
    High level command line commands
"""

import logging

from .index_common import DiskMemMapProps
from .node_indexer import MorphIndexBuilder
from .synapse_indexer import SynapseIndexBuilder
from .util import check_free_space, docopt_get_args, get_dirname, is_likely_same_index
from .io import open_index


def spatial_index_nodes(args=None):
    """spatial-index-nodes

    Usage:
        spatial-index-nodes [options] <nodes-file> <morphology-dir>
        spatial-index-nodes --help

    Options:
        -v, --verbose            Increase verbosity level
        -o, --out=<filename>     The index output filename [default: out.spi]
        --use-mem-map=<SIZE_MB>  Whether to use a mapped file instead [experimental]
        --shrink-on-close        Whether to shrink the memory file upon closing the object
        --multi-index            Whether to create a multi-index
    """
    options = docopt_get_args(spatial_index_nodes, args)
    _run_spatial_index_nodes(options["morphology_dir"], options["nodes_file"], options)


def spatial_index_synapses(args=None):
    """spatial-index-synapses

    Usage:
        spatial-index-synapses [options] <edges_file> [<population>]
        spatial-index-synapses --help

    Options:
        -v, --verbose            Increase verbosity level
        -o, --out=<filename>     The index output filename [default: out.spi]
        --use-mem-map=<SIZE_MB>  Whether to use a mapped file instead [experimental]
        --shrink-on-close        Whether to shrink the memory file upon closing the object
        --multi-index            Whether to create a multi-index
    """
    options = docopt_get_args(spatial_index_synapses, args)
    _run_spatial_index_synapses(options["edges_file"], options.get("population"), options)


def spatial_index_circuit(args=None):
    """spatial-index-circuit

    Create an index for the circuit defined by a SONATA circuit config. The
    index can either be a segment index or a synapse index.

    The segment index expects the SONATA config to provide:
        components/morphologies_dir
        networks/nodes

    For a synapse index we expect the SONATA config to provide
        networks/edges

    Currently, only a single population is supported. Therefore, the
    'networks/{nodes,edges}' must identify a unique file. This is possible
    if either the list only contains one dictionary, or, if a population has
    been selected, only one file matches the specified population.

    Note: requires libsonata

    Usage:
        spatial-index-circuit segments <circuit-file> [options]
        spatial-index-circuit synapses <circuit-file> [options]
        spatial-index-circuit --help

    Options:
        -o, --out=<out_file>     The index output filename [default: out.spi]
        --use-mem-map=<SIZE_MB>  Whether to use a mapped file instead (experimental)
        --shrink-on-close        Whether to shrink the memory file upon closing the object
        --multi-index            Whether to create a multi-index
        --populations=<populations>...  Restrict the spatial index to the listed
                                        Currently, at most one population is supported
    """

    import logging
    logging.basicConfig(level=logging.INFO)

    options = docopt_get_args(spatial_index_circuit, args)
    circuit_config, json_config = _sonata_circuit_config(options["circuit_file"])
    population = _validated_population(circuit_config, options)

    if options['segments']:
        nodes_file = _sonata_nodes_file(json_config, population)
        morphology_dir = _sonata_morphology_dir(circuit_config, population)
        _run_spatial_index_nodes(morphology_dir, nodes_file, options)

    elif options['synapses']:
        edges_file = _sonata_edges_file(json_config, population)
        _run_spatial_index_synapses(edges_file, population, options)

    else:
        raise NotImplementedError("Missing subcommand.")


def spatial_index_compare(args=None):
    """spatial-index-compare

    Compares two circuits and returns with a non-zero exit code
    if a difference was detect. Otherwise the exit code is zero.

    Usage:
        spatial-index-compare <lhs-circuit> <rhs-circuit>
    """
    options = docopt_get_args(spatial_index_compare, args)

    lhs = open_index(options["lhs_circuit"])
    rhs = open_index(options["rhs_circuit"])

    if not is_likely_same_index(lhs, rhs):
        logging.info("The two indexes differ.")
        exit(-1)


def _validated_population(circuit_config, options):
    populations = options["populations"]

    if options["segments"]:
        available_populations = circuit_config.node_populations

    elif options["synapses"]:
        available_populations = circuit_config.edge_populations

    else:
        raise NotImplementedError("Missing circuit kind.")

    if populations is None:
        populations = available_populations

    error_msg = "At most one population is supported."
    assert len(populations) == 1, error_msg

    return next(iter(populations))


def _sonata_select_by_population(iterable, key, population):
    def matches_population(n):
        return population == "All" or population in n.get("populations", dict())

    selection = [n[key] for n in iterable if matches_population(n)]
    assert len(selection) == 1, "Couln't determine a unique '{}'.".format(key)

    return selection[0]


def _sonata_circuit_config(config_file):
    import libsonata
    import json

    circuit_config = libsonata.CircuitConfig.from_file(config_file)
    json_config = json.loads(circuit_config.expanded_json)

    return circuit_config, json_config


def _sonata_nodes_file(config, population):
    nodes = config["networks"]["nodes"]
    return _sonata_select_by_population(nodes, key="nodes_file", population=population)


def _sonata_edges_file(config, population):
    edges = config["networks"]["edges"]
    return _sonata_select_by_population(edges, key="edges_file", population=population)


def _sonata_morphology_dir(config, population):
    node_prop = config.node_population_properties(population)
    return node_prop.morphologies_dir


def _parse_mem_map_options(options: dict) -> DiskMemMapProps:
    """Builds the disk memory-map properties object from CLI args"""

    use_mem_map = options.get("use_mem_map")
    filename = options["out"]
    if not use_mem_map:
        return

    logging.warning("Using experimental memory-mapped-file")
    fsize = int(use_mem_map)

    if not check_free_space(fsize * 1024 * 1024, get_dirname(filename)):
        raise Exception(
            f"Not enough free space to create a memory-mapped file of size {fsize} MB")

    return MorphIndexBuilder.DiskMemMapProps(filename, fsize, options["shrink_on_close"])


def _run_spatial_index_nodes(morphology_dir, nodes_file, options):
    try:
        from spatial_index import MorphMultiIndexBuilder
    except ModuleNotFoundError as e:
        logging.error("SpatialIndex was likely not built with MPI support.")
        raise e

    if options["multi_index"]:
        MorphMultiIndexBuilder.create(
            morphology_dir,
            nodes_file,
            output_dir=options["out"],
        )

    else:
        disk_mem_map = _parse_mem_map_options(options)
        index = MorphIndexBuilder.create(
            morphology_dir,
            nodes_file,
            disk_mem_map=disk_mem_map,
            progress=True
        )

        if not disk_mem_map:
            logging.info("Writing index to file: %s", options["out"])
            index.index.dump(options["out"])


def _run_spatial_index_synapses(edges_file, population, options):
    try:
        from spatial_index import SynapseMultiIndexBuilder
    except ModuleNotFoundError as e:
        logging.error("SpatialIndex was likely not built with MPI support.")
        raise e

    if options["multi_index"]:
        SynapseMultiIndexBuilder.from_sonata_file(
            edges_file,
            population,
            output_dir=options["out"]
        )

    else:
        disk_mem_map = _parse_mem_map_options(options)
        index = SynapseIndexBuilder.from_sonata_file(
            edges_file,
            population,
            disk_mem_map=disk_mem_map,
            progress=True
        )

        if not disk_mem_map:
            logging.info("Writing index to file: %s", options["out"])
            index.index.dump(options["out"])
