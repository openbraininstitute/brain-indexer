#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

#include "bind11_utils.hpp"


namespace bgi = boost::geometry::index;
namespace si = spatial_index;
namespace py = pybind11;
namespace pyutil = pybind_utils;

using namespace py::literals;



void bind_rtree(py::module& m) {
    using Entry = si::MorphoEntry;
    using Class = si::IndexTree<Entry>;
    using point_t = si::Point3D;
    using coord_t = si::CoordType;
    using id_t = si::identifier_t;
    using array_t = py::array_t<coord_t, py::array::c_style | py::array::forcecast>;
    using array_ids = py::array_t<id_t, py::array::c_style | py::array::forcecast>;

    std::string class_name("MorphIndex");

    py::class_<Class>(m, class_name.c_str())

        .def(py::init<>())

        .def(py::init([](array_t centroids, array_t radii) {
            auto c = centroids.template unchecked<2>();
            auto r = radii.template unchecked<1>();

            static_assert(sizeof(point_t) == 3*sizeof(coord_t), "numpy array not convertible to point3d");
            auto points = reinterpret_cast<const point_t*>(c.data(0, 0));

            auto enum_ = si::util::identity{size_t(c.shape(0))};
            auto soa = si::util::make_soa_reader<si::ISoma>(enum_, points, r);

            return std::unique_ptr<Class>{new Class(soa.begin(), soa.end())};
        }))

        .def("insert",
            [](Class& obj, id_t i, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                obj.insert(si::ISoma{i,point_t{cx, cy, cz}, r});
            },
            "Inserts a new soma object in the tree.")

        .def("insert",
            [](Class& obj, id_t i, unsigned part_id,
                    coord_t p1_cx, coord_t p1_cy, coord_t p1_cz,
                    coord_t p2_cx, coord_t p2_cy, coord_t p2_cz,
                    coord_t r) {
                obj.insert(si::ISegment{i, part_id, point_t{p1_cx, p1_cy, p1_cz}, si::Point3D{p2_cx, p2_cy, p2_cz}, r});
            },
            "Inserts a new segment object in the tree.")

        .def("find_intersecting",
            [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                std::vector<si::identifier_t> vec;
                obj.find_intersecting(si::Sphere{{cx, cy, cz}, r}, si::iter_ids_getter(vec));
                return pyutil::to_pyarray(vec);
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
