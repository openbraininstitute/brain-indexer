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


try:
    from .morphology_builder import MorphMultiIndexBuilder  # noqa
    from .synapse_builder import SynapseMultiIndexBuilder  # noqa
except ImportError:
    logging.warning("MPI MultiIndex builders have been disabled")

from .morphology_builder import MorphIndexBuilder  # noqa
from .synapse_builder import SynapseIndexBuilder  # noqa
from .builder import SphereIndexBuilder  # noqa

from .index import SynapseIndex, SynapseMultiIndex  # noqa
from .index import MorphIndex, MorphMultiIndex  # noqa
from .index import SphereIndex  # noqa

from .resolver import IndexResolver, SynapseIndexResolver, MorphIndexResolver  # noqa
from .resolver import SphereIndexResolver  # noqa
from .resolver import open_index  # noqa
