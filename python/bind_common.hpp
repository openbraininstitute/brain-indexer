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

template<class T>
using pybind_array_t = py::array_t<T, py::array::c_style | py::array::forcecast>;

using array_t = pybind_array_t<coord_t>;
using array_ids = pybind_array_t<id_t>;
using array_offsets = pybind_array_t<unsigned>;
using array_types = pybind_array_t<unsigned int>;

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

namespace detail {
template <class Int>
inline Int const *
extract_int_ptr(const pybind_array_t<Int>& int_array, const std::string& var_name) {
    if (int_array.ndim() != 1) {
        auto message = boost::str(
            boost::format(
                "Invalid numpy array shape for '%s': n_dims = %d"
            ) % var_name % int_array.ndim()
        );

        throw std::invalid_argument(message);
    }

    return static_cast<Int const*>(int_array.data());
}
}

inline id_t const*
extract_ids_ptr(array_ids const& ids) {
    return detail::extract_int_ptr(ids, "ids");
}

inline unsigned const*
extract_offsets_ptr(array_offsets const& offsets) {
    return detail::extract_int_ptr(offsets, "offsets");
}

inline unsigned int const*
extract_section_types_ptr(array_types const& section_types) {
    return detail::extract_int_ptr(section_types, "section_types");
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

inline void mark_deprecated(const std::string& deprecation_message) {
    // Credit: https://stackoverflow.com/a/62559865
    // See: https://docs.python.org/3/c-api/exceptions.html#c.PyErr_WarnEx
    auto error_code = PyErr_WarnEx(
        PyExc_DeprecationWarning, 
        deprecation_message.c_str(),
        /* stack_level = */ 1
    );

    if(error_code != 0) {
        throw std::runtime_error("Failed to create a decrecation warning.");
    }
}



}  // namespace py_bindings
}  // namespace spatial_index
