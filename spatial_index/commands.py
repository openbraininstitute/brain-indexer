"""
    High level command line commands
"""
import logging
from .node_indexer import MorphIndexBuilder
from .synapse_indexer import SynapseIndexBuilder
from .util import DiskMemMapProps, check_free_space, docopt_get_args, get_dirname


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
        --populations=<populations>...  Restrict the spatial index to the listed
                                        Currently, at most one population is supported
    """

    import logging
    logging.basicConfig(level=logging.INFO)

    options = docopt_get_args(spatial_index_circuit, args)
    config = _sonata_expanded_circuit_config(options["circuit_file"])
    population = _validated_population(options)

    if options['segments']:
        assert population is None, "Segment indices don't support populations."
        nodes_file = _sonata_nodes_file(config, population)
        morphology_dir = _sonata_morphology_dir(config)
        _run_spatial_index_nodes(morphology_dir, nodes_file, options)

    elif options['synapses']:
        edges_file = _sonata_edges_file(config, population)
        _run_spatial_index_synapses(edges_file, population, options)


def _validated_population(options):
    populations = options["populations"]

    error_msg = "At most one population is supported."
    assert populations is None or len(populations) <= 1, error_msg
    return populations[0] if populations else None


def _sonata_select_by_population(iterable, key, population):
    def matches_population(n):
        return population is None or population in n.get("populations", dict())

    selection = [n[key] for n in iterable if matches_population(n)]
    assert len(selection) == 1, "Couln't determine a unique '{}'.".format(key)

    return selection[0]


def _sonata_expanded_circuit_config(config_file):
    import libsonata
    import json

    config = libsonata.CircuitConfig.from_file(config_file)
    return json.loads(config.expanded_json)


def _sonata_nodes_file(config, population):
    nodes = config["networks"]["nodes"]
    return _sonata_select_by_population(nodes, key="nodes_file", population=population)


def _sonata_edges_file(config, population):
    edges = config["networks"]["edges"]
    return _sonata_select_by_population(edges, key="edges_file", population=population)


def _sonata_morphology_dir(config):
    return config["components"]["morphologies_dir"]


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
    disk_mem_map = _parse_mem_map_options(options)

    index = MorphIndexBuilder.create(
        morphology_dir,
        nodes_file,
        disk_mem_map=disk_mem_map,
        progress=True
    )

    if not disk_mem_map:
        logging.info("Writing index to file: %s", options["out"])
        index.dump(options["out"])


def _run_spatial_index_synapses(edges_file, population, options):
    disk_mem_map = _parse_mem_map_options(options)
    index = SynapseIndexBuilder.from_sonata_file(
        edges_file,
        population,
        disk_mem_map=disk_mem_map,
        progress=True
    )

    if not disk_mem_map:
        logging.info("Writing index to file: %s", options["out"])
        index.dump(options["out"])
