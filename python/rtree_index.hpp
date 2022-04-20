#pragma once
#include <iostream>
#include "bind_common.hpp"
#include <pybind11/eval.h>

namespace bg = boost::geometry;

namespace spatial_index {
namespace py_bindings {


///
/// 1 - Generic bindings
///

// We provide bindings to spatial indexes of spheres since they'r space efficient
inline void create_Sphere_bindings(py::module& m) {
    using Class = IndexedSphere;
    py::class_<Class>(m, "IndexedSphere")
        .def_property_readonly("centroid", [](Class& obj) {
                return py::array(3, reinterpret_cast<const si::CoordType*>(&obj.get_centroid()));
            },
            "Returns the centroid of the sphere"
        )
        .def_property_readonly("ids", [](Class& obj) {
                return std::make_tuple(long(obj.id));
            },
            "Return the id as a tuple (same API as other indexed objects)"
        )
        .def_property_readonly("id", [](Class& obj) {
                return long(obj.id);
            },
            "Returns the id of the indexed geometry"
        )
    ;
}


/// Generic IndexTree bindings. It is a common base between full in-memory
/// and disk-based memory mapped version, basically leaving ctors out

template <
    typename T,
    typename SomaT = T,
    typename Class = si::IndexTree<T>,
    typename HolderT = std::unique_ptr<Class>
>
inline py::class_<Class> generic_IndexTree_bindings(py::module& m,
                                                    const char* class_name) {
    return py::class_<Class, HolderT>(m, class_name)

    .def("insert",
        [](Class& obj, const id_t gid, const array_t& point, const coord_t radius) {
            obj.insert(SomaT{gid, mk_point(point), radius});
        },
        R"(
        Inserts a new sphere object in the tree.

        Args:
            gid(int): The id of the sphere
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("place",
        [](Class& obj, const array_t& region_corners,
                       const id_t gid, const array_t& center, const coord_t rad) {
            if (region_corners.ndim() != 2 || region_corners.size() != 6) {
                throw std::invalid_argument("Please provide a 2x3[float32] array");
            }
            const coord_t* c0 = region_corners.data(0, 0);
            const coord_t* c1 = region_corners.data(1, 0);
            return obj.place(si::Box3D{point_t(c0[0], c0[1], c0[2]),
                                       point_t(c1[0], c1[1], c1[2])},
                             SomaT{gid, mk_point(center), rad});
        },
        R"(
        Attempts to insert a sphere without overlapping any existing shape.

        place() will search the given volume region for a free spot for the
        given sphere. Whenever possible will insert it and return True,
        otherwise returns False.

        Args:
            region_corners(array): A 2x3 list/np.array of the region corners
                E.g. region_corners[0] is the 3D min_corner point.
            gid(int): The id of the sphere
            center(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("add_spheres",
        [](Class& obj, const array_t& centroids,
                       const array_t& radii,
                       const array_ids& py_ids) {
            const auto& point_radius = convert_input(centroids, radii);
            const auto& ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<SomaT>(
                ids, point_radius.first, point_radius.second);
            for (auto&& soma : soa) {
                obj.insert(soma);
            }
        },
        R"(
        Bulk add more spheres to the spatial index.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            radii(np.array): An array[float32] with the segments' radii
            py_ids(np.array): An array[int64] with the ids of the spheres
        )"
    )

    .def("add_points",
        [](Class& obj, const array_t& centroids, const array_ids& py_ids) {
            si::util::constant<coord_t> zero_radius(0);
            const auto points = convert_input(centroids);
            const auto& ids = py_ids.template unchecked<1>();
            auto soa = si::util::make_soa_reader<SomaT>(ids, points, zero_radius);
            for (auto&& soma : soa) {
                obj.insert(soma);
            }
        },
        R"(
        Bulk add more points to the spatial index.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            py_ids(np.array): An array[int64] with the ids of the points
        )"
    )

    .def("is_intersecting",
        [](Class& obj, const array_t& point, const coord_t radius) {
            return obj.is_intersecting(si::Sphere{mk_point(point), radius});
        },
        R"(
        Checks whether the given sphere intersects any object in the tree.

        Args:
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("find_intersecting",
        [](Class& obj, const array_t& point, const coord_t r) {
            const auto& vec = obj.find_intersecting(si::Sphere{mk_point(point), r});
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches objects intersecting the given sphere, and returns their ids.

        Args:
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    )

    .def("find_intersecting_window",
        [](Class& obj, const array_t& min_corner, const array_t& max_corner) {
            const auto& vec = obj.find_intersecting(si::Box3D{mk_point(min_corner),
                                                              mk_point(max_corner)});
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches objects intersecting the given window, and returns their ids.

        Args:
            min_corner, max_corner(float32) : min/max corners of the query window
        )"
    )

    .def("find_intersecting_window_pos",
        [](Class& obj, const array_t& min_corner, const array_t& max_corner) {
            const auto& vec = obj.find_intersecting_pos(si::Box3D{mk_point(min_corner),
                                                                  mk_point(max_corner)});
            return py::array(std::array<unsigned long, 2>{vec.size(), 3},
                             reinterpret_cast<const si::CoordType*>(vec.data()));
        },
        R"(
        Searches objects intersecting the given window, and returns their position.

        Args:
            min_corner, max_corner(float32) : min/max corners of the query window
        )"
    )

    .def("find_intersecting_objs",
        [](Class& obj, const array_t& centroid, const coord_t& radius) {
            return obj.find_intersecting_objs(si::Sphere{mk_point(centroid), radius});
        },
        R"(
        Searches objects intersecting the given Sphere, and returns them
        )"
    )

    .def("find_intersecting_window_objs",
        [](Class& obj, const array_t& min_corner, const array_t& max_corner) {
            return obj.find_intersecting_objs(si::Box3D{mk_point(min_corner),
                                                        mk_point(max_corner)});
        },
        R"(
        Searches objects intersecting the given Box3D, and returns them
        )"
    )

    .def("count_intersecting",
         [](Class& obj, const array_t& min_corner, const array_t& max_corner) {
             return obj.count_intersecting(si::Box3D{mk_point(min_corner),
                                                     mk_point(max_corner)});
         },
         R"(
         Count objects intersecting the given Box3D
         )"
    )

