import os
import json

import spatial_index
from .node_indexer import MorphIndex, MorphMultiIndex
from .synapse_indexer import SynapseIndex, SynapseMultiIndex


def load_json(filename):
    with open(filename, "r") as f:
        return json.load(f)


class MetaData:
    _Constants = spatial_index.core._MetaDataConstants

    class _SubConfig:
        def __init__(self, meta_data, sub_config_name):
            self._raw_sub_config = meta_data._raw_meta_data[sub_config_name]
            self._meta_data = meta_data

        @property
        def index_path(self):
            return self._meta_data._meta_data_filename

        def path(self, name):
            # expand path somehow
            return self._meta_data.resolve_path(self._raw_sub_config[name])

    def __init__(self, path):
        self._meta_data_filename = self._deduce_meta_data_filename(path)
        self._raw_meta_data = load_json(self._meta_data_filename)
        self._dirname = os.path.dirname(self._meta_data_filename)

    @property
    def element_type(self):
        return self._raw_meta_data["element_type"]

    @property
    def index_variant(self):
        known_index_variants = [
            MetaData._Constants.in_memory_key,
            MetaData._Constants.memory_mapped_key,
            MetaData._Constants.multi_index_key
        ]

        variants = list(
            filter(lambda k: k in self._raw_meta_data, known_index_variants)
        )

        assert len(variants) == 1, "A meta data file can't have multiple index variants."
        return variants[0]

    @property
    def extended(self):
        return self._sub_config("extended")

    @property
    def in_memory(self):
        return self._sub_config(MetaData._Constants.in_memory_key)

    @property
    def memory_mapped(self):
        return self._sub_config(MetaData._Constants.memory_mapped_key)

    @property
    def multi_index(self):
        return self._sub_config(MetaData._Constants.multi_index_key)

    def resolve_path(self, path):
        return os.path.join(self._dirname, path)

    def _sub_config(self, sub_config_name):
        if sub_config_name in self._raw_meta_data:
            return MetaData._SubConfig(self, sub_config_name)

    def _deduce_meta_data_filename(self, path):
        return spatial_index.core.deduce_meta_data_path(path)


class IndexResolver:
    @staticmethod
    def from_meta_data(meta_data):
        return IndexResolver.get(
            element_type=meta_data.element_type,
            index_variant=meta_data.index_variant
        )

    @staticmethod
    def get(element_type, index_variant):
        if element_type == "morpho_entry":
            if IndexResolver._is_regular_index(index_variant):
                return MorphIndex
            elif IndexResolver._is_multi_index(index_variant):
                return MorphMultiIndex
            else:
                raise ValueError(f"Invalid index_variant: {index_variant}")

        elif element_type == "synapse":
            if IndexResolver._is_regular_index(index_variant):
                return SynapseIndex
            elif IndexResolver._is_multi_index(index_variant):
                return SynapseMultiIndex
            else:
                raise ValueError(f"Invalid index_variant: {index_variant}")

        else:
            raise ValueError(f"Invalid element_type: {element_type}")

    @staticmethod
    def _is_regular_index(index_variant):
        regular_index_keys = [
            MetaData._Constants.in_memory_key,
            MetaData._Constants.memory_mapped_key
        ]
        return index_variant in regular_index_keys

    @staticmethod
    def _is_multi_index(index_variant):
        return index_variant == MetaData._Constants.multi_index_key


def _open_single_population_index(meta_data, **kwargs):
    Index = IndexResolver.from_meta_data(meta_data)
    return Index.from_meta_data(meta_data, **kwargs)


def open_index(path, **kwargs):
    meta_data = MetaData(path)
    return _open_single_population_index(meta_data, **kwargs)
