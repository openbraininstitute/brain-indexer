#pragma once

#include "../index.hpp"

#include <fstream>
#include <iostream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "output_iterators.hpp"

namespace spatial_index {


// Specialization of geometry_intersects for variant geometries

template <typename T, typename... VarT>
bool geometry_intersects(const boost::variant<VarT...>& geom1, const T& geom2) {
    return boost::apply_visitor(
        [&geom2](const auto& g1) {
            // In case T is another variant, can be matched
            return geometry_intersects(g1, geom2);
        },
        geom1);
}

template <typename T, typename... VarT>
bool geometry_intersects(const T& geom1, const boost::variant<VarT...>& geom2) {
    return geometry_intersects(geom2, geom1);
}


/////////////////////////////////////////
// class IndexTree
/////////////////////////////////////////

template <typename T, typename A>
template <typename ShapeT, typename OutputIt>
inline void IndexTree<T, A>::find_intersecting(const ShapeT& shape, const OutputIt& iter) const {
    // Using a callback makes the query slightly faster than using qbegin()...qend()
    auto real_intersection = [&shape](const T& v){ return geometry_intersects(shape, v); };
    this->query(bgi::intersects(bgi::indexable<ShapeT>{}(shape))
                    && bgi::satisfies(real_intersection),
                iter);
}

template <typename T, typename A>
template <typename OutputIt>
inline void IndexTree<T, A>::find_intersecting(const Box3D& shape, const OutputIt& iter) const {
    this->query(bgi::intersects(shape), iter);
}


template <typename T, typename A>
template <typename ShapeT>
inline decltype(auto) IndexTree<T, A>::find_intersecting(const ShapeT& shape) const {
    using ids_getter = typename detail::id_getter_for<T>::type;
    std::vector<typename ids_getter::value_type> ids;
    find_intersecting(shape, ids_getter(ids));
    return ids;
}

template <typename T, typename A>
template <typename ShapeT>
inline decltype(auto) IndexTree<T, A>::find_intersecting_pos(const ShapeT& shape) const {
    std::vector<Point3D> points;
    find_intersecting(shape, iter_pos_getter(points));
    return points;
}


template <typename T, typename A>
template <typename ShapeT>
inline std::vector<typename IndexTree<T, A>::cref_t>
IndexTree<T, A>::find_intersecting_objs(const ShapeT& shape) const {
    std::vector<cref_t> results;
    find_intersecting(shape, std::back_inserter(results));
    return results;
}


template <typename T, typename A>
template <typename ShapeT>
inline bool IndexTree<T, A>::is_intersecting(const ShapeT& shape) const {
    for (auto it = this->qbegin(bgi::intersects(bgi::indexable<ShapeT>{}(shape)));
            it != this->qend();
            ++it) {
        if (geometry_intersects(shape, *it)) {
            return true;
        }
    }
    return false;
}


template <typename T, typename A>
template <typename ShapeT>
inline decltype(auto)
IndexTree<T, A>::find_nearest(const ShapeT& shape, unsigned k_neighbors) const {
    using ids_getter = typename detail::id_getter_for<T>::type;
    std::vector<typename ids_getter::value_type> ids;
    this->query(bgi::nearest(shape, k_neighbors), ids_getter(ids));
    return ids;
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
                if (!is_intersecting(shape)) {
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


// Serialization: Load ctor
template <typename T, typename A>
inline IndexTree<T, A>::IndexTree(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> static_cast<super&>(*this);
}

template <typename T, typename A>
inline void IndexTree<T, A>::dump(const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    boost::archive::binary_oarchive oa(ofs);
    oa << static_cast<const super&>(*this);
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
    return os << '(' << obj.gid() << ", " << obj.section_id() << ", " << obj.segment_id() << ')';
}

template <typename ShapeT, typename IndexT>
inline std::ostream& IndexedShape<ShapeT, IndexT>::repr(
        std::ostream& os, const std::string& cls_name) const {
    return os << cls_name << "("
              "id=" << static_cast<const IndexT&>(*this) << ", "
              << static_cast<const ShapeT&>(*this) << ')';
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


}  // namespace spatial_index


// It's fundamental to specialize bgi::indexable for our new Value type
// on how to retrieve the bounding box
// In this case we have a boost::variant of the existing shapes
// where all classes shall implement bounding_box()

namespace boost {
namespace geometry {
namespace index {

using namespace ::spatial_index;

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
template<> struct indexable<Soma> : public indexable_with_bounding_box<Soma> {};
template<> struct indexable<Segment> : public indexable_with_bounding_box<Segment> {};

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