    .def("find_nearest",
        [](Class& obj, const array_t& point, const int k_neighbors) {
            const auto& vec = obj.find_nearest(mk_point(point), k_neighbors);
            return pyutil::to_pyarray(vec);
        },
        R"(
        Searches and returns the ids of the nearest K objects to the given point.

        Args:
            point(array): A len-3 list or np.array[float32] with the point
                to search around
            k_neighbors(int): The number of neighbour shapes to return
        )"
    )

    .def("__str__", [](Class& obj) {
        std::stringstream strs;
        strs << obj;
        return strs.str();
    })

    .def("__len__", &Class::size);
}


/// Bindings for IndexTree<T>, based on generic IndexTree<T> bindings

template <
    typename T,
    typename SomaT = T,
    typename Class = si::IndexTree<T>,
    std::enable_if_t<std::is_same<Class, si::IndexTree<T>>::value, int> = 0
>
inline py::class_<Class> create_IndexTree_bindings(py::module& m,
                                                   const char* class_name) {
    return generic_IndexTree_bindings<T, SomaT, Class>(m, class_name)

    .def(py::init<>(), "Constructor of an empty SpatialIndex.")

    /// Load tree dump
    .def(py::init<const std::string&>(),
        R"(
        Loads a Spatial Index from a dump()'ed file on disk.

        Args:
            filename(str): The file path to read the spatial index from.
        )"
    )

    .def(py::init([](const array_t& centroids, const array_t& radii) {
            if (!radii.ndim()) {
                si::util::constant<coord_t> zero_radius(0);
                const auto points = convert_input(centroids);
                const auto enum_ = si::util::identity<>{size_t(centroids.shape(0))};
                auto soa = si::util::make_soa_reader<SomaT>(enum_, points, zero_radius);
                return std::make_unique<Class>(soa.begin(), soa.end());
            } else {
                const auto& point_radius = convert_input(centroids, radii);
                const auto enum_ = si::util::identity<>{size_t(radii.shape(0))};
                auto soa = si::util::make_soa_reader<SomaT>(enum_,
                                                         point_radius.first,
                                                         point_radius.second);
                return std::make_unique<Class>(soa.begin(), soa.end());
            }
        }),
        py::arg("centroids"),
        py::arg("radii").none(true),
        R"(
        Creates a SpatialIndex prefilled with Spheres given their centroids and radii
        or Points (radii = None) automatically numbered.

        Args:
             centroids(np.array): A Nx3 array[float32] of the segments' end points
             radii(np.array): An array[float32] with the segments' radii, or None
        )"
    )

    .def(py::init([](const array_t& centroids,
                     const array_t& radii,
                     const array_ids& py_ids) {
            if (!radii.ndim()) {
                si::util::constant<coord_t> zero_radius(0);
                const auto points = convert_input(centroids);
                const auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<SomaT>(ids, points, zero_radius);
                return std::make_unique<Class>(soa.begin(), soa.end());
            } else {
                const auto& point_radius = convert_input(centroids, radii);
                const auto ids = py_ids.template unchecked<1>();
                auto soa = si::util::make_soa_reader<SomaT>(ids,
                                                            point_radius.first,
                                                            point_radius.second);
                return std::make_unique<Class>(soa.begin(), soa.end());
            }
        }),
        py::arg("centroids"),
        py::arg("radii").none(true),
        py::arg("py_ids"),
        R"(
        Creates a SpatialIndex prefilled with spheres with explicit ids
        or points with explicit ids and radii = None.

        Args:
            centroids(np.array): A Nx3 array[float32] of the segments' end points
            radii(np.array): An array[float32] with the segments' radii, or None
            py_ids(np.array): An array[int64] with the ids of the spheres
        )"
    )

    .def("dump", &Class::dump,
        R"(
        Save the spatial index tree to a file on disk.

        Args:
            filename(str): The file path to write the spatial index to.
        )"
    );
}


