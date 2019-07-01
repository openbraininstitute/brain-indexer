""" spatial_index classes """
# pylint: disable = no-name-in-module

import pkg_resources
from _spatial_index import *

__copyright__ = "2019 Blue Brain Project, EPFL"

try:
    __version__ = pkg_resources.get_distribution(__name__).version
except Exception:
    __version__ = 'devel'
