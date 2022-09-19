#pragma once
#include "bind_common.hpp"
#include <iostream>
#include <pybind11/eval.h>

#include <spatial_index/logging.hpp>

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

template<typename T, typename SomaT, typename Class>
inline void add_IndexTree_insert_bindings(py::class_<Class>& c) {
    c
    .def("_insert",
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
    );
}

template<typename T, typename SomaT, typename Class>
inline void add_IndexTree_place_bindings(py::class_<Class>& c) {
    c
    .def("_place",
        [](Class& obj, const array_t& region_corners,
                       const id_t gid, const array_t& center, const coord_t rad) {
            if (region_corners.ndim() != 2 || region_corners.size() != 6) {
                throw std::invalid_argument("Please provide a 2x3[float32] array");
            }
            const coord_t* c0 = region_corners.data(0, 0);
            const coord_t* c1 = region_corners.data(1, 0);
            return obj.place(
                si::make_query_box(
                    point_t(c0[0], c0[1], c0[2]),
                    point_t(c1[0], c1[1], c1[2])
                ),
                SomaT{gid, mk_point(center), rad}
            );
        },
        R"(
        Attempts to insert a sphere without overlapping any existing shape.

        place() will search the given volume region for a free spot for the
        given sphere. Whenever possible will insert it and return True,
        otherwise returns False.

        Args:
            region_corners(array): A 2x3 list/np.array of the region corners
            gid(int): The id of the sphere
            center(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
        )"
    );
}

template<typename Class>
inline void add_IndexTree_bounds_bindings(py::class_<Class>& c) {
    c
    .def("bounds",
        [](Class& obj) {
            auto box = obj.bounds();

            return py::make_tuple(
                pyutil::to_pyarray(std::vector<CoordType>{
                    bg::get<bg::min_corner, 0>(box),
                    bg::get<bg::min_corner, 1>(box),
                    bg::get<bg::min_corner, 2>(box)
                }),
                pyutil::to_pyarray(std::vector<CoordType>{
                    bg::get<bg::max_corner, 0>(box),
                    bg::get<bg::max_corner, 1>(box),
                    bg::get<bg::max_corner, 2>(box)
                })
            );
        },
        R"(
        The bounding box of all elements in the index.
        )"
    );
}

template<typename T, typename SomaT, typename Class>
inline void add_IndexTree_add_spheres_bindings(py::class_<Class>& c) {
    c
    .def("_add_spheres",
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
    );
}

template<typename T, typename SomaT, typename Class>
inline void add_IndexTree_add_points_bindings(py::class_<Class>& c) {
    c
    .def("_add_points",
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
    );
}

namespace detail {

template<typename Class, typename Shape>
inline decltype(auto)
is_intersecting(Class& obj, const Shape& query_shape, const std::string& geometry) {
    if(geometry == "bounding_box") {
        return obj.template is_intersecting<BoundingBoxGeometry>(query_shape);
    }

    if(geometry == "best_effort") {
        return obj.template is_intersecting<BestEffortGeometry>(query_shape);
    }

    throw std::runtime_error("Invalid geometry: " + geometry + ".");
}

template<typename Class, typename Shape>
inline decltype(auto)
find_intersecting_objs(Class& obj, const Shape& query_shape, const std::string& geometry) {
    if(geometry == "bounding_box") {
        return obj.template find_intersecting_objs<BoundingBoxGeometry>(query_shape);
    }

    if(geometry == "best_effort") {
        return obj.template find_intersecting_objs<BestEffortGeometry>(query_shape);
    }

    throw std::runtime_error("Invalid geometry: " + geometry + ".");
}

template<typename Class, typename Shape>
inline decltype(auto)
find_intersecting_np(Class& obj, const Shape& query_shape, const std::string& geometry) {
    if(geometry == "bounding_box") {
        return obj.template find_intersecting_np<BoundingBoxGeometry>(query_shape);
    }

    if(geometry == "best_effort") {
        return obj.template find_intersecting_np<BestEffortGeometry>(query_shape);
    }

    throw std::runtime_error("Invalid geometry: " + geometry + ".");
}

template<typename Class, typename Shape>
inline decltype(auto)
count_intersecting(Class& obj, const Shape& query_shape, const std::string& geometry) {
    if(geometry == "bounding_box") {
        return obj.template count_intersecting<BoundingBoxGeometry>(query_shape);
    }

    if(geometry == "best_effort") {
        return obj.template count_intersecting<BestEffortGeometry>(query_shape);
    }

    throw std::runtime_error("Invalid geometry: " + geometry + ".");
}

template<typename Class, typename Shape>
inline decltype(auto)
count_intersecting_agg_gid(Class& obj, const Shape& query_shape, const std::string& geometry) {
    if(geometry == "bounding_box") {
        return obj.template count_intersecting_agg_gid<BoundingBoxGeometry>(query_shape);
    }

    if(geometry == "best_effort") {
        return obj.template count_intersecting_agg_gid<BestEffortGeometry>(query_shape);
    }

    throw std::runtime_error("Invalid geometry: " + geometry + ".");
}

}

