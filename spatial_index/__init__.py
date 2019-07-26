""" spatial_index classes """
import pkg_resources
from ._spatial_index import *  # NOQA

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = pkg_resources.get_distribution(__name__).version
except Exception:
    __version__ = 'devel'
