""" spatial_index classes """
import logging
import pkg_resources

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = pkg_resources.get_distribution(__name__).version
except Exception:
    __version__ = 'devel'

from . import _spatial_index as core  # noqa


# Set up logging
def register_logger(new_logger):
    """Register `new_logger` as the logger used by SI."""
    global logger
    logger = new_logger

    core._register_python_logger(logger)


register_logger(logging.getLogger(__name__))
# --------------


from ._spatial_index import *  # noqa

try:
    from .node_indexer import MorphMultiIndexBuilder  # noqa
    from .synapse_indexer import SynapseMultiIndexBuilder  # noqa
except ImportError:
    logging.warning("MPI MultiIndex builders have been disabled")

from .node_indexer import MorphIndexBuilder, MorphMultiIndex  # noqa
from .synapse_indexer import PointIndex, SynapseIndexBuilder, SynapseMultiIndex  # noqa

from .io import open_index # noqa