template<typename Class>
inline void add_IndexTree_is_intersecting_bindings(py::class_<Class>& c) {
    c
    .def("_is_intersecting",
        [](Class& obj, const array_t& point, const coord_t radius, const std::string& geometry) {
            return detail::is_intersecting(obj, si::Sphere{mk_point(point), radius}, geometry);
        },
        py::arg("point"),
        py::arg("radius"),
        py::arg("geometry") = std::string("bounding_box"),
        R"(
        Checks whether the given sphere intersects any object in the tree.

        Args:
            point(array): A len-3 list or np.array[float32] with the center point
            radius(float): The radius of the sphere
            geometry(str): Either 'bounding_box' or 'best_effort' (default: bounding_box).
        )"
    );
}


template<typename Class>
inline void add_IndexTree_find_intersecting_objs_bindings(py::class_<Class>& c) {
    c
    .def("_find_intersecting_objs",
        [](Class& obj, const array_t& centroid, const coord_t& radius, const std::string& geometry) {
            return detail::find_intersecting_objs(
                obj, si::Sphere{mk_point(centroid), radius}, geometry
            );
        },
        py::arg("centroid"),
        py::arg("radius"),
        py::arg("geometry") = std::string("bounding_box")
    );
}

template<typename Class>
inline void add_IndexTree_find_intersecting_box_objs_bindings(py::class_<Class>& c) {
    c
    .def("_find_intersecting_box_objs",
        [](Class& obj, const array_t& corner, const array_t& opposite_corner, const std::string& geometry) {
            return detail::find_intersecting_objs(
                obj, si::make_query_box(mk_point(corner), mk_point(opposite_corner)), geometry
            );
        },
        py::arg("corner"),
        py::arg("opposite_corner"),
        py::arg("geometry") = std::string("bounding_box")
    );
}



template<typename Class>
inline void add_MorphIndex_find_intersecting_box_np(py::class_<Class>& c) {
    auto wrap_results_in_dict = [](const auto& results) {
        auto centroid = py::array_t<CoordType>({results.centroid.size(), 3ul},
                                                (CoordType*)results.endpoint1.data());
        auto endpoint1 = py::array_t<CoordType>({results.endpoint1.size(), 3ul},
                                                (CoordType*)results.endpoint1.data());
        auto endpoint2 = py::array_t<CoordType>({results.endpoint2.size(), 3ul},
                                                (CoordType*)results.endpoint2.data());

        return py::dict(
            "gid"_a=pyutil::to_pyarray(results.gid), 
            "section_id"_a=pyutil::to_pyarray(results.section_id),
            "segment_id"_a=pyutil::to_pyarray(results.segment_id),
            "ids"_a=pyutil::to_pyarray(results.ids),
            "centroid"_a=centroid,
            "radius"_a=pyutil::to_pyarray(results.radius),
            "endpoints"_a=py::make_tuple(endpoint1, endpoint2),
            "is_soma"_a=pyutil::to_pyarray(results.is_soma)
        );
    };

    add_IndexTree_find_intersecting_box_np(c, wrap_results_in_dict);
}

