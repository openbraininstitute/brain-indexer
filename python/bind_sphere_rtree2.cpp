#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>
#include <memory>

#include <spatial_index/index.hpp>

#include "bind_utils.cpp"


namespace py = pybind11;
namespace bg = boost::geometry;
namespace si = spatial_index;

using namespace py::literals;


void bind_rtree_sphere(py::module &m)
{
    using Entry = si::ISoma;

    using Class = si::IndexTree<si::ISoma>;

    using point_t = si::Point3D;

    using coord_t = si::CoordType;

    using array_t = py::array_t<si::CoordType, py::array::c_style | py::array::forcecast>;

    using wrapper_t = ArrayWrapper<si::identifier_t>;

    std::string class_name("SomaTree");

    py::class_<Class>(m, class_name.c_str())

        .def(py::init<>())

        .def(py::init( []( array_t centroids, array_t radii )
        {
            // proxy to iterate over the array without checks
            // https://github.com/pybind/pybind11/issues/1400#issue-324406655
            auto c = centroids.template unchecked<2>();
            auto r = radii.template unchecked<1>();

            std::vector<si::ISoma> indexed_entries;
            indexed_entries.reserve(c.shape(0));

            for(size_t i = 0; i < r.shape(0); ++i) {
                indexed_entries.emplace_back(i, point_t{c(i, 0), c(i, 1), c(i, 2)}, r(i));
            }

            return std::unique_ptr<Class>{new Class(indexed_entries)};
        }))

        .def("insert", [](Class& obj, unsigned long i, coord_t cx, coord_t cy, coord_t cz, coord_t r)
        {
            obj.insert(Entry{i, si::Point3D{cx, cy, cz}, r});
        })

        .def("find_intersecting", [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r){

            wrapper_t wrapper;
            auto& vec = wrapper.as_vector();
            for (const Entry* soma : obj.find_intersecting(si::Sphere{{cx, cy, cz}, r})) {
                vec.push_back(soma->gid());
            }
            return wrapper.as_array();
        })



        // .def("is_intersecting", [](Class& obj, CoordinateType cx, CoordinateType cy, CoordinateType cz, CoordinateType r)
        // {
        //     const auto& q_sphere = entry_t::from_raw_data( cx, cy, cz, r );
        //     return obj.is_intersecting(q_sphere);
        // })

        .def("find_nearest", [](Class& obj, coord_t cx, coord_t cy, coord_t cz, int k_neighbors)
        {
            wrapper_t wrapper;
            obj.query(bgi::nearest(point_t{cx, cy, cz}, k_neighbors), si::iter_ids_getter(wrapper.as_vector()));

            return wrapper.as_array();
        })

        // .def("data", [](Class& obj)
        // {
        //     ArrayWrapper<coord_t> wrapper{};

        //     obj.data(wrapper.as_vector());

        //     size_t size = wrapper.size();

        //     auto array = wrapper.as_array();

        //     ssize_t n_rows(size / 4);
        //     ssize_t n_cols = 4;

        //     array.resize({n_rows, n_cols});

        //     return array;

        // })

        .def("__len__", &Class::size);

}

