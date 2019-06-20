#pragma once

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>
#else
#error("SpatialIndex requires BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")
#endif

#include <boost/geometry/index/rtree.hpp>
#include <boost/variant.hpp>

#include <fstream>
#include <iostream>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


#include "geometries.hpp"


namespace spatial_index {


// Type of the pieces identifiers
using identifier_t = unsigned long;


/**
 * \brief IShape is an Indexed shape.
 *     It Adds an 'id' field to the underlying struct.
 */
template <typename ShapeT>
struct IShape : public ShapeT
{
    typedef IShape<ShapeT> type;

    identifier_t id;

    inline IShape() = default;
    inline IShape(type const&) = default;
    inline IShape(type&&) = default;
    inline type& operator=(type const&) = default;
    inline type& operator=(type&&) = default;


    // Deduct / Convert matching types
    inline IShape(identifier_t id_, ShapeT const& geom_)
        : ShapeT{geom_}
        , id{id_} {}

    template <typename U>
    inline IShape(identifier_t id_, U&& geom_)
        : ShapeT{std::forward<U>(geom_)}
        , id{id_} {}

    inline bool operator==(type const& o) const {
        return id == o.id;
    }


private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & id;
        ar & *static_cast<ShapeT*>(this);
    }

};


template <typename ShapeT>
struct NeuronPiece : public IShape<ShapeT>
{
    using IShape<ShapeT>::IShape;
    typedef NeuronPiece<ShapeT> type;

    inline NeuronPiece(identifier_t gid, unsigned segment_i, ShapeT const& geom_)
        : super{(gid<<12) + segment_i, geom_} {}

    template <typename U>
    inline NeuronPiece(identifier_t gid, unsigned segment_i, U&& geom_)
        : super{(gid<<12) + segment_i, std::forward<U>(geom_)} {}

    inline identifier_t gid() const {
        return this->id>>12;
    }

    inline unsigned segment_i() const {
        return static_cast<unsigned>(this->id & 0x0fff);
    }

private:
    typedef IShape<ShapeT> super;
};



struct ISoma : public NeuronPiece<Sphere>
{
    using NeuronPiece<Sphere>::NeuronPiece;

    // Override to pass segment id 0
    inline ISoma(identifier_t gid, Sphere const& geom_)
        : type{gid, 0, geom_} {}

    template <typename U>
    inline ISoma(identifier_t gid, U&& center, const CoordType& radius)
        : type(gid, 0, Sphere{std::forward<U>(center), radius}) {}


};


struct ISegment : public NeuronPiece<Cylinder>
{
    using NeuronPiece<Cylinder>::NeuronPiece;

    // Disable "upstream" ctor with two args
    inline ISegment(identifier_t id_, Cylinder const& geom_) = delete;

    template <typename U>
    inline ISegment(identifier_t gid, U&& geom_) = delete;


};


// User can use Rtree directly with Geometries or use morphology
// pieces, the main difference being that MorphoEntry(s) include
// identifiers for the gid (and segment id for segments)

typedef boost::variant<Sphere, Cylinder> GeometryEntry;
typedef boost::variant<ISoma,  ISegment> MorphoEntry;

template <typename T>
using IndexTree = bgi::rtree<T, bgi::linear<16, 2> >;


} //namespace spatial_index


// It's fundamental to specialize indexable for our new Value type
// on how to retrieve the bounding box
// In this case we have a boost::variant of the existing shapes
// where all classes shall implement bounding_box()

namespace boost { namespace geometry { namespace index {

using namespace ::spatial_index;

// Generic
template<typename T>
struct indexable_with_bounding_box {
    typedef T V;
    typedef Box3D const result_type;

    inline result_type operator()(T const& s) const {
        return s.bounding_box();
    }
};

// // Specializations of boost indexable

template<> struct indexable<Sphere> :   public indexable_with_bounding_box<Sphere> {};
template<> struct indexable<Cylinder> : public indexable_with_bounding_box<Cylinder> {};
template<> struct indexable<ISoma> :    public indexable_with_bounding_box<ISoma> {};
template<> struct indexable<ISegment> : public indexable_with_bounding_box<ISegment> {};


template <typename... VariantArgs>
struct indexable< boost::variant<VariantArgs...> >
{
    typedef boost::variant<VariantArgs...> V;

    typedef spatial_index::Box3D const result_type;

    struct BoundingBoxVisitor : boost::static_visitor<result_type> {
        template<class T>
        inline result_type operator()(const T& t)const { return t.bounding_box(); }
    };

    inline result_type operator()(V const& v) const {
        return boost::apply_visitor(bbox_visitor_, v);
    }

private:
    BoundingBoxVisitor bbox_visitor_;
};


}}} // namespace boost::geometry::index


namespace spatial_index {


//////////////////////////////////////////////
// High Level API
//////////////////////////////////////////////

struct result_ids_getter {
    result_ids_getter(std::vector<identifier_t> & output)
        : output_(output) {}

    // Works with whatever has an 'id' field
    template<typename T>
    inline result_ids_getter& operator=(const IShape<T>& result_entry) {
        output_.push_back(result_entry.id);
        return *this;
    }

    // Specialization for variants
    struct GetIdVisitor : boost::static_visitor<identifier_t> {
        template<class T>
        inline identifier_t operator()(const T& t) const { return t.id; }
    };

    template<typename... ManyT>
    inline result_ids_getter& operator=(const boost::variant<ManyT...>& v) {
        static GetIdVisitor id_visitor;
        output_.push_back(boost::apply_visitor(id_visitor, v));
        return *this;
    }

    inline result_ids_getter& operator*()  { return *this; }
    inline result_ids_getter& operator++() { return *this; }
    inline result_ids_getter& operator--(int) { return *this; }

private:
    // Keep a ref to modify the original vec
    std::vector<identifier_t> & output_;
};


struct gid_segm_t {
    identifier_t gid;
    unsigned segment_i;
};


struct result_gid_segm_getter {
    result_gid_segm_getter(std::vector<gid_segm_t> & output)
        : output_(output) {}

    // Works with whatever has gi() and segment_i()
    template<typename T>
    inline result_gid_segm_getter& operator=(const NeuronPiece<T>& result_entry) {
        output_.emplace_back(result_entry.gid(), result_entry.segment_i());
        return *this;
    }

    // Specialization for variants
    struct GetIdVisitor : boost::static_visitor<gid_segm_t> {
        template<class T>
        inline gid_segm_t operator()(const T& t) const {
            return {t.gid(), t.segment_i()};
        }
    };

    template<typename... ManyT>
    inline result_gid_segm_getter& operator=(const boost::variant<ManyT...>& v) {
        static GetIdVisitor id_visitor;
        output_.emplace_back(boost::apply_visitor(id_visitor, v));
        return *this;
    }

    inline result_gid_segm_getter& operator* ()    { return *this; }
    inline result_gid_segm_getter& operator++()    { return *this; }
    inline result_gid_segm_getter& operator--(int) { return *this; }

private:
    std::vector<gid_segm_t> & output_;
};



template<typename... T>
inline void index_dump(bgi::rtree<T...> rtree, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    boost::archive::binary_oarchive oa(ofs);
    oa << rtree;
}

template<typename T>
inline IndexTree<T> index_load(const std::string& filename) {
    IndexTree<T> loaded_tree;
    std::ifstream ifs(filename, std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> loaded_tree;
    return loaded_tree;
}

}  // namespace spatial_index