template<typename Class>
inline void add_IndexTree_count_intersecting_bindings(py::class_<Class>& c) {
    c
    .def("_count_intersecting",
         [](Class& obj, const array_t& corner, const array_t& opposite_corner, const std::string& geometry) {
             return detail::count_intersecting(
                obj,
                si::make_query_box(mk_point(corner), mk_point(opposite_corner)),
                geometry);
         },
         py::arg("corner"),
         py::arg("opposite_corner"),
         py::arg("geometry") = std::string("bounding_box")
    )
    .def("_count_intersecting_sphere",
         [](Class& obj, const array_t& center, CoordType radius, const std::string& geometry) {
             return detail::count_intersecting(
                obj,
                si::Sphere{mk_point(center), radius},
                geometry);
         },
         py::arg("center"),
         py::arg("radius"),
         py::arg("geometry") = std::string("bounding_box")
    );
}

template<typename Class>
inline void add_IndexTree_find_nearest_bindings(py::class_<Class>& c) {
    c
    .def("_find_nearest",
        [](Class& obj, const array_t& point, const int k_neighbors) {
            const auto& vec = obj.find_nearest(mk_point(point), k_neighbors);
            return pyutil::to_pyarray(vec);
        }
    );
}

template<typename Class>
inline void add_str_for_streamable_bindings(py::class_<Class>& c) {
    c
    .def("__str__", [](Class& obj) {
        std::stringstream strs;
        strs << obj;
        return strs.str();
    });
}

template<typename Class>
inline void add_len_for_size_bindings(py::class_<Class>& c) {
    c
    .def("__len__", [](const Class& obj) { return obj.size(); });
}

/// Generic IndexTree bindings. It is a common base between full in-memory
/// and disk-based memory mapped version, basically leaving ctors out

template<typename Class>
inline void add_IndexTree_query_bindings(py::class_<Class> &c) {
    add_IndexTree_is_intersecting_bindings<Class>(c);

    add_IndexTree_find_intersecting_objs_bindings<Class>(c);
    add_IndexTree_find_intersecting_box_objs_bindings<Class>(c);

    add_IndexTree_count_intersecting_bindings<Class>(c);

    add_IndexTree_find_nearest_bindings<Class>(c);
}

template <
    typename T,
    typename SomaT = T,
    typename Class = si::IndexTree<T>,
    typename HolderT = std::unique_ptr<Class>
