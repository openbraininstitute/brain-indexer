from . import _spatial_index as core
from .index import SphereIndex, PointIndex
from .io import write_sonata_meta_data_section

import numpy as np


class _WriteSONATAMetadataMixin:
    def _write_extended_meta_data_section(*a, **kw):
        write_sonata_meta_data_section(*a, **kw)


class _WriteSONATAMetadataMultiMixin:
    def _write_extended_meta_data_section(*a, **kw):
        from mpi4py import MPI

        if MPI.COMM_WORLD.Get_rank() == 0:
            write_sonata_meta_data_section(*a, **kw)

        MPI.COMM_WORLD.Barrier()


class SimpleShapeIndexBuilder:
    @classmethod
    def create(cls, *args, ids=None, output_dir=None):
        assert len(args) > 0

        builder = cls()

        if ids is None:
            ids = np.arange(args[0].shape[0])

        builder._core_index = cls.core_index_type(*args, ids)

        builder._write_index_if_needed(output_dir)
        return builder._index_if_loaded

    @classmethod
    def create_empty(cls):
        core_index = core.SphereIndex()
        return SphereIndex(core_index)

    def _write_index_if_needed(self, output_dir):
        self.index.write(output_dir)

    @property
    def _index_if_loaded(self):
        return self.index

    @property
    def index(self):
        return self.index_type(self._core_index)


class SphereIndexBuilder(SimpleShapeIndexBuilder):
    core_index_type = core.SphereIndex
    index_type = SphereIndex

    @classmethod
    def from_numpy(cls, centroids, radii, ids=None, output_dir=None):
        return cls.create(centroids, radii, ids=ids, output_dir=output_dir)

    def add_sphere(self, *a, **kw):
        import warnings

        warnings.warn("Deprecated in favour of 'from_numpy'.", DeprecationWarning)
        raise RuntimeError("Invalid method, see deprecation warning.")


class PointIndexBuilder(SimpleShapeIndexBuilder):
    core_index_type = core.PointIndex
    index_type = PointIndex

    @classmethod
    def from_numpy(cls, positions, ids=None, output_dir=None):
        return cls.create(positions, ids=ids, output_dir=output_dir)
