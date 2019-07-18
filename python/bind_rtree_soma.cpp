#include <spatial_index/index.hpp>

#include "bind11_utils.hpp"


namespace bgi = boost::geometry::index;
namespace si = spatial_index;
namespace py = pybind11;
namespace pyutil = pybind_utils;

using namespace py::literals;



void bind_rtree_soma(py::module& m) {
    using Entry = si::ISoma;
    using Class = si::IndexTree<si::ISoma>;
    using point_t = si::Point3D;
    using coord_t = si::CoordType;
    using array_t = py::array_t<coord_t, py::array::c_style | py::array::forcecast>;

    std::string class_name("SomaTree");

    py::class_<Class>(m, class_name.c_str())

        .def(py::init<>())

        .def(py::init([](array_t centroids, array_t radii) {
            // proxy to iterate over the array without checks
            // https://github.com/pybind/pybind11/issues/1400#issue-324406655
            auto c = centroids.template unchecked<2>();
            auto r = radii.template unchecked<1>();
            auto points = reinterpret_cast<const point_t*>(c.data(0, 0));

            std::vector<si::ISoma> indexed_entries;
            indexed_entries.reserve(c.shape(0));

            for (size_t i = 0; i < r.shape(0); ++i) {
                indexed_entries.emplace_back(i, points[i], r(i));
            }

            return std::unique_ptr<Class>{new Class(indexed_entries)};
        }))

        .def("insert",
             [](Class& obj, unsigned long i, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                 obj.insert(Entry{i, si::Point3D{cx, cy, cz}, r});
             },
             "Inserts a new object in the tree.")

        .def("find_intersecting",
             [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                 std::vector<si::identifier_t> vec;
                 /** This commented code block is an alternative for finer control */
                 // auto entries = obj.find_intersecting(si::Sphere{{cx, cy, cz}, r});
                 // vec.reserve(entries.size());
                 // for (const Entry& soma: entries) {
                 //     vec.push_back(soma.gid());
                 // }
                 obj.find_intersecting(si::Sphere{{cx, cy, cz}, r}, si::iter_ids_getter(vec));
                 return pyutil::as_pyarray(std::move(vec));
             },
             "Searches objects intersecting the given sphere, and returns their ids.")

        .def("is_intersecting",
             [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                 return obj.is_intersecting(si::Sphere{{cx, cy, cz}, r});
             },
            "Checks whether the given sphere intersects any object in the tree.")

        .def("find_nearest",
             [](Class& obj, coord_t cx, coord_t cy, coord_t cz, int k_neighbors) {
                 std::vector<si::identifier_t> vec;
                 obj.query(bgi::nearest(point_t{cx, cy, cz}, k_neighbors),
                           si::iter_ids_getter(vec));
                return pyutil::as_pyarray(std::move(vec));
             },
             "Searches and returns the ids of the nearest K objects to the given point.")

        .def("__len__", &Class::size);
}
