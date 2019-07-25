#pragma once
#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

#include "bind11_utils.hpp"


namespace bgi = boost::geometry::index;
namespace si = spatial_index;
namespace py = pybind11;
namespace pyutil = pybind_utils;

using namespace py::literals;


// Both indexes share a great deal of API in common
template <typename Entry>
struct py_rtree {
    using value_type = Entry;
    using Class = si::IndexTree<Entry>;
    using point_t = si::Point3D;
    using coord_t = si::CoordType;
    using id_t = si::identifier_t;
    using array_t = py::array_t<coord_t, py::array::c_style | py::array::forcecast>;
    using array_ids = py::array_t<id_t, py::array::c_style | py::array::forcecast>;
    using array_offsets = py::array_t<size_t, py::array::c_style | py::array::forcecast>;

  protected:
    inline py::class_<Class> init_class_bindings(py::module& m, const char* class_name) {
        return py::class_<Class>(m, class_name)
            .def(py::init<>())

            // Initialize a structure with Somas automatically numbered
            .def(py::init([](array_t centroids, array_t radii) {
                auto point_radius = convert_input(centroids, radii);
                auto enum_ = si::util::identity<>{size_t(radii.shape(0))};
                auto soa = si::util::make_soa_reader<si::Soma>(enum_, point_radius.first,
                                                               point_radius.second);
                return std::unique_ptr<Class>{new Class(soa.begin(), soa.end())};
            }))

            // Initialize structure with somas and specific ids
            .def(py::init([](array_t centroids, array_t radii, array_ids py_ids) {
                auto point_radius = convert_input(centroids, radii);
                auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<si::Soma>(ids, point_radius.first,
                                                               point_radius.second);
                return std::unique_ptr<Class>{new Class(soa.begin(), soa.end())};
            }))

            .def(
                "insert",
                [](Class& obj, id_t i, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                    obj.insert(si::Soma{i, point_t{cx, cy, cz}, r});
                },
                "Inserts a new soma object in the tree.")

            .def(
                "add_somas",
                [](Class& obj, array_ids py_ids, array_t centroids, array_t radii) {
                    auto point_radius = convert_input(centroids, radii);
                    auto ids = py_ids.template unchecked<1>();
                    auto soa = si::util::make_soa_reader<si::Soma>(ids, point_radius.first,
                                                                   point_radius.second);
                    for (auto soma: soa) {
                        obj.insert(std::move(soma));
                    }
                },
                "Bulk add more somas to the spatial index")

            .def(
                "is_intersecting",
                [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                    return obj.is_intersecting(si::Sphere{{cx, cy, cz}, r});
                },
                "Checks whether the given sphere intersects any object in the tree.")

            .def("__len__", &Class::size);
    }


    static inline auto convert_input(array_t const& centroids, array_t const& radii) {
        static_assert(sizeof(point_t) == 3 * sizeof(coord_t),
                      "numpy array not convertible to point3d");
        auto points_ptr = reinterpret_cast<const point_t*>(centroids.data(0, 0));
        auto r = radii.template unchecked<1>();
        return std::make_pair(points_ptr, r);
    }
};
