# This file is part of SpatialIndex, the new-gen spatial indexer for BBP
# Copyright Blue Brain Project 2020-2021. All rights reserved
import logging
from abc import ABCMeta, abstractmethod
from libsonata import Selection


class DiskMemMapProps:
    """A class to configure memory-mapped files as the backend for spatial indices."""

    def __init__(self, map_file, file_size=1024, close_shrink=False):
        self.memdisk_file = map_file
        self.file_size = file_size
        self.shrink = close_shrink

    @property
    def args(self):
        return self.memdisk_file, self.file_size, self.shrink


class ExtendedIndex(metaclass=ABCMeta):
    """
    A High level Index, able to combine in additional fields from the Sonata dataset.

    This Index provides window and spheric querying functionalities.
    Lower-level changes and queries require access to the core index-tree object (.index)
    """
    __slots__ = ('index', 'dataset')

    # Subclasses override these
    CoreIndexClass = None
    IndexClassMemMap = None
    DefaultExtraFields = ()

    def __init__(self, index, src_data) -> None:
        """An extended index basically only holds the core index and the src dataset
        """
        self.index = index
        self.dataset = src_data

    @property
    def raw_index(self):
        return self.index

    def __len__(self):
        return len(self.index)

    @classmethod
    def open_core_index(cls, input, *args):
        """Opens and returns a core index directly"""
        return cls.CoreIndexClass(input, *args)

    @abstractmethod
    def open_dataset(self, storage_file, population_name: str):
        """Implements the opening of the source dataset, e.g. a Sonata population"""

    def window_q(self, corner_min, corner_max,
                 output="dataset",
                 extra_fields=None,
                 **kw):
        """Performs a window query in the spatial index.

        Results can be extended with selected additional fields.

        Args:
            corner_min: The window min corner point coordinates
            corner_max: The window max corner point coordinates
            output: The kind of output, one of ['ids', 'all', 'objs']
            extra_fields: What additional fields should be fetched from sonata

        Returns:
            dict: a dictionalry of {field_name: numpy_array}
        """

        return_type_to_fn = {
            "ids": self.index.find_intersecting_window,
            "dataset": self.index.find_intersecting_window_np,
            "objects": self.index.find_intersecting_window_objs
        }
        query_fn = return_type_to_fn.get(output)
        if not query_fn:
            raise RuntimeError("Requested return collection not available: " + output)

        result = query_fn(corner_min, corner_max, **kw)

        # Query additonal properties. Allowed only with "full" output
        if extra_fields is None:
            extra_fields = self.DefaultExtraFields
        if output != "dataset" or not extra_fields:
            return result

        extra_fields = set(extra_fields).difference(result.keys())
        selection = Selection(result['id'])

        for extra_f in extra_fields:
            result[extra_f] = self.dataset.get_attribute(extra_f, selection)

        return result

    @classmethod
    def open(cls, index_conf_f: str):
        import toml
        conf = toml.load(index_conf_f)
        index_path = conf["index"]
        index_storage = conf.get("index_storage", "dump")
        sonata_path = conf["sonata_file"]
        sonata_pop = conf.get("sonata_population")

        if index_storage not in ("dump", "memory_map"):
            raise Exception("Unknown index storage: " + index_storage)

        load_fn = cls.from_dump if index_storage == "dump" else cls.from_memory_map
        return load_fn(index_path, sonata_path, sonata_pop)

    @classmethod
    def from_dump(cls, filename, dataset_path, population=None):
        """Opens a Spatial index from a core index and sonata population dataset"""
        core_index = cls.CoreIndexClass(filename)
        return cls._from_core_index(core_index, dataset_path, population)

    @classmethod
    def from_memory_map(cls, filename, dataset_path, population=None):
        """Opens a Spatial index from a core memory map and sonata population dataset"""
        core_index = cls.IndexClassMemMap.open(filename)
        return cls._from_core_index(core_index, dataset_path, population)

    @classmethod
    def _from_core_index(cls, core_index, dataset_path, population):
        if not dataset_path:
            logging.warning("No dataset file provided. Returning a core index")
            return core_index
        return cls(core_index, cls.open_dataset(dataset_path, population))

    # TODO: Delete this and improve API all around
    def __getattr__(self, item):
        return getattr(self.index, item)


class ExtendedMultiIndexMixin:
    _DEFAULT_MEM = 1e9

    @classmethod
    def open_core_index(cls, dir, mem=_DEFAULT_MEM):
        return cls.CoreIndexClass(dir, mem)

    @classmethod
    def from_dir(cls, output_dir, dataset_path=None, population=None, mem=_DEFAULT_MEM):
        print(cls.open_core_index)
        core_index = cls.open_core_index(output_dir, mem)
        return cls._from_core_index(core_index, dataset_path, population)

    @classmethod
    def from_dump(cls, *_args, **_kw):
        raise NotImplementedError("Multi-indices don't support bare dumps.")

    @classmethod
    def from_memory_map(cls, *_args, **_kw):
        raise NotImplementedError("Multi-indices don't support memory maps")


class IndexBuilderBase:
    __slots__ = ["_src_data", "_selection", "_index", "_extended_index"]

    IndexClass = None  # override me
    """The Python specific ExtendedIndex class"""

    CoreIndexBuilder = None  # override if the core builder is not the core index class
    """The Core Index Builder. By default it's IndexClass.CoreIndexClass"""

    def __init__(self, src_data, selection, *,
                 disk_mem_map: DiskMemMapProps = None,
                 index_ctor_args=()):
        self._src_data = src_data
        self._selection = selection
        if disk_mem_map:
            self._index = self.IndexClass.IndexClassMemMap.create(*disk_mem_map.args)
        else:
            IndexBuilder = self.CoreIndexBuilder or self.IndexClass.CoreIndexClass
            self._index = IndexBuilder(*index_ctor_args)

        # Create obj right away so that potential multiple get() requests yield the same
        self._extended_index = self.IndexClass(self.index, src_data)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, new_index):
        self._index = new_index
        self._extended_index.index = new_index

    def get_object(self):
        return self._extended_index

    @classmethod
    def load_dump(cls, filename):
        """Load the index from a dump file"""
        return cls.IndexClass.from_dump(filename, None)

    @classmethod
    def load_disk_mem_map(cls, filename):
        """Load the index from a memory mapped file"""
        return cls.IndexClass.from_memory_map(filename, None)
