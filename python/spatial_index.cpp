
#include "spatial_index.hpp"

namespace si_python = spatial_index::py_bindings;


PYBIND11_MODULE(_spatial_index, m) {
    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, segment_i);  // Pybind11 wow!

    si_python::create_SphereIndex_bindings(m);
    si_python::create_MorphIndex_bindings(m);
}
