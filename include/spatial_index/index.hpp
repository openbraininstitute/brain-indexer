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


///
/// Result processing iterators
///

/// \brief structure holding composite ids (gid and segment id)
struct gid_segm_t {
    identifier_t gid;
    unsigned segment_i;
};


/// \brief result iterator to run a given callback
template <typename ArgT>
struct iter_callback;

/// \brief result iterator to collect gids
struct iter_ids_getter;

/// \brief result iterator to collect gids and segment ids
struct iter_gid_segm_getter;


/**
 * \brief ShapeId adds an 'id' field to the underlying struct
 */
struct ShapeId {
    identifier_t id;

    using id_getter_t = iter_ids_getter;

    inline bool operator==(const ShapeId& rhs) const noexcept {
        return id == rhs.id;
    }
};


/**
 * \brief A neuron piece extends IndexedShape in concept, adding gid() and segment_i()
 *        accessors where both infos are encoded in the id
 */
struct MorphPartId : public ShapeId {
    using id_getter_t = iter_gid_segm_getter;

    inline MorphPartId() = default;

    inline MorphPartId(identifier_t gid, unsigned segment_i = 0)
        : ShapeId{(gid << 12) + segment_i} {}

    inline MorphPartId(const std::tuple<const identifier_t&, const unsigned&>& ids)
        : MorphPartId(std::get<0>(ids), std::get<1>(ids)) {}

    inline identifier_t gid() const noexcept {
        return id >> 12;
    }

    inline unsigned segment_i() const noexcept {
        return static_cast<unsigned>(id & 0x0fff);
    }
};



template <typename ShapeT, typename IndexT=ShapeId>
struct IndexedShape : public IndexT, public ShapeT {
    typedef ShapeT geometry_type;
    typedef IndexT id_type;

    inline IndexedShape() = default;

    template <typename IdTup>
    inline IndexedShape(IdTup ids, const ShapeT& shape)
        : IndexT{ids}
        , ShapeT{shape} {}

    /** \brief Generic constructor
     * Note: it relies on second argument to be a point so that it doesnt clash with
     * Specific constructors
     */
    template <typename IdTup, typename... T>
    inline IndexedShape(IdTup ids, const Point3D& p1, T&&... shape_data)
        : IndexT{ids}
        , ShapeT{p1, std::forward<T>(shape_data)...} {}

    // subclasses can easily create a string representation
    inline std::ostream& repr(std::ostream& os,
                              const std::string& cls_name="IShape") const;

  protected:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /* version*/) {
        ar& this->id;
        ar& boost::serialization::base_object<ShapeT>(*this);
    }

};



class Soma: public IndexedShape<Sphere, MorphPartId> {
    using super = IndexedShape<Sphere, MorphPartId>;
  public:
    // bring contructors
    using super::IndexedShape;
};



class Segment: public IndexedShape<Cylinder, MorphPartId> {
    using super = IndexedShape<Cylinder, MorphPartId>;
  public:
    // bring contructors
    using super::IndexedShape;

    /**
     * \brief Initialize the Segment directly from ids and gemetric properties
     **/
    inline Segment(identifier_t gid, unsigned segment_i,
                   Point3D const& center1, Point3D const& center2, CoordType const& r)
        noexcept
        : super(std::tie(gid, segment_i), center1, center2, r)
    {}
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
class IndexTree: public bgi::rtree<T, A> {
public:
    using bgi::rtree<T, A>::rtree;

    using cref_t = std::reference_wrapper<const T>;

    inline IndexTree() = default;

    /**
     * \brief Find elements in tree intersecting the given shape.
     *
     * \param iter: An iterator object used to collect matching entries.
     *   Consider using the builtin transformation iterators: iter_ids_getter and
     *   iter_gid_segm_getter. For finer control check the alternate overload
     */
    template <typename ShapeT, typename OutputIt>
    inline void find_intersecting(const ShapeT& shape, const OutputIt& iter) const;

    /**
     * \brief Find elements in tree intersecting the given bounding box.
     * An overload of find_intersecting
     */
    template <typename OutputIt>
    inline void find_intersecting(const Box3D& shape, const OutputIt& iter) const;

    /**
     * \brief Gets the ids of the intersecting objects
     * \returns The object ids, identifier_t or gid_segm_t, depending on the default id getter
     */
    template <typename ShapeT>
    inline decltype(auto) find_intersecting(const ShapeT& shape) const;

    /**
     * \brief Finds & return objects which intersect. To be used mainly with id-less objects
     * \returns A vector of references to tree objects
     */
    template <typename ShapeT>
    inline std::vector<cref_t> find_get_intersecting(const ShapeT& shape) const;

    /**
     * \brief Gets the ids of the the nearest K objects
     * \returns The object ids, identifier_t or gid_segm_t, depending on the default id getter
     */
    template <typename ShapeT>
    inline decltype(auto) find_nearest(const ShapeT& shape, unsigned k_neighbors) const;

    /// \brief Checks whether a given shape intersects any object in the tree
    template <typename ShapeT>
    inline bool is_intersecting(const ShapeT& shape) const;

    /// \brief onstructor to rebuild from binary data file
    // Note: One must override char* and string since there is a template<T> constructor
    inline explicit IndexTree(const std::string& filename);
    inline explicit IndexTree(const char* dump_file)
        : IndexTree(std::string(dump_file)) {}

    /// \brief Output tree to binary data file
    inline void dump(const std::string& filename) const;

    /// \brief Non-overlapping placement of Shapes
    template <typename ShapeT>
    inline bool place(const Box3D& region, ShapeT& shape);

    template <typename ShapeT>
    inline bool place(const Box3D& region, ShapeT&& shape) {
        // Allow user to provide a temporary if they dont care about the new position
        return place(region, shape);
    }

    /// \brief list all ids in the tree
    /// note: this will allocate a full vector. Consider iterating over the tree using
    ///     begin()->end()
    inline decltype(auto) all_ids();

  private:
    typedef bgi::rtree<T, A> super;
};


}  // namespace spatial_index

#include "detail/index.hpp"
