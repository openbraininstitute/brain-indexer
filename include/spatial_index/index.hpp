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



// It's fundamental to specialize indexable for our new Value type
// on how to retrieve the bounding box
// In this case we have a boost::variant of the existing shapes
// where all classes shall implement bounding_box()

namespace boost { namespace geometry { namespace index {


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


// Type of the pieces identifiers
using identifier_t = unsigned long;


/**
 * \brief
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
    inline ISoma(identifier_t gid, unsigned segment_i, U&& geom_)
        : type{gid, 0, std::forward<U>(geom_)} {}


};


struct ISegment : public NeuronPiece<Cylinder>
{
    using NeuronPiece<Cylinder>::NeuronPiece;

    // Disable "upstream" ctor with two args
    inline ISegment(identifier_t id_, Cylinder const& geom_) = delete;

    template <typename U>
    inline ISegment(identifier_t gid, U&& geom_) = delete;


};




typedef boost::variant<ISoma, ISegment> IndexEntry;
typedef bgi::rtree< IndexEntry, bgi::linear<16, 2> > IndexTree;


template<typename... T>
void index_dump(bgi::rtree<T...> rtree, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    boost::archive::binary_oarchive oa(ofs);
    oa << rtree;
}


IndexTree index_load(const std::string& filename) {
    IndexTree loaded_tree;
    std::ifstream ifs(filename, std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> loaded_tree;
    return loaded_tree;
}

}  // namespace spatial_index


