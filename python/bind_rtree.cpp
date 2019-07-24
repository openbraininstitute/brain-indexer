#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

#include "bind11_utils.hpp"


namespace bgi = boost::geometry::index;
namespace si = spatial_index;
namespace py = pybind11;
namespace pyutil = pybind_utils;

using namespace py::literals;


#define CONVERT_INPUT(centroids, radii) \
    auto c = centroids.template unchecked<2>(); \
    auto r = radii.template unchecked<1>(); \
    static_assert(sizeof(point_t) == 3*sizeof(coord_t), \
                  "numpy array not convertible to point3d"); \
    auto points = reinterpret_cast<const point_t*>(c.data(0, 0));


void bind_rtree(py::module& m) {
    using Entry = si::MorphoEntry;
    using Class = si::IndexTree<Entry>;
    using point_t = si::Point3D;
    using coord_t = si::CoordType;
    using id_t = si::identifier_t;
    using array_t = py::array_t<coord_t, py::array::c_style | py::array::forcecast>;
    using array_ids = py::array_t<id_t, py::array::c_style | py::array::forcecast>;
    using array_offsets = py::array_t<size_t, py::array::c_style | py::array::forcecast>;

    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, segment_i);  // Pybind11 wow!

    std::string class_name("MorphIndex");

    py::class_<Class>(m, class_name.c_str())

        .def(py::init<>())

        // Initialize a structure with Somas automatically numbered
        .def(py::init([](array_t centroids, array_t radii) {
            CONVERT_INPUT(centroids, radii);
            auto enum_ = si::util::identity<>{size_t(c.shape(0))};
            auto soa = si::util::make_soa_reader<si::Soma>(enum_, points, r);
            return std::unique_ptr<Class>{new Class(soa.begin(), soa.end())};
        }))

        // Initialize structure with somas and specific ids
        .def(py::init([](array_t centroids, array_t radii, array_ids py_ids) {
            CONVERT_INPUT(centroids, radii);
            auto ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<si::Soma>(ids, points, r);
            return std::unique_ptr<Class>{new Class(soa.begin(), soa.end())};
        }))

        .def("insert",
            [](Class& obj, id_t i, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                obj.insert(si::Soma{i,point_t{cx, cy, cz}, r});
            },
            "Inserts a new soma object in the tree.")

        .def("insert",
            [](Class& obj, id_t i, unsigned part_id,
                    coord_t p1_cx, coord_t p1_cy, coord_t p1_cz,
                    coord_t p2_cx, coord_t p2_cy, coord_t p2_cz,
                    coord_t r) {
                obj.insert(si::Segment{i,
                                       part_id,
                                       point_t{p1_cx, p1_cy, p1_cz},
                                       point_t{p2_cx, p2_cy, p2_cz},
                                       r});
            },
            "Inserts a new segment object in the tree.")

        .def("add_somas",
            [](Class& obj, array_ids py_ids, array_t centroids, array_t radii) {
                CONVERT_INPUT(centroids, radii);
                auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<si::Soma>(ids, points, r);
                for (auto soma : soa) {
                    obj.insert(std::move(soma));
                }
            },
            "Bulk add more somas to the spatial index")

        .def("add_neuron",
            [](Class& obj, id_t neuron_id, array_t centroids, array_t radii) {
                CONVERT_INPUT(centroids, radii);
                // Add soma
                obj.insert(si::Soma{neuron_id, points[0], r[0]});

                // Create soa reader for data as segments
                auto nrn_ids = si::util::constant<>{neuron_id, size_t(r.size() - 1)};
                auto seg_ids = si::util::identity<unsigned>{};
                auto second_points = points + 1;   // Next point is current segment end

                auto soa = si::util::make_soa_reader<si::Segment>(
                    nrn_ids, seg_ids, points, second_points, r);

                // Start at begin() + 1 to skip soma
                for(auto iter = soa.begin() + 1; iter < soa.end() ; ++iter) {
                    obj.insert(*iter);
                }
            },
            "Bulk add a neuron (1 soma and many segments) to the spatial index")

        .def("find_intersecting",
            [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                std::vector<si::gid_segm_t> vec;
                obj.find_intersecting(si::Sphere{{cx, cy, cz}, r}, si::iter_gid_segm_getter(vec));
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
                 std::vector<si::gid_segm_t> vec;
                 obj.query(bgi::nearest(point_t{cx, cy, cz}, k_neighbors),
                           si::iter_gid_segm_getter(vec));
                 return pyutil::to_pyarray(vec);
             },
             "Searches and returns the ids of the nearest K objects to the given point.")

        .def("__len__", &Class::size);
}