/// Bindings for IndexTreeMemDisk<T>, based on generic IndexTree<T> bindings

template <
    typename T,
    typename SomaT = T,
    typename Class = si::IndexTree<T>,
    std::enable_if_t<std::is_same<Class, si::MemDiskRtree<T>>::value, int> = 0
>
inline py::class_<Class> create_IndexTree_bindings(py::module& m, const char* class_name) {
    using MemDiskManager = si::MemDiskPtr<Class>;

    return generic_IndexTree_bindings<T, SomaT, Class, MemDiskManager>(m, class_name)
    .def_static("open", &MemDiskManager::open,
        py::return_value_policy::take_ownership,
        "Opens a SpatialIndex from a memory mapped file."
    )

    /// Build tree allocated in disk
    .def_static("create", &MemDiskManager::create,
        py::return_value_policy::take_ownership,
        R"(
        Creates a SpatialIndex where memory is mapped to a file.

        Args:
            filename(str): The file path to read the spatial index from.
            size_mb (int): The size of the file to allocate (avoid resizes)
            close_shrink (bool): Whether to shrink the mem mapped file to contents (experimental!)
        )",
        py::arg("fname"),
        py::arg("size_mb") = 1024,  // 1GB
        py::arg("close_shrink") = false
    )
    ;
}


///
/// 1.1 - Synapse index
///

inline void create_Synapse_bindings(py::module& m) {
    using Class = Synapse;
    py::class_<Class>(m, "Synapse")
        .def_property_readonly("centroid", [](Class& obj) {
                return py::array(3, reinterpret_cast<const si::CoordType*>(&obj.get_centroid()));
            },
            "The position of the synapse"
        )
        .def_property_readonly("ids", [](Class& obj) {
                return std::make_tuple(long(obj.id), long(obj.post_gid()));
            },
            "The Synapse ids as a tuple (id, gid)"
        )
        .def_property_readonly("id", [](Class& obj) {
                return long(obj.id);
            },
            "The Synapse id"
        )
        .def_property_readonly("post_gid",
            &Class::post_gid,
            "The post-synaptic Neuron id (gid)"
        )
        .def_property_readonly("pre_gid",
            &Class::pre_gid,
            "The pre-synaptic Neuron id (gid)"
        )
    ;
}


