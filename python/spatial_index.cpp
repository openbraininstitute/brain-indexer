#include "spatial_index.hpp"


struct py_sphere_rtree : public py_rtree<si::IndexedSphere>{
    using rtree_type = py_rtree<si::IndexedSphere>;

    inline void make_bindings(py::module& m) {
        init_class_bindings(m, "SomaIndex")
            .def(
                "find_intersecting",
                [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                    std::vector<si::identifier_t> vec;
                    obj.find_intersecting(si::Sphere{{cx, cy, cz}, r}, si::iter_ids_getter(vec));
                    return pyutil::to_pyarray(vec);
                },
                "Searches objects intersecting the given sphere, and returns their ids.")

            .def(
                "find_nearest",
                [](Class& obj, coord_t cx, coord_t cy, coord_t cz, int k_neighbors) {
                    std::vector<si::identifier_t> vec;
                    obj.query(bgi::nearest(point_t{cx, cy, cz}, k_neighbors),
                              si::iter_ids_getter(vec));
                    return pyutil::to_pyarray(vec);
                },
                "Searches and returns the ids of the nearest K objects to the given point.");
    }
};


struct py_morph_rtree: public py_rtree<si::MorphoEntry, si::Soma> {
    using rtree_type = py_rtree<si::MorphoEntry, si::Soma>;

    inline void make_bindings(py::module& m) {
        init_class_bindings(m, "MorphIndex")
            .def(
                "insert",
                [](Class& obj, id_t i, unsigned part_id, coord_t p1_cx, coord_t p1_cy,
                   coord_t p1_cz, coord_t p2_cx, coord_t p2_cy, coord_t p2_cz, coord_t r) {
                    obj.insert(si::Segment{i, part_id,
                                           point_t{p1_cx, p1_cy, p1_cz},
                                           point_t{p2_cx, p2_cy, p2_cz}, r});
                },
                "Inserts a new segment object in the tree.")

            .def(
                "add_neuron",
                [](Class& obj, id_t neuron_id, array_t centroids, array_t radii) {
                    auto point_radius = convert_input(centroids, radii);
                    const auto& points = point_radius.first;

                    // Add soma
                    obj.insert(si::Soma{neuron_id, points[0], point_radius.second[0]});

                    // Create soa reader for data as segments
                    auto nrn_ids = si::util::constant<>{neuron_id, size_t(radii.size() - 1)};
                    auto seg_ids = si::util::identity<unsigned>{};
                    auto second_points = points + 1;  // Next point is current segment end

                    auto soa = si::util::make_soa_reader<si::Segment>(nrn_ids, seg_ids, points,
                                                                      second_points,
                                                                      point_radius.second);

                    // Start at begin() + 1 to skip soma
                    for (auto iter = soa.begin() + 1; iter < soa.end(); ++iter) {
                        obj.insert(*iter);
                    }
                },
                "Bulk add a neuron (1 soma and many segments) to the spatial index")

            .def(
                "find_intersecting",
                [](Class& obj, coord_t cx, coord_t cy, coord_t cz, coord_t r) {
                    std::vector<si::gid_segm_t> vec;
                    obj.find_intersecting(si::Sphere{{cx, cy, cz}, r},
                                          si::iter_gid_segm_getter(vec));
                    return pyutil::to_pyarray(vec);
                },
                "Searches objects intersecting the given sphere, and returns their ids.")

            .def(
                "find_nearest",
                [](Class& obj, coord_t cx, coord_t cy, coord_t cz, int k_neighbors) {
                    std::vector<si::gid_segm_t> vec;
                    obj.query(bgi::nearest(point_t{cx, cy, cz}, k_neighbors),
                              si::iter_gid_segm_getter(vec));
                    return pyutil::to_pyarray(vec);
                },
                "Searches and returns the ids of the nearest K objects to the given point.");
    }
};


PYBIND11_MODULE(_spatial_index, m) {
    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, segment_i);  // Pybind11 wow!

    py_sphere_rtree{}.make_bindings(m);
    py_morph_rtree{}.make_bindings(m);
}