>
inline py::class_<Class> generic_IndexTree_bindings(py::module& m,
                                                    const char* class_name) {
    py::class_<Class> c = py::class_<Class, HolderT>(m, class_name);
    add_IndexTree_place_bindings<T, SomaT, Class>(c);
    add_IndexTree_insert_bindings<T, SomaT, Class>(c);
    add_IndexTree_add_spheres_bindings<T, SomaT, Class>(c);
    add_IndexTree_add_points_bindings<T, SomaT, Class>(c);

    add_IndexTree_query_bindings(c);

    add_IndexTree_bounds_bindings(c);
    add_str_for_streamable_bindings<Class>(c);
    add_len_for_size_bindings<Class>(c);

    return c;
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

    .def("_dump",
        [](const Class& obj, const std::string& filename) { obj.dump(filename); },
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
    .def_static("open",
        [](const std::string& filename) {
            return MemDiskManager::open(filename);
        },
        py::return_value_policy::take_ownership,
        R"(
        Opens a SpatialIndex from a memory mapped file.

        Args:
            filename(str): The path of the memory mapped file.
        )"
    )

    /// Build tree allocated in disk
    .def_static("create",
        [](const std::string& filename, size_t size_mb, bool close_shrink) {
            return MemDiskManager::create(filename, size_mb, close_shrink);
        },
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
/// 1.0 - Sphere index
///

template <typename Class>
inline void add_SphereIndex_fields_bindings(py::class_<Class>& c) {
    c.def_property_readonly_static(
        "builtin_fields",
        [](py::object& /* self */) {
            return std::vector<std::string>{
                "id",
                "centroid",
                "radius"
            };
        },
        "List of fields that are builtin."
    );
}

template<typename Class, typename WrapAsDict>
inline void add_IndexTree_find_intersecting_box_np(
        py::class_<Class>& c,
        const WrapAsDict& wrap_as_dict) {

    c
    .def("_find_intersecting_box_np",
            [wrap_as_dict](Class& obj,
                           const array_t& corner, const array_t& opposite_corner,
                           const std::string& geometry) {

                const auto& results = detail::find_intersecting_np(
                    obj,
                    si::make_query_box(mk_point(corner), mk_point(opposite_corner)),
                    geometry
                );
                return wrap_as_dict(results);
            },
            py::arg("corner"),
            py::arg("opposite_corner"),
            py::arg("geometry") = std::string("bounding_box")
        );

    c
    .def("_find_intersecting_np",
            [wrap_as_dict](Class& obj,
                           const array_t& center, CoordType radius,
                           const std::string& geometry) {
                const auto& results = detail::find_intersecting_np(
                    obj,
                    si::Sphere{mk_point(center), radius},
                    geometry
                );

                return wrap_as_dict(results); 
            },
            py::arg("center"),
            py::arg("radius"),
            py::arg("geometry") = std::string("bounding_box")
        );
    
}

template<typename Class>
inline void add_SphereIndex_find_intersecting_box_np(py::class_<Class>& c) {
    auto wrap_as_dict = [](const auto& results) {
        return py::dict(
            "id"_a=pyutil::to_pyarray(results.id),
            "centroid"_a=py::array_t<CoordType>({results.centroid.size(), 3ul},
                                                (CoordType*)results.centroid.data()),
            "radius"_a=pyutil::to_pyarray(results.radius)
        );
    };

    add_IndexTree_find_intersecting_box_np(c, wrap_as_dict);
}

template <typename Class = si::IndexTree<si::IndexedSphere>>
inline void create_SphereIndex_bindings(py::module& m, const char* class_name) {
    using value_type = typename Class::value_type;
    auto c = create_IndexTree_bindings<value_type, value_type, Class>(m, class_name);

    add_SphereIndex_find_intersecting_box_np(c);
    add_SphereIndex_fields_bindings(c);
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
            [](Class& obj) { return obj.post_gid(); },
            "The post-synaptic Neuron id (gid)"
        )
        .def_property_readonly("pre_gid",
            [](Class& obj) { return obj.pre_gid(); },
            "The pre-synaptic Neuron id (gid)"
        )
    ;
}


template<typename Class>
inline void add_SynapseIndex_find_intersecting_box_np(py::class_<Class>& c) {
    auto wrap_as_dict = [](const auto& results) {
        return py::dict(
            "id"_a=pyutil::to_pyarray(results.id),
            "pre_gid"_a=pyutil::to_pyarray(results.pre_gid),
            "post_gid"_a=pyutil::to_pyarray(results.post_gid),
            "position"_a=py::array_t<CoordType>({results.position.size(), 3ul},
                                                (CoordType*)results.position.data())
        );
    };

    add_IndexTree_find_intersecting_box_np(c, wrap_as_dict);
}


template <typename Class>
inline void add_SynapseIndex_fields_bindings(py::class_<Class>& c) {
    c.def_property_readonly_static(
        "builtin_fields",
        [](py::object& /* self */) {
            return std::vector<std::string>{
                "id",
                "pre_gid",
                "post_gid",
                "position"
            };
        },
        "List of fields that are builtin."
    );
}


template<class Class>
inline void add_SynapseIndex_add_synapses_bindings(py::class_<Class>& c) {
    c
    .def("_add_synapses",
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
    );
}


template<class Class>
inline void add_SynapseIndex_count_intersecting_agg_gid_bindings(py::class_<Class>& c) {
    c
    .def("_count_intersecting_agg_gid",
        [](Class& obj,
           const array_t& corner, const array_t& opposite_corner,
           const std::string& geometry) {

            return detail::count_intersecting_agg_gid(obj, 
                si::make_query_box(mk_point(corner), mk_point(opposite_corner)),
                geometry
            );
        },
        py::arg("corner"),
        py::arg("opposite_corner"),
        py::arg("geometry") = std::string("bounding_box")
    )

    .def("_count_intersecting_sphere_agg_gid",
        [](Class& obj,
           const array_t& center, CoordType radius,
           const std::string& geometry) {

            return detail::count_intersecting_agg_gid(obj, 
                si::Sphere{mk_point(center), radius},
                geometry
            );
        },
        py::arg("point"),
        py::arg("radius"),
        py::arg("geometry") = std::string("bounding_box")
    );
}


template <typename Class = si::IndexTree<si::Synapse>>
inline void create_SynapseIndex_bindings(py::module& m, const char* class_name) {
    using value_type = typename Class::value_type;
    auto c = create_IndexTree_bindings<value_type, value_type, Class>(m, class_name);

    add_SynapseIndex_add_synapses_bindings(c);
    add_SynapseIndex_count_intersecting_agg_gid_bindings(c);
    add_SynapseIndex_find_intersecting_box_np(c);
    add_SynapseIndex_fields_bindings(c);
}


///
/// 2 - MorphIndex tree
///

/// Bindings for Base Type si::MorphoEntry

inline void create_MorphoEntry_bindings(py::module& m) {

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
        // TODO reconsider this choice of using only `radii[i]`; because the
        // input frequently has one radius per cap. Not one radius per cylinder.
        obj.insert(si::Segment{neuron_id, section_id, i , points[i], points[i + 1], radii[i]});
    }
}

