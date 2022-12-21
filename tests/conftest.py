import subprocess
import spatial_index.io
import os

TINY_DATA_DIR = "tests/data/tiny_circuits"
CIRCUIT_MORPH_DIR = os.path.join(TINY_DATA_DIR, "circuit-10")
CIRCUIT_SYN_DIR = os.path.join(TINY_DATA_DIR, "syn-2k")
INDEXES_MORPH_DIR = os.path.join(TINY_DATA_DIR, "circuit-10/indexes")
INDEXES_SYN_DIR = os.path.join(TINY_DATA_DIR, "syn-2k/indexes")
NO_SONATA_SYN_MEM = os.path.join(
    INDEXES_SYN_DIR, "synapse_no_sonata/in_memory/meta_data.json")
NO_SONATA_SYN_MULTI = os.path.join(
    INDEXES_SYN_DIR, "synapse_no_sonata/multi_index/meta_data.json")
NO_SONATA_MORPH_MEM = os.path.join(
    INDEXES_MORPH_DIR, "morphology_no_sonata/in_memory/meta_data.json")
NO_SONATA_MORPH_MULTI = os.path.join(
    INDEXES_MORPH_DIR, "morphology_no_sonata/multi_index/meta_data.json")


def generate_syn_index():
    subprocess.run(["bash", os.path.join(CIRCUIT_SYN_DIR, "generate.sh")])

    meta_data = spatial_index.io.read_json(NO_SONATA_SYN_MEM)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_SYN_MEM, meta_data)

    meta_data = spatial_index.io.read_json(NO_SONATA_SYN_MULTI)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_SYN_MULTI, meta_data)


def generate_morph_index():
    subprocess.run(["bash", os.path.join(CIRCUIT_MORPH_DIR, "generate.sh")])

    meta_data = spatial_index.io.read_json(NO_SONATA_MORPH_MEM)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_MORPH_MEM, meta_data)

    meta_data = spatial_index.io.read_json(NO_SONATA_MORPH_MULTI)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_MORPH_MULTI, meta_data)


def pytest_configure(config):

    config.addinivalue_line(
        "markers", "long: mark tests that take a while to run"
    )

    # Skip synapse index generation if it already exists
    # All test indexes will be regenerated and overwritten
    if not os.path.exists(os.path.join(INDEXES_SYN_DIR, "synapse/in_memory")):
        generate_syn_index()
        print("Synapse test indexes generated!")

    # Skip morphology index generation if it already exists
    # All test indexes will be regenerated and overwritten
    if not os.path.exists(os.path.join(INDEXES_MORPH_DIR, "morphology/in_memory")):
        generate_morph_index()
        print("Morph test indexes generated!")
