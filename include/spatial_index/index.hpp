#pragma once

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#error "SpatialIndex requires definition BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL"
#endif
#include <cstdio>
#include <functional>
#include <iostream>
#include <unordered_map>

// boost::serialize before boost::geometry
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/version.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <boost/variant.hpp>

#include <spatial_index/meta_data.hpp>
#include <spatial_index/geometries.hpp>
#include <spatial_index/util.hpp>
#include <spatial_index/logging.hpp>
#include <spatial_index/version.hpp>

namespace spatial_index {

// Type of the pieces identifiers
using identifier_t = unsigned long;


// Constants for packing identifiers.
static constexpr int N_SEGMENT_BITS = 14;
static constexpr int N_SECTION_BITS = 14;
static constexpr int N_TOTAL_BITS = N_SEGMENT_BITS + N_SECTION_BITS;
static constexpr int N_GID_BITS = 64 - N_TOTAL_BITS;

template<class Int=identifier_t>
inline constexpr Int mask_bits(int n_bits) {
  return ((Int(1) << n_bits) - 1);
}

static constexpr identifier_t MASK_SEGMENT_BITS = mask_bits(N_SEGMENT_BITS);
static constexpr identifier_t MASK_SECTION_BITS = mask_bits(N_SECTION_BITS) << N_SEGMENT_BITS;


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

/// \brief result iterator to collect gids, segment ids, section ids and centroids
template<typename Element>
struct iter_entry_getter;

/**
 * \brief ShapeId adds an 'id' field to the underlying struct
 */
struct ShapeId {
    identifier_t id;

    using id_getter_t = iter_ids_getter;

    inline bool operator==(const ShapeId& rhs) const noexcept {
        return id == rhs.id;
    }

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for ShapeId."); }

        ar & this->id;
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

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for SynapseId."); }

        ar & boost::serialization::base_object<ShapeId>(*this);
        ar & post_gid_;
        ar & pre_gid_;
    }
};


inline bool is_gid_safe(identifier_t gid) {
    return (gid & ~mask_bits(N_GID_BITS)) == 0;
}

inline bool is_section_id_safe(unsigned section_id) {
    return (section_id & ~mask_bits<unsigned>(N_SECTION_BITS)) == 0;
}

inline bool is_segment_id_safe(unsigned segment_id) {
    return (segment_id & ~mask_bits<unsigned>(N_SEGMENT_BITS)) == 0;
}


/**
 * \brief A neuron piece extends IndexedShape in concept, adding gid(), section_id and segment_id()
 *        accessors where both infos are encoded in the id
 */
struct MorphPartId : public ShapeId {
    using id_getter_t = iter_gid_segm_getter;

    inline MorphPartId() = default;

    inline MorphPartId(identifier_t gid, unsigned section_id = 0, unsigned segment_id = 0)
        : ShapeId{(gid << N_TOTAL_BITS) + (section_id << N_SEGMENT_BITS) + segment_id}
    {
        if (!(is_gid_safe(gid) && is_section_id_safe(section_id) && is_segment_id_safe(segment_id))) {

            log_error("One of the IDs is too large to be encoded in the current data structure!");

            if(!is_gid_safe(gid)) {
                throw std::runtime_error("Invalid gid.");
            } else if (!is_section_id_safe(section_id)) {
                throw std::runtime_error("Invalid section_id.");
            } else {
                throw std::runtime_error("Invalid segment_id.");
            }
        }
    }

    inline MorphPartId(const std::tuple<const identifier_t&, const unsigned&, const unsigned&>& ids)
        : MorphPartId(std::get<0>(ids), std::get<1>(ids), std::get<2>(ids))
    {}

    inline identifier_t gid() const noexcept {
        return id >> N_TOTAL_BITS;
    }

    inline unsigned segment_id() const noexcept {
        return util::integer_cast<unsigned>(id & MASK_SEGMENT_BITS);
    }

    inline unsigned section_id() const noexcept {
        return util::integer_cast<unsigned>((id & MASK_SECTION_BITS) >> N_SEGMENT_BITS);
    }

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for MorphPartId."); }

        ar & boost::serialization::base_object<ShapeId>(*this);
    }
};


template <typename ShapeT, typename IndexT=ShapeId>
struct IndexedShape : public IndexT, public ShapeT {
    typedef ShapeT geometry_type;
    typedef IndexT id_type;

    inline IndexedShape() = default;
    IndexedShape(const IndexedShape&) = default;

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

    IndexedShape& operator=(const IndexedShape&) = default;

    // subclasses can easily create a string representation
    inline std::ostream& repr(std::ostream& os,
                              const std::string& cls_name="IShape") const;

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for IndexedShape."); }

        ar & boost::serialization::base_object<IndexT>(*this);
        ar & boost::serialization::base_object<ShapeT>(*this);
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

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for Synapse."); }

        ar & boost::serialization::base_object<super>(*this);
    }
};



class Soma: public IndexedShape<Sphere, MorphPartId> {
    using super = IndexedShape<Sphere, MorphPartId>;

  public:
    // bring contructors
    using super::IndexedShape;

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for Soma."); }

        ar & boost::serialization::base_object<super>(*this);
    }
};

/**
 * \brief enum to describe the type of segment
 */
enum class SectionType : unsigned char {
    undefined = 0,
    soma = 1,
    axon = 2,
    basal_dendrite = 3,
    apical_dendrite = 4
};

class Segment: public IndexedShape<Cylinder, MorphPartId> {
    using super = IndexedShape<Cylinder, MorphPartId>;

  public:
    // bring contructors
    using super::IndexedShape;

