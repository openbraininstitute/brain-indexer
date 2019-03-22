
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>

#include "bind_point_rtree.cpp"
#include "bind_sphere_rtree.cpp"

namespace py = pybind11;
using namespace py::literals;

PYBIND11_MODULE(spatial_index, m) {

    bind_point_rtree(m);
    bind_sphere_rtree(m);
}