template <typename Class>
inline void add_MorphIndex_insert_bindings(py::class_<Class>& c) {
    c
    .def("_insert",
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
    );
}

template <typename Class>
inline void add_MorphIndex_place_bindings(py::class_<Class>& c) {
    c
    .def("_place",
        [](Class& obj, const array_t& region_corners,
                       const id_t gid, const unsigned section_id, const unsigned segment_id,
                       const array_t& p1, const array_t& p2, const coord_t radius) {
            if (region_corners.ndim() != 2 || region_corners.size() != 6) {
                throw std::invalid_argument("Please provide a 2x3[float32] array");
            }
            const coord_t* c0 = region_corners.data(0, 0);
            const coord_t* c1 = region_corners.data(1, 0);
            return obj.place(
                si::make_query_box(
                    point_t(c0[0], c0[1], c0[2]),
                    point_t(c1[0], c1[1], c1[2])
                ),
                si::Segment{gid, section_id, segment_id, mk_point(p1), mk_point(p2), radius}
            );
        },
        R"(
        Attempts at inserting a segment without overlapping any existing shape.

        Args:
            region_corners(array): A 2x3 list/np.array of the region corners.
            gid(int): The id of the neuron
            section_id(int): The id of the section
            segment_id(int): The id of the segment
            p1(array): A len-3 list or np.array[float32] with the cylinder first point
            p2(array): A len-3 list or np.array[float32] with the cylinder second point
            radius(float): The radius of the cylinder
        )"
    );
}

template <typename Class>
inline void add_MorphIndex_add_branch_bindings(py::class_<Class>& c) {
    c
    .def("_add_branch",
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
    );
}

template <typename Class>
inline void add_MorphIndex_add_soma_bindings(py::class_<Class>& c) {
    c
    .def("_add_soma",
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
    );
}

