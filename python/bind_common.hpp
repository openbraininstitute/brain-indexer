#pragma once

#include <spatial_index/index.hpp>
#include "spatial_index/multi_index.hpp"
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
using array_types = py::array_t<unsigned int, py::array::c_style | py::array::forcecast>;

inline coord_t const* extract_radii_ptr(array_t const& radii) {
    return static_cast<coord_t const*>(radii.data());
}

inline point_t const* extract_points_ptr(array_t const& points) {
    static_assert(sizeof(point_t) == 3 * sizeof(coord_t),
                  "numpy array not convertible to point3d");

    if (points.ndim() != 2 || points.shape(1) != 3) {
        auto message = boost::str(
            boost::format(
                "Invalid numpy array shape for 'points': n_dims = %d, shape[0] = %d"
            ) % points.ndim() % points.shape(0)
        );
        throw std::invalid_argument(message);
    }

    return reinterpret_cast<point_t const*>(points.data());
}

inline std::pair<point_t const*, coord_t const*>
extract_points_radii_ptrs(array_t const& points, array_t const& radii) {
    return std::make_pair(extract_points_ptr(points), extract_radii_ptr(radii));

}

inline unsigned const*
extract_offsets_ptr(array_offsets const& offsets) {
    if (offsets.ndim() != 1) {
        auto message = boost::str(
            boost::format(
                "Invalid numpy array shape for 'offsets': n_dims = %d"
            ) % offsets.ndim()
        );

        throw std::invalid_argument(message);
    }
    return static_cast<unsigned const*>(offsets.data());
}


inline const auto& mk_point(array_t const& point) {
    if (point.ndim() != 1 || point.size() != 3) {
        auto message = boost::str(
            boost::format(
                "Invalid numpy array shape for 'point': n_dims = %d, shape[0] = %d"
            ) % point.ndim() % point.shape(0)
        );
        throw std::invalid_argument(message);
    }
    return *reinterpret_cast<const point_t*>(point.data());
}



}  // namespace py_bindings
}  // namespace spatial_index
