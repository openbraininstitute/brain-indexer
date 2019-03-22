#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>

#include <memory>

#include <boost/geometry/geometries/point.hpp>

#include <spatial_index/rtree_points.hpp>

#include "bind_utils.cpp"

namespace py = pybind11;
using namespace py::literals;

namespace bg = boost::geometry;

namespace si = spatial_index;


template<typename CoordinateType, typename SizeType>
void declare_point_class(py::module &m)
{
    using Class = si::PointRTree<CoordinateType, SizeType>;
    using point_type = bg::model::point<CoordinateType, 3, bg::cs::cartesian>;

    using array_t = py::array_t<CoordinateType, py::array::c_style | py::array::forcecast>;
    using wrapper_type = ArrayWrapper<SizeType>;

    std::string class_name("point_rtree");

    py::class_<Class>(m, class_name.c_str())
        .def(py::init<>())

        .def(py::init( []( array_t point_array )
        {
            // proxy to iterate over the array without checks
            // https://github.com/pybind/pybind11/issues/1400#issue-324406655
            auto r = point_array.template unchecked<2>();

            std::vector<point_type> points; points.reserve(r.shape(0));

            for(size_t i = 0; i < r.shape(0); ++i)
                points.emplace_back(r(i, 0), r(i, 1), r(i, 2));

            return std::unique_ptr<Class>{new Class(points)};
        }))

        .def("intersection", [](Class& obj, 
                                CoordinateType xmin, CoordinateType ymin, CoordinateType zmin,
                                CoordinateType xmax, CoordinateType ymax, CoordinateType zmax){

            const auto min_point = point_type(xmin, ymin, zmin);
            const auto max_point = point_type(xmax, ymax, zmax);

            ArrayWrapper<SizeType> wrapper;
            obj.intersection(min_point, max_point, wrapper.as_vector());
            return wrapper.as_array(); // 1D
        })

        .def("intersection_data", [](Class& obj, 
                                CoordinateType xmin, CoordinateType ymin, CoordinateType zmin,
                                CoordinateType xmax, CoordinateType ymax, CoordinateType zmax){

            const auto min_point = point_type(xmin, ymin, zmin);
            const auto max_point = point_type(xmax, ymax, zmax);

            ArrayWrapper<CoordinateType> wrapper;
            obj.intersection(min_point, max_point, wrapper.as_vector());
            return wrapper.as_array(3); // 3 columns in coordinate array
        })

        .def("__len__", &Class::size);

}


void bind_point_rtree(py::module &m)
{
    declare_point_class<float, size_t>(m);
}
