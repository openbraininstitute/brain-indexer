#pragma once

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#error "SpatialIndex requires definition BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL"
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <boost/variant.hpp>


#include "geometries.hpp"


namespace spatial_index {


// Type of the pieces identifiers
using identifier_t = unsigned long;


/**
 * \brief IShape is an Indexed shape.
 *     It Adds an 'id' field to the underlying struct.
 */
template <typename ShapeT>
struct IShape: public ShapeT {
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

    inline bool operator==(type const& o) const noexcept {
        return id == o.id;
    }


  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& id;
        ar&* static_cast<ShapeT*>(this);
    }
};


/**
 * \brief A neuron piece extends IShape in concept, adding gid() and segment_i()
 *        accessors where both infos are encoded in the id
 */
template <typename ShapeT>
struct NeuronPiece: public IShape<ShapeT> {
    using IShape<ShapeT>::IShape;
    typedef NeuronPiece<ShapeT> type;

    inline NeuronPiece(identifier_t gid, unsigned segment_i, ShapeT const& geom_)
        : super{(gid << 12) + segment_i, geom_} {}

    template <typename U>
    inline NeuronPiece(identifier_t gid, unsigned segment_i, U&& geom_)
        : super{(gid << 12) + segment_i, std::forward<U>(geom_)} {}

    inline identifier_t gid() const noexcept {
        return this->id >> 12;
    }

    inline unsigned segment_i() const noexcept {
        return static_cast<unsigned>(this->id & 0x0fff);
    }

  private:
    typedef IShape<ShapeT> super;
};


struct ISoma: public NeuronPiece<Sphere> {
    using NeuronPiece<Sphere>::NeuronPiece;

    // Override to pass segment id 0
    inline ISoma(identifier_t gid, Sphere const& geom_)
        : type{gid, 0, geom_} {}

    template <typename U>
    inline ISoma(identifier_t gid, U&& center, const CoordType& radius)
        : type(gid, 0, Sphere{std::forward<U>(center), radius}) {}
};


struct ISegment: public NeuronPiece<Cylinder> {
    using NeuronPiece<Cylinder>::NeuronPiece;

    // Disable "upstream" ctor with two args
    inline ISegment(identifier_t id_, Cylinder const& geom_) = delete;

    template <typename U>
    inline ISegment(identifier_t gid, U&& geom_) = delete;

    template <typename U>
    inline ISegment(identifier_t gid,
                    unsigned segment_i,
                    U&& center1,
                    U&& center2,
                    const CoordType& radius)
        : type(gid,
               segment_i,
               Cylinder{std::forward<U>(center1), std::forward<U>(center2), radius}) {}
};


//////////////////////////////////////////////
// High Level API
//////////////////////////////////////////////


// User can use Rtree directly with Any of the single Geometries or
// use combined variant<geometries...> or variant<morphologies...>
// the latters include gid() and segment_id() methods.
//
// To simplify typing, GeometryEntry and MorphoEntry are predefined

typedef boost::variant<Sphere, Cylinder> GeometryEntry;
typedef boost::variant<ISoma, ISegment> MorphoEntry;


///
/// IndexTree is a Boost::rtree spatial index tree
/// It adds methods for finding intersections and serialization.
///
template <typename T, typename A = bgi::linear<16, 2>>
struct IndexTree: public bgi::rtree<T, A> {
    using bgi::rtree<T, A>::rtree;

    template <typename Shap>
    inline std::vector<const T*> find_intersecting(const Shap& shape) const;

    template <typename Shap>
    inline bool is_intersecting(const Shap& shape) const;

    // Load / Dump serialization
    inline explicit IndexTree(const std::string& filename);
    inline explicit IndexTree(const char* dump_file)
        : IndexTree(std::string(dump_file)) {}

    inline void dump(const std::string& filename) const;

    /// Non-overlapping placement of Shapes
    template <typename S>
    inline bool place(const Box3D& region, S& shape);

  private:
    typedef bgi::rtree<T, A> super;
};


///
/// Result processing iterators
///

/// \brief result iterator to run a given callback
template <typename ArgT>
struct iter_callback;

/// \brief result iterator to collect gids
struct iter_ids_getter;

/// \brief result iterator to collect gids and segment ids
struct iter_gid_segm_getter;


}  // namespace spatial_index

#include "detail/index.hpp"
