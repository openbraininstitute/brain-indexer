""" spatial_index classes """
import pkg_resources

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = pkg_resources.get_distribution(__name__).version
except Exception:
    __version__ = 'devel'

from . import _spatial_index as core  # noqa
from ._spatial_index import *  # noqa
from .node_indexer import MorphIndexBuilder  # noqa

if hasattr(core, "MorphMultiIndexBulkBuilder"):
    from .node_indexer import MorphMultiIndexBuilder  # noqa

from .synapse_indexer import PointIndex, SynapseIndexBuilder  # noqa
