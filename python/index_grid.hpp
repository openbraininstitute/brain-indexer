
#include <spatial_index/index_grid.hpp>

#include "bind11_utils.hpp"
#include "bind_common.hpp"

namespace spatial_index {
namespace py_bindings {


/// Bindings for generic SpatialGrid
template <typename GridT>
inline py::class_<GridT> create_SpatialGrid_bindings(py::module& m,
                                                     const std::string& class_name) {
    using Class = GridT;

    return py::class_<Class>(m, class_name.c_str())

    .def(py::init<>(), "Constructor of an empty SpatialGrid.")

    .def("__len__", &Class::size, "The total number of elements")

    .def("__str__", [](Class& obj) {
        std::stringstream ss;
        si::operator<<(ss, obj);
        return ss.str();
    })

    .def("__iadd__", &Class::operator+=)

    .def("__eq__", &Class::operator==)

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
                                static_cast<std::size_t>(arr.size())));
            boost::archive::binary_iarchive ia(buf);
            auto obj = std::make_unique<Class>();
            ia >> (*obj);
            return obj;
         }
    ));
}


///
/// Index of Bare Geometries (without ids) implement `insert(data)`
///
template <typename GridT>
inline void create_GeometryGrid_bindings(py::module& m, const std::string& cls_name) {
    using value_type = typename GridT::value_type;

    create_SpatialGrid_bindings<GridT>(m, cls_name)
    .def("insert",
        [](GridT& obj, const array_t& items) {
            // With generic types the best we can do is to reinterpret memory.
            //  Up to the user to create numpy arrays that match
            // Input is probably either 1D array of coords (for a single point in space)
            // or 2D for an array of points.
            if (items.ndim() == 1) {
                const auto &data = *reinterpret_cast<const value_type*>(items.data(0));
                obj.insert(data);
            } else if (items.ndim() == 2) {
                const auto* first = reinterpret_cast<const value_type*>(items.data(0, 0));
                const auto* end = first + items.shape(0);
                obj.insert(first, end);
            } else {
                throw std::invalid_argument("Unknown arg format. Generic insert() "
                                            "only supports vector or matrix of points.");
            }
        },
        R"(
        Inserts elements in the tree.
        Please note that no data conversions are performed.
        )"
    );

}

///
/// IndexedShape implementing generic `insert(data, ids)`
///
template <typename GridT>
inline void create_IndexedShapeGrid_bindings(py::module& m, const std::string& cls_name) {
    using value_type = typename GridT::value_type;

    create_SpatialGrid_bindings<GridT>(m, cls_name)
    .def("insert",
        [](GridT& obj, const array_t& items, const array_ids& py_ids) {
            // Quick check dimensions match
            if (items.ndim() == 2 && items.shape(0) != py_ids.shape(0)) {
                throw std::invalid_argument(
                    "Mismatch among the number of elements (rows) and ids");
            }
            const auto* g_data = reinterpret_cast<
                const typename value_type::geometry_type*>(items.data(0));
            const auto ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<value_type>(ids, g_data);
            for (auto&& o : soa) {
                obj.insert(o);
            }
        },
        R"(
        Inserts elements in the tree with given ids.
        NOTE: no conversions are performed. Ensure matrix width contains enough data
        for each element.
        )"
    );

}



}  // namespace py_bindings
}  // namespace spatial_index
