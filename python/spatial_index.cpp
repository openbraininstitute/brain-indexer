#include "spatial_index.hpp"


struct py_sphere_rtree : public py_rtree<si::IndexedSphere>{
    using rtree_type = py_rtree<si::IndexedSphere>;

    inline void make_bindings(py::module& m) {
        init_class_bindings(m, "SphereIndex")
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

    inline static void add_branch(Class& obj,
                                  id_t neuron_id, unsigned segment_i, unsigned n_segments,
                                  const point_t *points, const coord_t *radii) {
        for(int i=0; i < n_segments; i++, segment_i++) {
            obj.insert(si::Segment{
                neuron_id, segment_i, points[i], points[i+1], radii[i]});
        }
    }

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
                "add_branch",
                [](Class& obj, id_t neuron_id, unsigned segment_i,
                               array_t centroids_np, array_t radii_np) {
                    auto point_radii = convert_input(centroids_np, radii_np);
                    add_branch(
                        obj, neuron_id, segment_i, radii_np.size() - 1,
                        point_radii.first, point_radii.second.data(0)
                    );
                },
                "Adds a branch, i.e., a line of cylinders")

            .def(
                "add_neuron",
                [](Class& obj, id_t neuron_id,
                               array_t centroids_np, array_t radii_np,
                               array_offsets branches_offset_np) {
                    auto point_radii = convert_input(centroids_np, radii_np);
                    // Get raw pointers to data
                    const auto points = point_radii.first;
                    const auto radii = point_radii.second.data(0);
                    auto n_branches = branches_offset_np.size();
                    const auto offsets = branches_offset_np.template unchecked<1>().data(0);

                    // Add soma
                    obj.insert(si::Soma{neuron_id, points[0], radii[0]});

                    // Add segments
                    int cur_segment_i = 1;
                    for (unsigned branch_i=0; branch_i < n_branches - 1; branch_i++) {
                        unsigned p_start = offsets[branch_i];
                        unsigned n_segments = offsets[branch_i + 1] - p_start - 1;
                        add_branch(obj, neuron_id, cur_segment_i, n_segments,
                                        points + p_start,
                                        radii + p_start);
                        cur_segment_i += n_segments;
                    }
                    // Last
                    unsigned p_start = offsets[n_branches - 1];
                    unsigned n_segments = radii_np.size() - p_start - 1;
                    add_branch(obj, neuron_id, cur_segment_i, n_segments,
                                    points + p_start,
                                    radii + p_start);
                },
                "Bulk add a neuron (1 soma and lines of segments) to the spatial index")

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
