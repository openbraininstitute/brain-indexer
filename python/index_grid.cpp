
#include <spatial_index/index_grid.hpp>

#include "bind11_utils.hpp"
#include "spatial_index.hpp"

namespace spatial_index {
namespace py_bindings {


/// Bindings for generic SpatialGrid
template <typename GridT>
py::class_<GridT> create_SpatialGrid_bindings(py::module& m,
                                              const char* class_name) {
    using Class = GridT;
    using value_type = typename GridT::value_type;

    return py::class_<Class>(m, class_name)
        .def(py::init<>(), "Constructor of an empty SpatialGrid.")

        .def(
            "insert",
            [](Class& obj, const array_t& items) {
                // With generic types the best we can do is to reinterpret memory.
                //  Up to the user to create numpy arrays that match
                // Input is probably either 1D array of coords (for a single point in space)
                // or 2D for an array of points.
                if (items.ndim() == 1) {
                    const auto &data = *reinterpret_cast<const value_type*>(items.data(0));
                    obj.insert(data);
                } else if (items.ndim() == 2) {
                    const auto* first = reinterpret_cast<const value_type*>(items.data(0, 0));
                    const auto* last = first + items.shape(0);
                    obj.insert(first, last);
                }

            },
            R"(
                Inserts elements in the tree.
                Please note that no data conversions are performed.
            )")

        .def("__len__", &Class::size, "The total number of elements")

        .def("__str__", [](Class& obj) {
            std::stringstream ss;
            si::operator<<(ss, obj);
            return ss.str();
        })

        .def("__iadd__", &Class::operator+=)

        .def(py::pickle(
             [](const Class& obj) {  // __getstate__ / serialize
                pyutil::StringBuffer buf;
                boost::archive::binary_oarchive oa(buf);
                oa << obj;
                return pyutil::as_pyarray(std::move(buf));
             },
             [](const py::array& arr) {  // unserialize
                pyutil::StringBuffer buf;
                buf.str(std::string(static_cast<const char*>(arr.data()),
                                    static_cast<std::size_t>(arr.size())
                        ));
                boost::archive::binary_iarchive ia(buf);
                auto obj = std::make_unique<Class>();
                ia >> (*obj);
                return obj;
             }
        ));
}


void create_SphereGrid_bindings(py::module& m) {
    using Grid = si::SpatialGrid<si::Point3D, 5>;
    create_SpatialGrid_bindings<Grid>(m, "SphereGrid");
    // ... TODO: Not point, spheres!
}


void create_MorphGrid_bindings(py::module& m) {
    using Grid = si::MorphSpatialGrid<5>;
    create_SpatialGrid_bindings<Grid>(m, "MorphGrid");
}


}  // namespace py_bindings
}  // namespace spatial_index
