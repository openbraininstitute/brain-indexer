"""
    High level command line commands
"""

import os

import spatial_index

from .util import docopt_get_args, is_likely_same_index
from .util import is_strictly_sensible_filename, is_non_string_iterable
from .io import write_multi_population_meta_data
from .resolver import open_index, MorphIndexResolver, SynapseIndexResolver


def spatial_index_nodes(args=None):
    """spatial-index-nodes

    Usage:
        spatial-index-nodes [options] <nodes-file> <morphology-dir>
        spatial-index-nodes --help

    Options:
        -v, --verbose            Increase verbosity level
        -o, --out=<folder>       The index output folder [default: out]
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
        -o, --out=<folder>       The index output folder [default: out]
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
                              [(--populations <populations>) [<populations>...]]
        spatial-index-circuit synapses <circuit-file> [options]
                              [(--populations <populations>) [<populations>...]]
        spatial-index-circuit --help

    Options:
        -o, --out=<out_file>     The index output folder [default: out]
        --multi-index            Whether to create a multi-index
    """
    options = docopt_get_args(spatial_index_circuit, args)
    circuit_config, json_config = _sonata_circuit_config(options["circuit_file"])
    populations = _validated_populations(options, circuit_config)

    if is_non_string_iterable(populations):
        _spatial_index_circuit_multi_population(
            options, circuit_config, json_config, populations
        )
    else:
        output_dir = options["out"]
        _spatial_index_circuit_single_population(
            options, circuit_config, json_config, populations, output_dir
        )


def _spatial_index_circuit_single_population(options, circuit_config, json_config,
                                             population, output_dir):
    if options['segments']:
        nodes_file = _sonata_nodes_file(json_config, population)
        morphology_dir = _sonata_morphology_dir(circuit_config, population)
        _run_spatial_index_sonata_nodes(
            morphology_dir, nodes_file, population, options, output_dir=output_dir
        )

    elif options['synapses']:
        edges_file = _sonata_edges_file(json_config, population)
        _run_spatial_index_synapses(
            edges_file, population, options, output_dir=output_dir
        )

    else:
        raise NotImplementedError("Missing subcommand.")


def _spatial_index_circuit_multi_population(options, circuit_config, json_config,
                                            populations):
    basedir = options["out"]

    for pop in populations:
        output_dir = os.path.join(basedir, pop)
        _spatial_index_circuit_single_population(
            options, circuit_config, json_config, pop, output_dir=output_dir
        )

    element_type = "synpase" if options["synapses"] else "morphology"
    write_multi_population_meta_data(basedir, element_type, populations)


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
        spatial_index.logger.info("The two indexes differ.")
        exit(-1)


def _sonata_available_populations(options, circuit_config):
    if options["segments"]:
        available_populations = circuit_config.node_populations

    elif options["synapses"]:
        available_populations = circuit_config.edge_populations

    else:
        raise NotImplementedError("Missing circuit kind.")

    return available_populations


def _validated_single_population(options, circuit_config, population):
    available_populations = _sonata_available_populations(options, circuit_config)

    assert population in available_populations, f"{population=}, {available_populations=}"
    assert is_strictly_sensible_filename(population)

    return population


def _validated_populations(options, circuit_config):
    populations = options["populations"]

    if not populations:
        populations = _sonata_available_populations(options, circuit_config)

        error_msg = "At most one population is supported."
        assert len(populations) == 1, error_msg

        populations = next(iter(populations))

    if is_non_string_iterable(populations):
        for pop in populations:
            _validated_single_population(options, circuit_config, pop)

    else:
        _validated_single_population(options, circuit_config, populations)

    return populations


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


def _parse_options_for_builder_args(options, output_dir):
    if options["multi_index"]:
        index_kind = "multi_index"
        index_kwargs = {}
    else:
        index_kind = "in_memory"
        index_kwargs = {"progress": True}

    if output_dir is None:
        output_dir = options["out"]

    index_kwargs["output_dir"] = output_dir

    return index_kind, index_kwargs


def _run_spatial_index_nodes(morphology_dir, nodes_file, options, output_dir=None):
    index_kind, index_kwargs = _parse_options_for_builder_args(options, output_dir)

    Builder = MorphIndexResolver.builder_class(index_kind)
    Builder.create(
        morphology_dir, nodes_file, **index_kwargs
    )


def _run_spatial_index_sonata_nodes(morphology_dir, nodes_file, population, options,
                                    output_dir=None):
    index_kind, index_kwargs = _parse_options_for_builder_args(options, output_dir)

    Builder = MorphIndexResolver.builder_class(index_kind)
    Builder.from_sonata_file(
        morphology_dir, nodes_file, population, **index_kwargs
    )


def _run_spatial_index_synapses(edges_file, population, options, output_dir=None):
    index_kind, index_kwargs = _parse_options_for_builder_args(options, output_dir)

    Builder = SynapseIndexResolver.builder_class(index_kind)
    Builder.from_sonata_file(
        edges_file, population, **index_kwargs
    )
