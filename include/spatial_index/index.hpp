#pragma once

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#error "SpatialIndex requires definition BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL"
#endif
#include <cstdio>
#include <functional>
#include <iostream>
#include <unordered_map>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>

#include <boost/interprocess/managed_mapped_file.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <boost/variant.hpp>


#include "geometries.hpp"
#define N_SEGMENT_BITS 10
#define N_SECTION_BITS 14
#define N_TOTAL_BITS (N_SEGMENT_BITS + N_SECTION_BITS)
#define MASK_SEGMENT_BITS ((1 << N_SEGMENT_BITS)-1)
#define MASK_SECTION_BITS (((1 << N_SECTION_BITS)-1) << N_SEGMENT_BITS)
#define MASK_TOTAL_BITS ((1 << N_TOTAL_BITS)-1)

namespace spatial_index {


// Type of the pieces identifiers
using identifier_t = unsigned long;


///
/// Result processing iterators
///

/// \brief structure holding composite ids (gid, section id and segment id)
struct gid_segm_t {
    identifier_t gid;
    unsigned section_id;
    unsigned segment_id;
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
 * \brief A synapse extends IndexedShape in concept, adding gid field for ease of aggregating
 */
struct SynapseId : public ShapeId {
    identifier_t post_gid_;
    identifier_t pre_gid_;

    inline SynapseId() = default;

    inline SynapseId(const identifier_t& syn_id, const identifier_t& post_gid = 0, const identifier_t pre_gid = 0) noexcept
        : ShapeId{syn_id}
        , post_gid_(post_gid)
        , pre_gid_(pre_gid)
    {}

    inline SynapseId(std::tuple<const identifier_t&, const identifier_t&, const identifier_t&> ids) noexcept
        : ShapeId{std::get<0>(ids)}
        , post_gid_(std::get<1>(ids))
        , pre_gid_(std::get<2>(ids))
    {}

    inline identifier_t post_gid() const noexcept {
        return post_gid_;
    }

    inline identifier_t pre_gid() const noexcept {
        return pre_gid_;
    }
};


/**
 * \brief A neuron piece extends IndexedShape in concept, adding gid(), section_id and segment_id()
 *        accessors where both infos are encoded in the id
 */
struct MorphPartId : public ShapeId {
    using id_getter_t = iter_gid_segm_getter;

    inline MorphPartId() = default;

    inline MorphPartId(identifier_t gid, unsigned section_id = 0, unsigned segment_id = 0)
        : ShapeId{((gid << N_TOTAL_BITS) & (~MASK_TOTAL_BITS))
                  + ((section_id << N_SEGMENT_BITS) & MASK_SECTION_BITS)
                  + (segment_id & MASK_SEGMENT_BITS)}
    {}

    inline MorphPartId(const std::tuple<const identifier_t&, const unsigned&, const unsigned&>& ids)
        : MorphPartId(std::get<0>(ids), std::get<1>(ids), std::get<2>(ids))
    {}

    inline identifier_t gid() const noexcept {
        return id >> N_TOTAL_BITS;
    }

    inline unsigned segment_id() const noexcept {
        return static_cast<unsigned>(id & MASK_SEGMENT_BITS);
    }