inline void create_SynapseIndex_bindings(py::module& m, const char* class_name) {
    using Class = si::IndexTree<Synapse>;

    create_IndexTree_bindings<si::Synapse>(m, class_name)

    .def("add_synapses",
        [](Class& obj, const array_ids& syn_ids, const array_ids& post_gids, const array_ids& pre_gids, const array_t& points) {
            const auto syn_ids_ = syn_ids.template unchecked<1>();
            const auto post_gids_ = post_gids.template unchecked<1>();
            const auto pre_gids_ = pre_gids.template unchecked<1>();
            const auto points_ = convert_input(points);
            auto soa = si::util::make_soa_reader<Synapse>(syn_ids_, post_gids_, pre_gids_, points_);
            obj.insert(soa.begin(), soa.end());
        },
        R"(
        Builds a synapse index.
        These indices maintain the gids as well to enable computing aggregated counts.
        )"
    )

    .def("count_intersecting_agg_gid",
        [](Class& obj, const array_t& min_corner, const array_t& max_corner) {
            return obj.count_intersecting_agg_gid(
                si::Box3D{mk_point(min_corner), mk_point(max_corner)}
            );
        },
        R"(
        Finds the points inside a given window and aggregated them by gid

        Args:
            min_corner, max_corner(float32) : min/max corners of the query window
        )"
    );
}


///
/// 2 - MorphIndex tree
///

/// Bindings for Base Type si::MorphoEntry

void create_MorphoEntry_bindings(py::module& m) {

    using Class = MorphoEntry;

    struct EndpointsVisitor: boost::static_visitor<const point_t*> {
        inline const point_t* operator()(const Segment& seg) const {
            return &seg.p1;
        }
        inline const point_t* operator()(const Soma&) const {
            return nullptr;
        }
    };

    py::class_<Class>(m, "MorphoEntry")
        .def_property_readonly("centroid", [](Class& obj) {
                const auto& p3d = boost::apply_visitor(
                    [](const auto& g){ return g.get_centroid(); },
                    obj
                );
                return py::array(3, &p3d.get<0>());
            },
            "Returns the centroid of the morphology parts as a Numpy array"
        )
        .def_property_readonly("endpoints",
            [](Class& obj) -> py::object {
                auto ptr = boost::apply_visitor(EndpointsVisitor(), obj);
                if (ptr == nullptr) {
                    return py::none();
                }
                return py::array({2, 3}, &ptr[0].get<0>());
            },
            "Returns the endpoints of the morphology parts as a Numpy array"
        )
        .def_property_readonly("ids", [](Class& obj) {
                return boost::apply_visitor([](const auto& g){
                    return std::make_tuple(g.gid(), g.section_id(), g.segment_id());
                }, obj);
            },
            "Return the tuple of ids, i.e. (gid, section_id, segment_id)"
        )
        .def_property_readonly("gid", [](Class& obj) {
                return boost::apply_visitor([](const auto& g){ return g.gid();}, obj);
            },
            "Returns the gid of the indexed morphology part"
        )
        .def_property_readonly("section_id", [](Class& obj) {
                return boost::apply_visitor([](const auto& g){ return g.section_id(); }, obj);
            },
            "Returns the section_id of the indexed morphology part"
        )
        .def_property_readonly("segment_id", [](Class& obj) {
                return boost::apply_visitor([](const auto& g){ return g.segment_id(); }, obj);
            },
            "Returns the segment_id of the indexed morphology part"
        )
    ;
}


/// Aux function to insert all segments of a branch
template <typename MorphIndexTree>
inline static void add_branch(MorphIndexTree& obj,
                              const id_t neuron_id,
                              unsigned section_id,
                              const unsigned n_segments,
                              const point_t* points,
                              const coord_t* radii) {
    // loop over segments. id is i + 1
    for (unsigned i = 0; i < n_segments; i++) {
        obj.insert(si::Segment{neuron_id, section_id, i , points[i], points[i + 1], radii[i]});
    }
}


