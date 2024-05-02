import subprocess
from pathlib import Path

import spatial_index.io

TINY_DATA_DIR = Path(__file__).parent / "data/tiny_circuits"
CIRCUIT_MORPH_DIR = TINY_DATA_DIR / "circuit-10"
CIRCUIT_SYN_DIR = TINY_DATA_DIR / "syn-2k"
INDEXES_MORPH_DIR = TINY_DATA_DIR / "circuit-10/indexes"
INDEXES_SYN_DIR = TINY_DATA_DIR / "syn-2k/indexes"
NO_SONATA_SYN_MEM = INDEXES_SYN_DIR / "synapse_no_sonata/in_memory/meta_data.json"
NO_SONATA_SYN_MULTI = INDEXES_SYN_DIR / "synapse_no_sonata/multi_index/meta_data.json"
NO_SONATA_MORPH_MEM = INDEXES_MORPH_DIR / "morphology_no_sonata/in_memory/meta_data.json"
NO_SONATA_MORPH_MULTI = (
    INDEXES_MORPH_DIR / "morphology_no_sonata/multi_index/meta_data.json"
)


def generate_syn_index():
    subprocess.run(["bash", str(CIRCUIT_SYN_DIR / "generate.sh")])

    meta_data = spatial_index.io.read_json(NO_SONATA_SYN_MEM)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_SYN_MEM, meta_data)

    meta_data = spatial_index.io.read_json(NO_SONATA_SYN_MULTI)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_SYN_MULTI, meta_data)


def generate_morph_index():
    subprocess.run(["bash", str(CIRCUIT_MORPH_DIR / "generate.sh")])

    meta_data = spatial_index.io.read_json(NO_SONATA_MORPH_MEM)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_MORPH_MEM, meta_data)

    meta_data = spatial_index.io.read_json(NO_SONATA_MORPH_MULTI)
    del meta_data["extended"]
    spatial_index.io.write_json(NO_SONATA_MORPH_MULTI, meta_data)


def pytest_configure(config):
    config.addinivalue_line("markers", "long: mark tests that take a while to run")

    # Skip synapse index generation if it already exists
    # All test indexes will be regenerated and overwritten
    if not (INDEXES_SYN_DIR / "synapse/in_memory").exists():
        generate_syn_index()
        print("Synapse test indexes generated!")

    # Skip morphology index generation if it already exists
    # All test indexes will be regenerated and overwritten
    if not (INDEXES_MORPH_DIR / "morphology/in_memory").exists():
        generate_morph_index()
        print("Morph test indexes generated!")