template <typename Class>
inline void add_MorphIndex_add_neuron_bindings(py::class_<Class>& c) {
    c
    .def("_add_neuron",
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
            if (has_soma && centroids_np.shape(0) == 0) {
                throw py::value_error("has_soma is True but no points provided");
            }

            const unsigned n_segment_points = npoints - has_soma;

            if (n_segment_points == 0) {
                if (has_soma && radii_np.size() != 1) {
                    throw py::value_error("Please provide the soma radius");
                }

                log_warn(boost::format("Neuron id=%d has no segments") % gid);
            }
            else {                                          // -- segments sanity check --
                // Check that the number of points is two or more
                if (n_segment_points < 2) {
                    throw py::value_error("Please provide at least two points for segments");
                }

                if (radii_np.size() != npoints) {
                    log_error(
                        boost::format("We require exactly one radius per point,"
                            " as if the section had a piecewise varying radius."
                            " Regardless, SI only has cylinders, i.e., constant radius"
                            " per segment. We ignore the second radius and only take"
                            " the first. Found: radii.size = %d, npoints = %d.")
                        % radii_np.size() % npoints
                    );

                    throw py::value_error(
                        "Please provide exactly one radius for each point."
                    );
                }

                // Check that the branches are at least one
                if (n_branches == 0) {
                    throw py::value_error("Please provide at least one branch offset");
                }

                // Check that the branches are less than the number of supplied points
                if (n_branches > n_segment_points - 1) {
                    throw py::value_error("Too many branches given the supplied points");
                }

                // Check that the max offset is less than the number of points
                for(size_t i = 0; i < n_branches-1; ++i) {
                    if(offsets[i] > offsets[i+1]) {
                        throw py::value_error("The 'branch_offsets' must be non-decreasing.");
                    }
                }

                if (offsets[n_branches - 1] > npoints - 2) {  // 2 To ensure the segment has a closing point
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
        close to an existing point to mimic branching.

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


template <typename Class>
inline void add_MorphIndex_fields_bindings(py::class_<Class>& c) {
    c.def_property_readonly_static(
        "builtin_fields",
        [](py::object& /* self */) {
            return std::vector<std::string>{
                "gid",
                "section_id",
                "segment_id",
                "ids",
                "centroid",
                "endpoints",
                "radius",
                "is_soma"
            };
        },
        "List of fields that are builtin."
    );
}


/// Bindings to index si::IndexTree<MorphoEntry>
template <typename Class = si::IndexTree<MorphoEntry>>
inline void create_MorphIndex_bindings(py::module& m, const char* class_name) {
    auto c = create_IndexTree_bindings<MorphoEntry, si::Soma, Class>(m, class_name);

    add_MorphIndex_insert_bindings<Class>(c);
    add_MorphIndex_place_bindings<Class>(c);

    add_MorphIndex_add_branch_bindings<Class>(c);
    add_MorphIndex_add_neuron_bindings<Class>(c);
    add_MorphIndex_add_soma_bindings<Class>(c);

    add_MorphIndex_find_intersecting_box_np<Class>(c);

    add_MorphIndex_fields_bindings<Class>(c);
}

#if SI_MPI == 1

template<typename Class>
inline void add_MultiIndexBulkBuilder_creation_bindings(py::class_<Class>& c) {
    c
    .def(py::init<std::string>(),
         py::arg("output_dir"),
         R"(
        Create a `MultiIndexBulkBuilder` that writes output to `output_dir`.

        A `MultiIndexBulkBuilder` is an interface to build a multi index. Currently,
        a multi index can only be built in bulk. Meaning first all elements to be
        indexed are loaded, then the index is created. As a consequence, the multi
        index in only created once `finalize` is called.

        Args:
            output_dir(string):  The directory where the all files that make up
                the multi index are stored.
        )"
    )

    .def("reserve",
         [](Class &obj, std::size_t n_local_elements) {
             obj.reserve(n_local_elements);
         },
         R"(
        Reserve space for the elements to be inserted into the index.

        In order to improve memory efficiency, the `MultiIndexBulkBuilder`
        needs to know how many elements will be inserted into the spatial
        index.

        Args:
            n_local_elements(int): Number of elements this MPI ranks will insert
                into the index.
        )"
    )

    .def("_finalize",
         [](Class &obj) {
            auto comm_size = mpi::size(MPI_COMM_WORLD);
            auto comm = mpi::comm_shrink(MPI_COMM_WORLD, comm_size - 1);
            if(*comm != comm.invalid_handle()) {
                obj.finalize(*comm);
            }
         },
         R"(
        This will trigger building the multi index in bulk.
        )"
    );

}

template<typename Class>
inline void add_MultiIndexBulkBuilder_local_size_bindings(py::class_<Class>& c) {
    c
    .def("local_size",
         [](Class &obj) {
             return obj.local_size();
         },
         R"(
    The current number of elements to be added to the index by this MPI rank.
    )"
    );
}


template <typename Value, typename Class=si::MultiIndexBulkBuilder<Value>>
inline py::class_<Class>
create_MultiIndexBulkBuilder_bindings(py::module& m, const char* class_name) {
    py::class_<Class> c = py::class_<Class>(m, class_name);

    add_MultiIndexBulkBuilder_creation_bindings(c);
    add_MultiIndexBulkBuilder_local_size_bindings(c);

    add_len_for_size_bindings(c);

    return c;
}


