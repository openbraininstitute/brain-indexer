from . import _spatial_index as core
from .index import SphereIndex


class SphereIndexBuilderBase:
    def __init__(self):
        self._core_builder = core.SphereIndex()

    def add_sphere(self, id, center, radius):
        self._core_builder._insert(id, center, radius)

    @classmethod
    def create(cls, centroids, radii, output_dir=None):
        builder = cls()

        for k, sphere in enumerate(zip(centroids, radii)):
            builder.add_sphere(k, *sphere)

        builder._write_index_if_needed(output_dir)
        return builder._index_if_loaded

    @classmethod
    def from_numpy(cls, centroids, radii, output_dir=None):
        return cls.create(centroids, radii)


class SphereIndexBuilder(SphereIndexBuilderBase):
    def _write_index_if_needed(self, output_dir):
        self.index.write(output_dir)

    @property
    def _index_if_loaded(self):
        return self.index

    @property
    def index(self):
        return SphereIndex(self._core_builder)
