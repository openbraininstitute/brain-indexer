#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>

#include <memory>

#include <boost/geometry/geometries/point.hpp>

#include <spatial_index/bounding_box.hpp>
#include <spatial_index/rtree_spheres.hpp>

#include "bind_utils.cpp"

namespace py = pybind11;
using namespace py::literals;

namespace bg = boost::geometry;

namespace si = spatial_index;

template<typename CoordinateType, typename SizeType>
void declare_sphere_class(py::module &m)
{
    using Class =
        si::SphereRTree<CoordinateType, SizeType>;

    using point_t =
        typename Class::point_type;

    // sphere entry
    using entry_t =
        typename Class::entry_type;

    // bounding box
    using box_t =
        typename Class::box_type;

    // indexed sphere entry
    using value_t =
        typename Class::value_type;

    using array_t = 
        py::array_t<CoordinateType, py::array::c_style | py::array::forcecast>;

    using wrapper_t =
        ArrayWrapper<SizeType>;

    std::string class_name("sphere_rtree");

    py::class_<Class>(m, class_name.c_str())
        .def(py::init<>())
        .def(py::init( []( array_t centroids, array_t radii )
        {
            // proxy to iterate over the array without checks
            // https://github.com/pybind/pybind11/issues/1400#issue-324406655
            auto c = centroids.template unchecked<2>();
            auto r = radii.template unchecked<1>();

            std::vector<value_t> indexed_entries; indexed_entries.reserve(c.shape(0));

            for(size_t i = 0; i < r.shape(0); ++i)
            {
                const auto& sphere = entry_t::from_raw_data( c(i, 0), c(i, 1), c(i, 2), r(i) );
                indexed_entries.emplace_back( std::move(sphere), std::move(i) );
            }

            return std::unique_ptr<Class>{new Class(indexed_entries)};
        }))

        .def("intersection", [](Class& obj, CoordinateType cx, CoordinateType cy, CoordinateType cz, CoordinateType r){

            wrapper_t wrapper{};

            const auto& q_sphere = entry_t::from_raw_data( cx, cy, cz, r );

            obj.intersection(q_sphere, wrapper.as_vector());

            return wrapper.as_array();
        })

        .def("insert", [](Class& obj, CoordinateType cx, CoordinateType cy, CoordinateType cz, CoordinateType r)
            {
                auto&& i_sphere = entry_t::from_raw_data( cx, cy, cz, r );
                obj.insert(std::move(i_sphere));
            })


        .def("is_intersecting", [](Class& obj, CoordinateType cx, CoordinateType cy, CoordinateType cz, CoordinateType r)
        {
            const auto& q_sphere = entry_t::from_raw_data( cx, cy, cz, r );
            return obj.is_intersecting(q_sphere);
        })

        .def("nearest", [](Class& obj, CoordinateType cx, CoordinateType cy, CoordinateType cz, int k_neighbors)
        {
            wrapper_t wrapper{};
            const point_t centroid(cx, cy, cz);
            obj.nearest(centroid, k_neighbors, wrapper.as_vector());

            return wrapper.as_array();
        })

        .def("data", [](Class& obj)
        {
            ArrayWrapper<CoordinateType> wrapper{};

            obj.data(wrapper.as_vector());

            size_t size = wrapper.size();

            auto array = wrapper.as_array();

            ssize_t n_rows(size / 4);
            ssize_t n_cols = 4;

            array.resize({n_rows, n_cols});

            return array;

        })

        .def("__len__", &Class::size);

}


void bind_sphere_rtree(py::module &m)
{
    declare_sphere_class<float, size_t>(m);
}