    inline unsigned section_id() const noexcept {
        return static_cast<unsigned>((id & MASK_SECTION_BITS) >> N_SEGMENT_BITS);
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


class Synapse : public IndexedShape<Sphere, SynapseId> {
    using super = IndexedShape<Sphere, SynapseId>;

  public:
    // bring contructors
    using super::IndexedShape;

    inline Synapse(identifier_t id, identifier_t post_gid, identifier_t pre_gid, Point3D const& point) noexcept
        : super(std::tie(id, post_gid, pre_gid), point, .0f)
    {}
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
    inline Segment(identifier_t gid,
                   unsigned section_id,
                   unsigned segment_id,
                   Point3D const& center1,
                   Point3D const& center2,
                   CoordType const& r) noexcept
        : super(std::tie(gid, section_id, segment_id), center1, center2, r)
    {}
};


//////////////////////////////////////////////
// High Level API
//////////////////////////////////////////////


// User can use Rtree directly with Any of the single Geometries or
// use combined variant<geometries...> or variant<morphologies...>
// the latters include gid(), section_id() and segment_id() methods.

// To simplify typing, GeometryEntry and MorphoEntry are predefined
typedef IndexedShape<Sphere> IndexedSphere;
typedef boost::variant<Sphere, Cylinder> GeometryEntry;
typedef boost::variant<Soma, Segment> MorphoEntry;


/// A shorthand for a default IndexTree with potentially custom allocator
template <typename T, typename A = boost::container::new_allocator<T>>
using IndexTreeBaseT = bgi::rtree<T, bgi::linear<16, 2>, bgi::indexable<T>, bgi::equal_to<T>, A>;


/**
 * \brief IndexTree is a Boost::rtree spatial index tree with helper methods
 *    for finding intersections and serialization.
 *
 * \note: For large arrays of raw data (vec[floats]...) consider using make_soa_reader to
 *       avoid duplicating all the data in memory. Init using IndexTree(soa.begin(), soa.end())
 */
template <typename T, typename A = boost::container::new_allocator<T>>
class IndexTree: public IndexTreeBaseT<T, A> {
    using super = IndexTreeBaseT<T, A>;

  public:
    using cref_t = std::reference_wrapper<const T>;

    using super::rtree::rtree;  // super ctors

    inline IndexTree() = default;

    /**
     * \brief Constructs an IndexTree using a custom allocator.
     *
     * \param alloc The allocator to be used in this instance.
     *  Particularly useful for super large indices using memory-mapped files
     */
    // Note: We need the following template here to create an universal reference
    template <typename Alloc = A, std::enable_if_t<std::is_same<Alloc, A>::value, int> = 0>
    IndexTree(Alloc&& alloc)
        : super::rtree(bgi::linear<16, 2>(),
                       bgi::indexable<T>(),
                       bgi::equal_to<T>(),
                       std::forward<Alloc>(alloc)) { }

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
     * \brief Gets the pos of the intersecting objects
     * \returns The object pos, depending on the default pos getter
     */
    template <typename ShapeT>
    inline decltype(auto) find_intersecting_pos(const ShapeT& shape) const;

    /**
     * \brief Finds & return objects which intersect. To be used mainly with id-less objects
     * \returns A vector of references to tree objects
     */
    template <typename ShapeT>
    inline std::vector<cref_t> find_intersecting_objs(const ShapeT& shape) const;

    /**
     * \brief Gets the ids of the the nearest K objects
     * \returns The object ids, identifier_t or gid_segm_t, depending on the default id getter
     */
    template <typename ShapeT>
    inline decltype(auto) find_nearest(const ShapeT& shape, unsigned k_neighbors) const;

    /// \brief Checks whether a given shape intersects any object in the tree
    template <typename ShapeT>
    inline bool is_intersecting(const ShapeT& shape) const;

    /// \brief Counts objects intersecting the given region deliminted by the shape
    template <typename ShapeT>
    inline size_t count_intersecting(const ShapeT& shape) const;

    /// \brief Counts objects intersecting the given region deliminted by the shape
    template <typename ShapeT>
    inline std::unordered_map<identifier_t, size_t>
        count_intersecting_agg_gid(const ShapeT& shape) const;

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
};


// Using Memory-Mapped-File for X-Large indices
// ============================================

namespace bip = boost::interprocess;

template <typename T>
using MemDiskAllocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;

template <typename T>
class IndexTreeMemDisk: public IndexTree<T, MemDiskAllocator<T>> {

  public:
    using super = IndexTree<T, MemDiskAllocator<T>>;
    using super::IndexTree::IndexTree;

    /// Enable move ctor and assignment operator. Copy not allowed.
    IndexTreeMemDisk(IndexTreeMemDisk&&) = default;
    IndexTreeMemDisk& operator=(IndexTreeMemDisk&&) = default;

    /// \brief The factory for IndexTreeMemDisk objects.
    ///
    /// IndexTreeMemDisk are special objects which hold the managed_mapped_file
    ///    used as memory for its rtree superclass. Therefore we must initialize
    ///    in advance.
    /// \param fname The filename to store the rtree memory
    /// \param size_mb The initial capacity, in MegaBytes
    /// \param truncate If true will delete and create a new mapped file
    /// \param close_shrink If true will shrink the mem file to contents
    inline static IndexTreeMemDisk open_or_create(const std::string& filename,
                                                  size_t size_mb = 1024,
                                                  bool truncate = false,
                                                  bool close_shrink = false);

    /// \brief Opens an rtree object from a managed-mapped-file for Reading
    /// \Note: Avoid modifying existing objects since they might not have free space left
    inline static IndexTreeMemDisk open(const std::string& filename);

    /// \brief Flush and close the current object.
    /// \note The object is not usable after this function
    inline void close();

    ~IndexTreeMemDisk() {
        close();  // Ensure object is sync'ed back to mem-file
    }

  protected:
    inline IndexTreeMemDisk(const std::string& fname,
                            std::unique_ptr<bip::managed_mapped_file>&& mapped_file,
                            bool close_shrink = false);

    std::string filename_;
    std::unique_ptr<bip::managed_mapped_file> mapped_file_;  // Keep in heap so it wont move
    bool close_shrink_;
};


}  // namespace spatial_index

#include "detail/index.hpp"
#include "detail/index_memdisk.hpp"