    /**
     * \brief Initialize the Segment directly from ids and geometric properties
     **/
    inline Segment(identifier_t gid,
                   unsigned section_id,
                   unsigned segment_id,
                   Point3D const& center1,
                   Point3D const& center2,
                   CoordType const& r,
                   SectionType const& section_type = SectionType::undefined) noexcept
        : super(std::tie(gid, section_id, segment_id), center1, center2, r), section_type_(section_type)
    {}

    inline SectionType section_type() const noexcept {
        return section_type_;
    };

protected:
    // section type
    SectionType section_type_;

private:
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int /*version*/) const
    {
        ar << boost::serialization::base_object<super>(*this);
        ar << section_type_;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for Segment."); }

        ar >> boost::serialization::base_object<super>(*this);

        if (version <= 4) {
            section_type_ = SectionType::undefined;
        }
        else {
            ar >> section_type_;
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};



class SubtreeId {
  public:
    size_t id;
    size_t n_elements;

    SubtreeId() : id(0), n_elements(0) {}

    SubtreeId(const SubtreeId&) = default;

    SubtreeId(size_t id, size_t n_elements)
    : id(id), n_elements(n_elements) {}

    SubtreeId& operator=(const SubtreeId&) = default;

    inline bool operator==(const ShapeId& rhs) const noexcept {
        return id == rhs.id;
    }

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for SubtreeId."); }

        ar & id;
        ar & n_elements;
    }

};


class IndexedSubtreeBox : public IndexedShape<Box3Dx, SubtreeId> {
    using super = IndexedShape<Box3Dx, SubtreeId>;

  public:
    using super::IndexedShape;

    inline IndexedSubtreeBox(size_t id, size_t n_elements, Box3D const& box)
        : super(SubtreeId(id, n_elements), Box3Dx(box)) {}

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version == 0) { throw std::runtime_error("Invalid version 0 for IndexedSubtreeBox."); }

        ar & boost::serialization::base_object<super>(*this);
    }
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


template <typename Derived, typename T>
class IndexTreeMixin {
  public:
    /**
     * \brief Find elements in tree that intersect with the given shape.
     *
     * The query shape is always treated as the exact shape. The indexed elements
     * on the other hand can either be selected if their bounding box intersects with
     * the query shape; or if the 'exact' shape intersects with the query shape. Note,
     * that exact isn't true for cylinders which are often treated as capsules instead.
     *
     * \tparam GeometryMode: Selects between best-effort (`BestEffortGeometry`) and bounding
     *   box geometry (`BoundingBoxGeometry`).
     *
     * \param iter: An iterator object used to collect matching entries.
     *   Consider using the builtin transformation iterators: iter_ids_getter and
     *   iter_gid_segm_getter. For finer control check the alternate overload
     */
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT, typename OutputIt>
    inline void find_intersecting(const ShapeT& shape, const OutputIt& iter) const;

    /**
     * \brief Finds & return objects which intersect, numpy version.
     * \returns A vector of POD objects, to be exposed as numpy arrays(dtype)
     */
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline decltype(auto) find_intersecting_np(const ShapeT& shape) const;

    /**
     * \brief Gets the ids of the the nearest K objects
     * \returns The object ids, identifier_t or gid_segm_t, depending on the default id getter
     */
    template <typename ShapeT>
    inline decltype(auto) find_nearest(const ShapeT& shape, unsigned k_neighbors) const;

    /// \brief Counts objects intersecting the given region deliminted by the shape
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline size_t count_intersecting(const ShapeT& shape) const;

    /// \brief Counts objects intersecting the given region deliminted by the shape
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline std::unordered_map<identifier_t, size_t> count_intersecting_agg_gid(
        const ShapeT& shape) const;
};

/**
 * \brief IndexTree is a Boost::rtree spatial index tree with helper methods
 *    for finding intersections and serialization.
 *
 * \note: For large arrays of raw data (vec[floats]...) consider using make_soa_reader to
 *       avoid duplicating all the data in memory. Init using IndexTree(soa.begin(), soa.end())
 */
template <typename T, typename A = boost::container::new_allocator<T>>
class IndexTree: public IndexTreeMixin<IndexTree<T, A>, T>, public IndexTreeBaseT<T, A> {
    using super = IndexTreeBaseT<T, A>;

  public:
    using value_type = T;
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
                       std::forward<Alloc>(alloc)) {}

    /// \brief Constructor to rebuild from binary data file
    // Note: One must override char* and string since there is a template<T> constructor
    inline explicit IndexTree(const std::string& filename);
    inline explicit IndexTree(const char* dump_file)
        : IndexTree(std::string(dump_file)) {}

    /// \brief Output tree to binary data file
    inline void dump(const std::string& filename) const;

    /// \brief Checks whether a given shape intersects any object in the tree
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline bool is_intersecting(const ShapeT& shape) const;

    /**
     * \brief Finds & return objects which intersect. To be used mainly with id-less objects
     * \returns A vector of references to tree objects
     */
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline std::vector<cref_t> find_intersecting_objs(const ShapeT& shape) const;


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
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if(version > SPATIAL_INDEX_STRUCT_VERSION) {
            throw std::runtime_error(
                "File format is in a future format. Please update Spatial Index."
            );
        }

        if(version <= 3) {
            log_error("Indexes with version 0, ..., 3 could have one of many conventions. These"
                      " indexes cannot be loaded safely anymore and must be recreated.");

            auto error_message = boost::format("Invalid index version: %d") % version;
            throw std::runtime_error(error_message.str());
        }

        ar & boost::serialization::base_object<super>(*this);
    }
};

}  // namespace spatial_index

#include "detail/index.hpp"
