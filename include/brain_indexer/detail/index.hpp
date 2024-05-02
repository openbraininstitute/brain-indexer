#pragma once

#include "../index.hpp"

#include <fstream>
#include <iostream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iterator/function_output_iterator.hpp>

#include "output_iterators.hpp"

namespace brain_indexer {


// Specialization of geometry_intersects for variant geometries
template <typename T, typename... VarT, typename GeometryMode>
bool geometry_intersects(const T& query_shape,
                         const boost::variant<VarT...>& element_shape,
                         GeometryMode geo) {

    return boost::apply_visitor(
        [&query_shape, geo](const auto& e1) {
            return geometry_intersects(query_shape, e1, geo);
        },
        element_shape);
}

template <typename T>
inline Point3D get_centroid(const T& geometry) {
    return geometry.get_centroid();
}

template <typename... VariantT>
inline Point3D get_centroid(const boost::variant<VariantT...>& mixed_geometry) {
    return boost::apply_visitor(
        [](const auto& geom) { return geom.get_centroid(); },
        mixed_geometry
    );
}

template<class Value>
std::string value_to_element_type() {
    return "unknown";
}

template<>
inline std::string value_to_element_type<IndexedSphere>() {
    return "sphere";
}

template <>
inline std::string value_to_element_type<IndexedPoint>() {
    return "point";
}

template<>
inline std::string value_to_element_type<MorphoEntry>() {
    return "morphology";
}

template<>
inline std::string value_to_element_type<Synapse>() {
    return "synapse";
}


/////////////////////////////////////////
// class IndexTree
/////////////////////////////////////////

template <typename Derived, typename T>
template <typename GeometryMode, typename ShapeT, typename OutputIt>
inline void IndexTreeMixin<Derived, T>::find_intersecting(const ShapeT& shape,
                                                          const OutputIt& iter) const {

    const auto &derived = static_cast<const Derived&>(*this);
    // Using a callback makes the query slightly faster than using qbegin()...qend()
    auto real_intersects = [&shape](const auto &v) {
        return geometry_intersects(shape, v, GeometryMode{});
    };

    derived.query(
        bgi::intersects(bgi::indexable<ShapeT>{}(shape)) && bgi::satisfies(real_intersects),
        iter);
}


// Function to return payload data as a numpy arrays

template <typename Derived, typename T>
template <typename GeometryMode, typename ShapeT>
inline decltype(auto) 
IndexTreeMixin<Derived, T>::find_intersecting_np(const ShapeT& shape) const {
    using getter_t = iter_entry_getter<T>;
    typename getter_t::result_t result;
    find_intersecting<GeometryMode>(shape, getter_t(result));
    return result;
}


template <typename Derived, typename T>
template <typename GeometryMode, typename ShapeT>
inline size_t IndexTreeMixin<Derived, T>::count_intersecting(const ShapeT& shape) const {
    size_t cardinality = 0; // number of matches in set
    auto counter = boost::make_function_output_iterator(
        [&cardinality](const auto&) { ++cardinality; }
    );

    find_intersecting<GeometryMode>(shape, counter);
    return cardinality;
}

template <typename Derived, typename T>
template <typename GeometryMode, typename ShapeT>
inline std::unordered_map<identifier_t, size_t>
IndexTreeMixin<Derived, T>::count_intersecting_agg_gid(const ShapeT& shape) const {
    std::unordered_map<identifier_t, size_t> counts;
    auto counter = boost::make_function_output_iterator(
        [&counts](const auto& elem) {
            counts[elem.post_gid()] += 1;
        }
    );

    find_intersecting<GeometryMode>(shape, counter);
    return counts;
}

template <typename Derived, typename T>
template <typename ShapeT>
inline decltype(auto) IndexTreeMixin<Derived, T>::find_nearest(const ShapeT& shape,
                                                               unsigned k_neighbors) const {
    const auto& derived = static_cast<const Derived&>(*this);

    using ids_getter = typename detail::id_getter_for<T>::type;
    std::vector<typename ids_getter::value_type> ids;
    derived.query(bgi::nearest(shape, k_neighbors), ids_getter(ids));
    return ids;
}


// Serialization: Load ctor
template <typename T, typename A>
inline IndexTree<T, A>::IndexTree(const std::string& path) {
    auto filename = resolve_heavy_data_path(path, MetaDataConstants::in_memory_key);

    auto ifs = util::open_ifstream(filename, std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
}


template <typename T, typename A>
inline void IndexTree<T, A>::dump(const std::string& index_path) const {
    util::ensure_valid_output_directory(index_path);

    auto heavy_data_relpath = "index.spi";
    auto filename = join_path(index_path, heavy_data_relpath);
    auto ofs = util::open_ofstream(filename, std::ios::binary | std::ios::trunc);
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;

    auto element_type = value_to_element_type<T>();
    auto meta_data = create_basic_meta_data(element_type);

    meta_data[MetaDataConstants::in_memory_key] = {
        // The heavy data, i.e. the relative path of the serialization
        // of the index:
        {"heavy_data_path", heavy_data_relpath}
    };

    write_meta_data(default_meta_data_path(index_path), meta_data);
}


template <typename T, typename A>
template <typename GeometryMode, typename ShapeT>
inline bool IndexTree<T, A>::is_intersecting(const ShapeT& shape) const {
    auto real_intersects = [&shape](const auto& v) {
        return geometry_intersects(shape, v, GeometryMode{});
    };

    auto it = this->qbegin(
        bgi::intersects(bgi::indexable<ShapeT>{}(shape)) && bgi::satisfies(real_intersects)
    );

    return it != this->qend();
}


template <typename T, typename A>
template <typename GeometryMode, typename ShapeT>
inline std::vector<typename IndexTree<T, A>::cref_t>
IndexTree<T, A>::find_intersecting_objs(const ShapeT& shape) const {
    std::vector<cref_t> results;
    this->template find_intersecting<GeometryMode>(shape, std::back_inserter(results));
    return results;
}


template <typename T, typename A>
template <typename ShapeT>
inline bool IndexTree<T, A>::place(const Box3D& region, ShapeT& shape) {
    // Align shape bbox to region bbox
    const Box3D region_bbox = bgi::indexable<Box3D>{}(region);
    const Box3D bbox = bgi::indexable<ShapeT>{}(shape);
    Point3Dx offset = Point3Dx(region_bbox.min_corner()) - bbox.min_corner();
    shape.translate(offset);

    // Reset offsets. We require previous offset to make relative geometric translations
    offset = {.0, .0, .0};
    Point3Dx previous_offset{.0, .0, .0};

    // Calc iteration step. We are doing at most 8 steps in each direction
    // Worst case is user provides a cubic region and shape fits in the very end -> 512 iters
    const Point3Dx diffs = Point3Dx(region_bbox.max_corner()) - region_bbox.min_corner();
    const CoordType base_step = std::max(std::max(diffs.get<0>(), diffs.get<1>()), diffs.get<2>())
                                / 8;
    const int nsteps[] = {int(diffs.get<0>() / base_step),
                          int(diffs.get<1>() / base_step),
                          int(diffs.get<2>() / base_step)};
    const CoordType step[] = {diffs.get<0>() / nsteps[0],
                              diffs.get<1>() / nsteps[1],
                              diffs.get<2>() / nsteps[2]};

    // Loop and Test for each step
    for (int x_i = 0; x_i < nsteps[0]; x_i++) {
        offset.set<1>(0.);

        for (int y_i = 0; y_i < nsteps[1]; y_i++) {
            offset.set<2>(0.);

            for (int z_i = 0; z_i < nsteps[2]; z_i++) {
                shape.translate(offset - previous_offset);
                if (!is_intersecting<BestEffortGeometry>(shape)) {
                    this->insert(shape);
                    return true;
                }
                previous_offset = offset;
                offset.set<2>(offset.get<2>() + step[2]);
            }
            offset.set<1>(offset.get<1>() + step[1]);
        }
        offset.set<0>(offset.get<0>() + step[0]);
    }

    return false;
}


template <typename T, typename A>
inline decltype(auto) IndexTree<T, A>::all_ids() {
    using ids_getter = typename detail::id_getter_for<T>::type;
    std::vector<typename ids_getter::value_type> ids;
    ids.reserve(this->size());
    std::copy(this->begin(), this->end(), ids_getter(ids));
    return ids;
}


// String representation

inline std::ostream& operator<<(std::ostream& os, const ShapeId& obj) {
    return os << obj.id;
}

inline std::ostream& operator<<(std::ostream& os, const MorphPartId& obj) {
    return os << "(" << obj.gid() << ", " << obj.section_id() << ", " << obj.segment_id() << ")";
}

inline std::ostream& operator<<(std::ostream& os, const SubtreeId& obj) {
    return os << "(" << obj.id << ", " << obj.n_elements << ")";
}

template <typename ShapeT, typename IndexT>
inline std::ostream& IndexedShape<ShapeT, IndexT>::repr(
        std::ostream& os, const std::string& cls_name) const {
    return os << cls_name << "("
              "id=" << static_cast<const IndexT&>(*this) << ", "
              << static_cast<const ShapeT&>(*this) << ")";
}


template <typename ShapeT, typename IndexT>
inline std::ostream& operator<<(std::ostream& os,
                                const IndexedShape<ShapeT, IndexT>& obj) {
    return obj.repr(os);
}

inline std::ostream& operator<<(std::ostream& os, const Soma& obj) {
    return obj.repr(os, "Soma");
}

inline std::ostream& operator<<(std::ostream& os, const Segment& obj) {
    return obj.repr(os, "Segment");
}

template <typename T, typename A>
inline std::ostream& operator<<(std::ostream& os, const IndexTree<T, A>& index) {
    int n_obj = 50;   // display the first 50 objects
    os << "IndexTree([\n";
    for (const auto& item : index) {
        if (n_obj-- == 0) {
            os << "  ...\n";
            break;
        }
        os << "  " << item << '\n';
    }
    return os << "])";
}

}  // namespace brain_indexer


// It's fundamental to specialize bgi::indexable for our new Value type
// on how to retrieve the bounding box
// In this case we have a boost::variant of the existing shapes
// where all classes shall implement bounding_box()

namespace boost {
namespace geometry {
namespace index {

using namespace ::brain_indexer;

// Generic
template <typename T>
struct indexable_with_bounding_box {
    typedef T V;
    typedef Box3D const result_type;

