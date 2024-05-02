""" spatial_index classes """
import logging
from importlib.metadata import version

from . import _spatial_index as core  # noqa

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = version("spatial-index")
except Exception:
    __version__ = 'devel'


# Set up logging
def register_logger(new_logger):
    """Register `new_logger` as the logger used by SI."""
    global logger
    logger = new_logger

    core._register_python_logger(logger)


register_logger(logging.getLogger(__name__))
# --------------

from ._spatial_index import SectionType # noqa

try:
    from .morphology_builder import MorphMultiIndexBuilder  # noqa
    from .synapse_builder import SynapseMultiIndexBuilder  # noqa
except ImportError:
    import textwrap
    logger.warning(
        "\n".join(textwrap.wrap(
            "The C++ backend of SpatialIndex was compiled without MPI support."
            " Therefore multi-index builders have been disabled. This could be"
            " because you're using a wheel, which (currently) don't support MPI."
            " If you need to create a big index you'll need to use multi-indexes"
            " and therefore a version built with MPI. Please install using Spack"
            " or directly from source."
        ))
    )

from .morphology_builder import MorphIndexBuilder  # noqa
from .synapse_builder import SynapseIndexBuilder  # noqa
from .builder import SphereIndexBuilder, PointIndexBuilder  # noqa

from .index import SynapseIndex, SynapseMultiIndex  # noqa
from .index import MorphIndex, MorphMultiIndex  # noqa
from .index import SphereIndex, PointIndex  # noqa
from .index import MultiPopulationIndex  # noqa

from .resolver import IndexResolver, SynapseIndexResolver, MorphIndexResolver  # noqa
from .resolver import SphereIndexResolver, PointIndexResolver  # noqa
from .resolver import open_index  # noqa
