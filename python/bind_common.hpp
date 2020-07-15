#pragma once

#include <spatial_index/index.hpp>
#include <spatial_index/index_grid.hpp>
#include <spatial_index/util.hpp>

#include "bind11_utils.hpp"

namespace bgi = boost::geometry::index;
namespace si = spatial_index;
namespace py = pybind11;
namespace pyutil = pybind_utils;


namespace spatial_index {
namespace py_bindings {

using namespace py::literals;

using point_t = si::Point3D;
using coord_t = si::CoordType;
using id_t = si::identifier_t;
using array_t = py::array_t<coord_t, py::array::c_style | py::array::forcecast>;
using array_ids = py::array_t<id_t, py::array::c_style | py::array::forcecast>;
using array_offsets = py::array_t<unsigned, py::array::c_style | py::array::forcecast>;


inline auto convert_input(array_t const& centroids, array_t const& radii) {
    static_assert(sizeof(point_t) == 3 * sizeof(coord_t),
                  "numpy array not convertible to point3d");
    // Return value pair. only contains a pointer and reference
    return std::make_pair(reinterpret_cast<const point_t*>(centroids.data(0, 0)),
                          radii.template unchecked<1>());
}

inline auto convert_input(array_t const& centroids) {
    static_assert(sizeof(point_t) == 3 * sizeof(coord_t),
                  "numpy array not convertible to point3d");
    return reinterpret_cast<const point_t*>(centroids.data(0, 0));
}

inline const auto& mk_point(array_t const& point) {
    if (point.ndim() != 1 || point.size() != 3) {
        throw std::invalid_argument("Numpy array not convertible to point3d");
    }
    return *reinterpret_cast<const point_t*>(point.data(0));
}



}  // namespace py_bindings
}  // namespace spatial_index