    inline result_type operator()(T const& s) const noexcept {
        return s.bounding_box();
    }
};

// Specializations of boost indexable

template<> struct indexable<Sphere> : public indexable_with_bounding_box<Sphere> {};
template<> struct indexable<Cylinder> : public indexable_with_bounding_box<Cylinder> {};
template<> struct indexable<IndexedSphere> : public indexable_with_bounding_box<IndexedSphere> {};
template<> struct indexable<Synapse> : public indexable_with_bounding_box<Synapse> {};
template<> struct indexable<Soma> : public indexable_with_bounding_box<Soma> {};
template<> struct indexable<Segment> : public indexable_with_bounding_box<Segment> {};
template<> struct indexable<IndexedSubtreeBox> : public indexable_with_bounding_box<IndexedSubtreeBox> {};

template <>
struct indexable<IndexedPoint> {
    typedef IndexedPoint V;
    typedef Box3D const result_type;

    inline result_type operator()(V const& s) const noexcept {
        auto p = static_cast<const Point3D&>(s);
        return result_type{p, p};
    }
};


template <typename... VariantArgs>
struct indexable<boost::variant<VariantArgs...>> {
    typedef boost::variant<VariantArgs...> V;
    typedef Box3D const result_type;

    inline result_type operator()(V const& v) const {
        return boost::apply_visitor(
            [](const auto& t) { return t.bounding_box(); },
            v);
    }
};


}  // namespace index
}  // namespace geometry
}  // namespace boost


// Version structures for serialization. We follow a global numbering
namespace boost {
namespace serialization {

template <typename T, typename A>
struct version<brain_indexer::IndexTree<T, A>>
{
    constexpr static unsigned int value = SPATIAL_INDEX_STRUCT_VERSION;
};

template <typename ShapeT, typename IndexT>
struct version<brain_indexer::IndexedShape<ShapeT, IndexT>>
{
    constexpr static unsigned int value = SPATIAL_INDEX_STRUCT_VERSION;
};

}  // namespace serialization
}  // namespace boost


BOOST_CLASS_VERSION(brain_indexer::ShapeId, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::SynapseId, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::MorphPartId, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::Synapse, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::Soma, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::Segment, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::SubtreeId, SPATIAL_INDEX_STRUCT_VERSION);
BOOST_CLASS_VERSION(brain_indexer::IndexedSubtreeBox, SPATIAL_INDEX_STRUCT_VERSION);
