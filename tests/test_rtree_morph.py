import numpy as np
from _spatial_index import MorphIndex
import importlib
import os.path

test_rtree_soma = importlib.import_module('test_rtree_soma',
                                          os.path.dirname(__file__))

test_rtree_soma.IndexClass = MorphIndex
test_rtree_soma.run_tests()
