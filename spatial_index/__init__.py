""" spatial_index classes """
import pkg_resources

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = pkg_resources.get_distribution(__name__).version
except Exception:
    __version__ = 'devel'

from ._spatial_index import *  # NOQA
from .node_indexer import NodeMorphIndexer  # noqa
from .synapse_indexer import PointIndexer, SynapseIndexer  # noqa
