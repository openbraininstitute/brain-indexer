#pragma once

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#error "SpatialIndex requires definition BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL"
#endif
#include <functional>

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
 * \brief IndexedShape adds an 'id' field to the underlying struct
 */
template <typename ShapeT>
struct IndexedShape: public ShapeT {
    typedef IndexedShape<ShapeT> type;

    identifier_t id;

    inline IndexedShape() = default;

    // Deduct / Convert matching types
    inline IndexedShape(identifier_t id_, ShapeT const& geom_) noexcept
        : ShapeT{geom_}
        , id{id_} {}

    template <typename... U>
    inline IndexedShape(identifier_t id_, U&&... geom_) noexcept
        : ShapeT{std::forward<U>(geom_)...}
        , id{id_} {}

    inline bool operator==(type const& o) const noexcept {
        return id == o.id;
    }

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int/* version*/) {
        ar& id;
        ar&* static_cast<ShapeT*>(this);
    }
};


/**
 * \brief A neuron piece extends IndexedShape in concept, adding gid() and segment_i()
 *        accessors where both infos are encoded in the id
 */
template <typename ShapeT>
struct NeuronPiece: public IndexedShape<ShapeT> {
    typedef NeuronPiece<ShapeT> type;

    inline NeuronPiece() = default;

    inline NeuronPiece(identifier_t gid, unsigned segment_i, ShapeT const& geom_) noexcept
        : super{pack_ids(gid, segment_i), geom_} {}

    inline identifier_t gid() const noexcept {
        return this->id >> 12;
    }

    inline unsigned segment_i() const noexcept {
        return static_cast<unsigned>(this->id & 0x0fff);
    }

  protected:
    inline static constexpr identifier_t pack_ids(identifier_t gid, unsigned segment_i=0) {
        return (gid << 12) + segment_i;
    }

  private:
    typedef IndexedShape<ShapeT> super;
};


struct Soma: public NeuronPiece<Sphere> {
    using NeuronPiece<Sphere>::NeuronPiece;

    inline Soma() = default;

    inline Soma(identifier_t gid, Sphere const& geom_) noexcept
        : type{gid, 0u, geom_} {}

    /**
      * \brief Initialize the Soma directly from ids and gemetric properties
      *  Note: This was changed from a more encapsulated version since initializing
      *    using super constructors ends up in 3 chained function calls (all way up)
      */
    template <typename U>
    inline Soma(identifier_t gid, U&& center, CoordType const& r) noexcept {
        id = pack_ids(gid);
        centroid = center;
        radius = r;
    }

    template <typename... T>
    inline Soma(std::tuple<T...> const& param) noexcept
        : Soma(std::get<0>(param), std::get<1>(param), std::get<2>(param)) {
        static_assert(sizeof...(T) == 3, "Wrong parameter tuple size");
    }
};


struct Segment: public NeuronPiece<Cylinder> {
    using NeuronPiece<Cylinder>::NeuronPiece;

    template <typename U>
    inline Segment(identifier_t gid, unsigned segment_i,
                   U&& center1, U&& center2,
                   CoordType const& r) noexcept {
        id = pack_ids(gid, segment_i);
        p1 = center1;
        p2 = center2;
        radius = r;
    }

    template <typename... T>
    inline Segment(std::tuple<T...> const& param) noexcept
        : Segment(std::get<0>(param), std::get<1>(param), std::get<2>(param), std::get<3>(param)) {
        static_assert(sizeof...(T) == 4, "Wrong parameter tuple size");
    }
};


//////////////////////////////////////////////
// High Level API
//////////////////////////////////////////////


// User can use Rtree directly with Any of the single Geometries or
// use combined variant<geometries...> or variant<morphologies...>
// the latters include gid() and segment_id() methods.

// To simplify typing, GeometryEntry and MorphoEntry are predefined
typedef IndexedShape<Sphere> IndexedSphere;
typedef boost::variant<Sphere, Cylinder> GeometryEntry;
typedef boost::variant<Soma, Segment> MorphoEntry;


/**
 * \brief IndexTree is a Boost::rtree spatial index tree with helper methods
 *    for finding intersections and serialization.
 *
 * \note: For large arrays of raw data (vec[floats]...) consider using make_soa_reader to
 *       avoid duplicating all the data in memory. Init using IndexTree(soa.begin(), soa.end())
 */
template <typename T, typename A = bgi::linear<16, 2>>
struct IndexTree: public bgi::rtree<T, A> {
    using bgi::rtree<T, A>::rtree;

    using cref_t = std::reference_wrapper<const T>;

    /**
     * \brief Find elements in tree intersecting the given shape.
     *
     * \param iter: An iterator object used to collect matching entries.
     *   Consider using the builtin transformation iterators: iter_ids_getter and
     *   iter_gid_segm_getter. For finer control check the alternate overload
     */
    template <typename ShapeT, typename OutputIt>
    inline void find_intersecting(const ShapeT& shape, const OutputIt& iter) const;

    template <typename ShapeT>
    inline std::vector<cref_t> find_intersecting(const ShapeT& shape) const;

    /// Checks whether a given shape intersects any object in the tree
    template <typename ShapeT>
    inline bool is_intersecting(const ShapeT& shape) const;

    /// Constructor to rebuild from binary data file
    inline explicit IndexTree(const std::string& filename);
    inline explicit IndexTree(const char* dump_file)
        : IndexTree(std::string(dump_file)) {}

    /// Output tree to binary data file
    inline void dump(const std::string& filename) const;

    /// Non-overlapping placement of Shapes
    template <typename ShapeT>
    inline bool place(const Box3D& region, ShapeT& shape);

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