/// Bindings to index si::IndexTree<MorphoEntry>
template <typename Class = si::IndexTree<MorphoEntry>>
inline void create_MorphIndex_bindings(py::module& m, const char* class_name) {

    create_IndexTree_bindings<MorphoEntry, si::Soma, Class>(m, class_name)

    .def("insert",
        [](Class& obj, const id_t gid, const unsigned section_id, const unsigned segment_id,
                       const array_t& p1, const array_t& p2, const coord_t radius) {
            obj.insert(si::Segment{gid, section_id, segment_id, mk_point(p1), mk_point(p2), radius});
        },
        R"(
        Inserts a new segment object in the tree.

        Args:
            gid(int): The id of the neuron
            section_id(int): The id of the section
            segment_id(int): The id of the segment
            p1(array): A len-3 list or np.array[float32] with the cylinder first point
            p2(array): A len-3 list or np.array[float32] with the cylinder second point
            radius(float): The radius of the cylinder
        )"
    )

    .def("place",
        [](Class& obj, const array_t& region_corners,
                       const id_t gid, const unsigned section_id, const unsigned segment_id,
                       const array_t& p1, const array_t& p2, const coord_t radius) {
            if (region_corners.ndim() != 2 || region_corners.size() != 6) {
                throw std::invalid_argument("Please provide a 2x3[float32] array");
            }
            const coord_t* c0 = region_corners.data(0, 0);
            const coord_t* c1 = region_corners.data(1, 0);
            return obj.place(si::Box3D{point_t(c0[0], c0[1], c0[2]),
                                       point_t(c1[0], c1[1], c1[2])},
                             si::Segment{gid, section_id, segment_id, mk_point(p1), mk_point(p2), radius});
        },
        R"(
        Attempts at inserting a segment without overlapping any existing shape.

        Args:
            region_corners(array): A 2x3 list/np.array of the region corners.\
                E.g. region_corners[0] is the 3D min_corner point.
            gid(int): The id of the neuron
            section_id(int): The id of the section
            segment_id(int): The id of the segment
            p1(array): A len-3 list or np.array[float32] with the cylinder first point
            p2(array): A len-3 list or np.array[float32] with the cylinder second point
            radius(float): The radius of the cylinder
        )"
    )

    .def("add_branch",
        [](Class& obj, const id_t gid, const unsigned section_id, const array_t& centroids_np,
                       const array_t& radii_np) {
            const auto& point_radii = convert_input(centroids_np, radii_np);
            add_branch(obj, gid, section_id, unsigned(radii_np.size() - 1), point_radii.first,
                       point_radii.second.data(0));
        },
        R"(
        Adds a branch, i.e., a line of cylinders.

        It adds a line of cylinders representing a branch. Each point in the centroids
        array is the beginning/end of a segment, and therefore it must be length N+1,
        where N is thre number of created cylinders.

        Args:
            gid(int): The id of the soma
            section_id(int): The id of the section
            centroids_np(np.array): A Nx3 array[float32] of the segments' end points
            radii_np(np.array): An array[float32] with the segments' radii
        )"
    )

    .def("add_soma",
        [](Class& obj, const id_t gid, const array_t& point, const coord_t radius) {
            obj.insert(si::Soma{gid, mk_point(point), radius});
        },
        R"(
        Adds a soma to the spatial index.

        Args:
            gid(int): The id of the soma
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the soma
        )"
    )

    .def("add_neuron",
        [](Class& obj, const id_t gid, const array_t& centroids_np, const array_t& radii_np,
                       const array_offsets& branches_offset_np,
                       bool has_soma) {
            const auto& point_radii = convert_input(centroids_np, radii_np);
            // Get raw pointers to data
            const auto points = point_radii.first;
            const auto npoints = centroids_np.shape(0);
            const auto radii = point_radii.second.data(0);
            const auto n_branches = branches_offset_np.size();
            const auto offsets = branches_offset_np.template unchecked<1>().data(0);

            // Check if at least one point was provided when has_soma==True
            if (has_soma && centroids_np.shape(0) < 1) {
                throw py::value_error("has_soma is True but no points provided");
            }

            const unsigned n_segment_points = npoints - has_soma;
            if (n_segment_points == 0) {
                if (has_soma && radii_np.size() != 1) {
                    throw py::value_error("Please provide the soma radius");
                }
                std::stringstream warn_cmd;
                warn_cmd << "import logging\n"
                         << "logging.warning('Neuron id=" << gid << " has no segments')\n";
                py::exec(warn_cmd.str());
            }
            else {                                          // -- segments sanity check --
                // Check that the number of points is two or more
                if (n_segment_points < 2) {
                    throw py::value_error("Please provide at least two points for segments");
                }

                if (radii_np.size() < centroids_np.shape(0) - 1) {
                    throw py::value_error("Please provide a radius per segment");
                }

                // Check that the branches are at least one
                if (n_branches < 1) {
                    throw py::value_error("Please provide at least one branch offset");
                }

                // Check that the branches are less than the number of supplied points
                if (n_branches > n_segment_points - 1) {
                    throw py::value_error("Too many branches given the supplied points");
                }

                // Check that the max offset is less than the number of points
                const auto max_offset = *std::max_element(offsets, offsets + n_branches);
                if (max_offset > npoints - 2) {  // 2 To ensure the segment has a closing point
                    throw py::value_error("At least one of the branches offset is too large");
                }
            }

            if (has_soma) {
                // Add soma
                obj.insert(si::Soma{gid, points[0], radii[0]});
            }

            // Add segments
            for (unsigned branch_i = 0; branch_i < n_branches - 1; branch_i++) {
                const unsigned p_start = offsets[branch_i];
                const unsigned n_segments = offsets[branch_i + 1] - p_start - 1;
                add_branch(obj, gid, branch_i + 1, n_segments, points + p_start,
                           radii + p_start);
            }
            // Last
            if (n_branches) {
                const unsigned p_start = offsets[n_branches - 1];
                const unsigned n_segments = npoints - p_start - 1;
                add_branch(obj, gid, n_branches, n_segments, points + p_start, radii + p_start);
            }
        },
        py::arg("gid"), py::arg("points"), py::arg("radii"), py::arg("branch_offsets"),
        py::arg("has_soma") = true,
        R"(
        Bulk add a neuron (1 soma and lines of segments) to the spatial index.

        It interprets the first point & radius as the soma properties. Subsequent
        points & radii are interpreted as branch segments (cylinders).
        The first point (index) of each branch must be specified in branches_offset_np,
        so that a new branch is started without connecting it to the last segment.

        has_soma = false:
        Bulk add neuron segments to the spatial index, soma point is not included.

        **Example:** Adding a neuron with two branches.
          With 1 soma, first branch with 9 segments and second branch with 5::

            ( S ).=.=.=.=.=.=.=.=.=.
                      .=.=.=.=.=.

          Implies 16 points. ('S' and '.'), and branches starting at points 1 and 11
          It can be created in the following way:

          >>> points = np.zeros([16, 3], dtype=np.float32)
          >>> points[:, 0] = np.concatenate((np.arange(11), np.arange(4, 10)))
          >>> points[11:, 1] = 1.0  # Change Y coordinate
          >>> radius = np.ones(N, dtype=np.float32)
          >>> rtree = MorphIndex()
          >>> rtree.add_neuron(1, points, radius, [1, 11])

        **Note:** There is not the concept of branching off from previous points.
        All branches start in a new point, the user can however provide a point
        close to an existing point to mimick branching.

        Args:
            gid(int): The id of the soma
            centroids_np(np.array): A Nx3 array[float32] of the segments' end points
            radii_np(np.array): An array[float32] with the segments' radii
            branches_offset_np(array): A list/array[int] with the offset to
                the first point of each branch
            has_soma : include the soma point or not, default = true
        )"
    );
}


}  // namespace py_bindings
}  // namespace spatial_index