template <typename Class = si::MultiIndexBulkBuilder<MorphoEntry>>
inline void create_MorphMultiIndexBulkBuilder_bindings(py::module& m, const char* class_name) {
    py::class_<Class> c = create_MultiIndexBulkBuilder_bindings<MorphoEntry>(m, class_name);

    add_IndexTree_insert_bindings<MorphoEntry, si::Soma, Class>(c);
    add_MorphIndex_insert_bindings<Class>(c);

    add_MorphIndex_add_branch_bindings<Class>(c);
    add_MorphIndex_add_neuron_bindings<Class>(c);
    add_MorphIndex_add_soma_bindings<Class>(c);
}


template <typename Class = si::MultiIndexBulkBuilder<Synapse>>
inline void create_SynapseMultiIndexBulkBuilder_bindings(py::module& m, const char* class_name) {
    py::class_<Class> c = create_MultiIndexBulkBuilder_bindings<Synapse>(m, class_name);

    add_IndexTree_insert_bindings<Synapse, Synapse, Class>(c);
    add_SynapseIndex_add_synapses_bindings<Class>(c);
}

#endif

template <typename Value, typename Class = si::MultiIndexTree<Value>>
inline py::class_<Class> create_MultiIndex_bindings(py::module& m, const char* class_name) {
    py::class_<Class> c = py::class_<Class>(m, class_name);

    c
    .def(py::init<std::string, std::size_t>(),
         py::arg("output_dir"),
         py::arg("max_cached_bytes"),
         R"(
        Create a `MultiIndexBulkBuilder` that writes output to `output_dir`.

        A `MultiIndexBulkBuilder` is an interface to build a multi index. Currently,
        a multi index can only be built in bulk. Meaning first all elements to be
        indexed are loaded, then the index is created. As a consequence, the multi
        index in only created once `finalize` is called.

        Args:
            output_dir(string):  The directory where the all files that make up
                the multi index are stored.

            max_cached_bytes(int):  The total size of the index should, up to a
                log factor, not use more than `max_cached_bytes` bytes of memory.
        )"
    );

    add_IndexTree_query_bindings(c);

    add_IndexTree_bounds_bindings(c);
    add_len_for_size_bindings(c);

    return c;
}


template <typename Class = si::MultiIndexTree<MorphoEntry>>
inline py::class_<Class> create_MorphMultiIndex_bindings(py::module& m, const char* class_name) {
    using value_type = typename Class::value_type;
    auto c = create_MultiIndex_bindings<value_type>(m, class_name);

    add_MorphIndex_find_intersecting_box_np(c);
    add_MorphIndex_fields_bindings(c);

    return c;
}


template <typename Class = si::MultiIndexTree<Synapse>>
inline py::class_<Class> create_SynapseMultiIndex_bindings(py::module& m, const char* class_name) {
    using value_type = typename Class::value_type;
    auto c = create_MultiIndex_bindings<value_type>(m, class_name);

    add_SynapseIndex_find_intersecting_box_np(c);
    add_SynapseIndex_fields_bindings(c);

    return c;
}

inline void create_MetaDataConstants_bindings(py::module& m) {
    py::class_<MetaDataConstants> c = py::class_<MetaDataConstants>(m, "_MetaDataConstants");

    c
    .def_property_readonly_static("version", [](py::object /* self */) {
        return MetaDataConstants::version;
    })

    .def_property_readonly_static("memory_mapped_key", [](py::object /* self */) {
        return py::str(MetaDataConstants::memory_mapped_key);
    })

    .def_property_readonly_static("in_memory_key", [](py::object /* self */) {
        return py::str(MetaDataConstants::in_memory_key);
    })

    .def_property_readonly_static("multi_index_key", [](py::object /* self */) {
        return py::str(MetaDataConstants::multi_index_key);
    });

    // Related free functions.
    m.def("deduce_meta_data_path", [](const std::string& path) {
        return deduce_meta_data_path(path);
    });

    m.def("default_meta_data_path", [](const std::string& path) {
        return default_meta_data_path(path);
    });
}

}  // namespace py_bindings
}  // namespace spatial_index
