#pragma once

#include "spatial_index.hpp"

namespace spatial_index {
namespace py_bindings {


template <typename T, typename SomaT>
py::class_<si::IndexTree<T>> create_IndexTree_bindings(py::module& m,
                                                       const char* class_name) {
    using Class = si::IndexTree<T>;

    return py::class_<Class>(m, class_name)
    .def(py::init<>(), "Constructor of an empty SpatialIndex.")

    /// Load tree dump
    .def(py::init<const std::string&>(),
        R"(
        Loads a Spatial Index from a dump()'ed file on disk.

        Args:
            filename(str): The file path to read the spatial index from.
        )"
    )

    .def(py::init([](array_t centroids, array_t radii) {
            if (!radii.ndim()) {
                si::util::constant<coord_t> zero_radius(0);
                auto points = convert_input(centroids);
                auto enum_ = si::util::identity<>{size_t(centroids.shape(0))};
                auto soa = si::util::make_soa_reader<SomaT>(enum_, points, zero_radius);
                return std::make_unique<Class>(soa.begin(), soa.end());
            } else {
                auto point_radius = convert_input(centroids, radii);
                auto enum_ = si::util::identity<>{size_t(radii.shape(0))};
                auto soa = si::util::make_soa_reader<SomaT>(enum_,
                                                         point_radius.first,
                                                         point_radius.second);
                return std::make_unique<Class>(soa.begin(), soa.end());
            }
        }),
        py::arg("centroids"),
        py::arg("radii").none(true),
        R"(
        Creates a SpatialIndex prefilled with Spheres given their centroids and radii
        or Points (radii = None) automatically numbered.

        Args:
             centroids(np.array): A Nx3 array[float32] of the segments' end points
             radii(np.array): An array[float32] with the segments' radii, or None
        )"
    )

    .def(py::init([](array_t centroids, array_t radii, array_ids py_ids) {
            if (!radii.ndim()) {
                si::util::constant<coord_t> zero_radius(0);
                auto points = convert_input(centroids);
                auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<SomaT>(ids, points, zero_radius);
                return std::make_unique<Class>(soa.begin(), soa.end());
            } else {
                auto point_radius = convert_input(centroids, radii);
                auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<SomaT>(ids,
                                                     point_radius.first,
                                                     point_radius.second);
                return std::make_unique<Class>(soa.begin(), soa.end());
            }
        }),
        py::arg("centroids"),
        py::arg("radii").none(true),
        py::arg("py_ids"),
        R"(
        Creates a SpatialIndex prefilled with spheres with explicit ids
        or points with explicit ids and radii = None.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            radii(np.array): An array[float32] with the segments' radii, or None
            py_ids(np.array): An array[int64] with the ids of the spheres
        )"
    )

    .def("dump", &Class::dump,
        R"(
        Save the spatial index tree to a file on disk.

        Args:
            filename(str): The file path to write the spatial index to.
        )"
    )

    .def("insert",
        [](Class& obj, id_t gid, array_t point, coord_t radius) {
            obj.insert(SomaT{gid, mk_point(point), radius});
        },
        R"(
        Inserts a new sphere object in the tree.

        Args:
            gid(int): The id of the sphere
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("place",
        [](Class& obj, array_t region_corners, id_t gid, array_t center, coord_t rad) {
            if (region_corners.ndim() != 2 || region_corners.size() != 6) {
                throw std::invalid_argument("Please provide a 2x3[float32] array");
            }
            const coord_t* c0 = region_corners.data(0, 0);
            const coord_t* c1 = region_corners.data(1, 0);
            return obj.place(si::Box3D{point_t(c0[0], c0[1], c0[2]),
                                       point_t(c1[0], c1[1], c1[2])},
                             SomaT{gid, mk_point(center), rad});
        },
        R"(
        Attempts to insert a sphere without overlapping any existing shape.

        place() will search the given volume region for a free spot for the
        given sphere. Whenever possible will insert it and return True,
        otherwise returns False.

        Args:
            region_corners(array): A 2x3 list/np.array of the region corners
                E.g. region_corners[0] is the 3D min_corner point.
            gid(int): The id of the sphere
            center(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("add_spheres",
        [](Class& obj, array_t centroids, array_t radii, array_ids py_ids) {
            auto point_radius = convert_input(centroids, radii);
            auto ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<SomaT>(
                        ids, point_radius.first, point_radius.second);
            for (auto soma: soa) {
                obj.insert(std::move(soma));
            }
        },
        R"(
        Bulk add more spheres to the spatial index.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            radii(np.array): An array[float32] with the segments' radii
            py_ids(np.array): An array[int64] with the ids of the spheres
        )"
    )

    .def("add_points",
        [](Class& obj, array_t centroids, array_ids py_ids) {
            si::util::constant<coord_t> zero_radius(0);
            auto points = convert_input(centroids);
            auto ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<SomaT>(ids, points, zero_radius);
            for (auto soma: soa) {
                obj.insert(std::move(soma));
            }
        },
        R"(
        Bulk add more points to the spatial index.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            py_ids(np.array): An array[int64] with the ids of the points
        )"
    )

    .def("is_intersecting",
        [](Class& obj, array_t point, coord_t radius) {
            return obj.is_intersecting(si::Sphere{mk_point(point), radius});
        },
        R"(
        Checks whether the given sphere intersects any object in the tree.

        Args:
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("find_intersecting",
        [](Class& obj, array_t point, coord_t r) {
            auto vec = obj.find_intersecting(si::Sphere{mk_point(point), r});
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches objects intersecting the given sphere, and returns their ids.

        Args:
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("find_intersecting_window",
        [](Class& obj, array_t min_corner, array_t max_corner) {
            si::Box3D bounding_box = si::Box3D{mk_point(min_corner), mk_point(max_corner)};
            auto vec = obj.find_intersecting(bounding_box);
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches objects intersecting the given window, and returns their ids.

        Args:
            min_corner, max_corner(float32) : min/max corners of the query window
        )"
    )

    .def("find_nearest",
        [](Class& obj, array_t point, int k_neighbors) {
            auto vec = obj.find_nearest(mk_point(point), k_neighbors);
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches and returns the ids of the nearest K objects to the given point.

        Args:
            point(array): A len-3 list or np.array[float32] with the point
                to search around
            k_neighbors(int): The number of neighbour shapes to return
        )"
    )

    .def("__str__", [](Class& obj) {
        std::stringstream strs;
        strs << obj;
        return strs.str();
    })

    .def("__len__", &Class::size);
}

}  // namespace py_bindings
}  // namespace spatial_index
