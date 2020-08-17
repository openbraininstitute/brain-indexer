#pragma once

#include <cmath>
#include <iostream>
#include <numeric>
#include <unordered_map>

#include "index.hpp"
#include "detail/grid_placement.hpp"
#include "detail/grid_db.hpp"


namespace spatial_index {


/**
 * \brief A class holding a grid of spatially split objects
 *
 * Voxels are the unitary regions, aligned at (0,0,0), each creating an RTree
 */
template <typename T, int VoxelLen>
class SpatialGrid {

  public:
    using value_type = T;
    using grid_type = detail::grid_type<T>;
    using key_type = detail::voxel_id_type;

    /** \brief Insert a single value in the tree
     */
    inline void insert(const value_type & value) {
        GridPlacementHelper<T>{grid_}.template insert<VoxelLen>(value);
    }

    /** \brief Insert a range of elements defined by begin-end pointers
     */
    inline void insert(const value_type* begin, const value_type* end) {
        GridPlacementHelper<T> placer{grid_};
        while (begin < end)
            placer. template insert<VoxelLen>(*begin++);
    }

    /** \brief Insert a range of elements defined by begin-end pointers
     */
    inline void insert(const std::vector<value_type>& vec) {
        insert(&vec.front(), &vec.back() + 1);
    }

    /** \brief Retrieves the total number of elements in the grid
     */
    inline std::size_t size() const noexcept;

    /** \brief Retrieves the ids of the voxels (keys)
     */
    inline auto voxels() const;

    /** \brief Retrieves the map (const-ref) of voxels
     */
    inline const auto& items() const noexcept { return grid_; }

    /** \brief Retieves the elements of a voxel (vector const-ref)
     */
    inline const auto& operator[](const key_type& key) const {
        return const_cast<const grid_type&>(grid_).at(key);
    }

    /** \brief inplace add user for combining grids
     */
    inline SpatialGrid& operator+=(const SpatialGrid& rhs);

    /** \brief Check spatial grids are equivallent.
    NOTE: This is a (very) quick check, it doesnt guarantee elements in
    each region are exactly the same
     */
    inline bool operator==(const SpatialGrid& rhs);

    /**
     * \brief Creates a lazy spatial index by converting SpatialGrid to IndexTrees in disk
     *
     * \param disk_location The location of the dir of the grid indexes
     */
    inline void create_indexes_disk(const std::string& location);


  protected:
    grid_type grid_;

     // Serialization
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) ;
};


/**
 * \brief A specialized SpatialGrid for MorphoEntry
 *
 * Is adds specializaed methods to add branches and somas
 */
template <int VoxelLen>
class MorphSpatialGrid : public SpatialGrid<MorphoEntry, VoxelLen> {
  public:

    inline void add_soma(identifier_t gid, const Point3D& pt, CoordType r) {
        this->placer_. template insert<VoxelLen>(Soma{gid, pt, r});
    }

    inline void add_branches(identifier_t gid,
                             int n_branches,
                             const Point3D *points,
                             const CoordType *radius,
                             const unsigned *offsets) {
        auto placer = GridPlacementHelper<MorphoEntry>{this->grid_};
        unsigned segment_i = 1;

        for (int branch_i = 0; branch_i < n_branches; branch_i++) {
            const auto branch_end = offsets[branch_i + 1] - 1;
            for (auto i = offsets[branch_i]; i < branch_end; i++) {
                placer. template insert<VoxelLen>(
                    gid, segment_i++, points[i], points[i + 1], radius[i]);
            }
        }
    }
};


/**
 * \brief Struct definiting a subset of the indexes by (part_index and nr_of_parts)
 */
struct IndexPart {
    unsigned index;
    unsigned total;

    inline static constexpr IndexPart all() noexcept {
        return {0, 1};
    }
};


/**
 * \brief MultiIndex is a structure of multiple rtrees which indexes elements from
 * contiguous regions in space. It can therefore be constructed from a SpatialGrid.
 *
 * The rational is, for very large indexes, they are partitioned in the first place,
 * stored to disk and then only the Regions of Interest loaded on demand.
 */
template <typename T>
class MultiIndex {
  public:
    using index_type = IndexTree<T>;

    /**
     * \brief Opens a grid of indexes from disk
     * NOTE: All indexes are gonna be loaded and must fit in memory
     *
     * \param disk_location The location of the dir of the grid indexes
     */
    inline MultiIndex(const std::string& disk_location)
        : disk_index_(disk_location) {
        for (const auto& v : disk_index_.voxels_avail()) {
            disk_index_.load_into(v, indexes_);
        }
    }

    /**
     * \brief Opens a grid of indexes from disk
     *
     * \param disk_location The location of the dir of the grid indexes
     * \param part The part to load
     */
    inline MultiIndex(const std::string& disk_location, const IndexPart& part);


    /**
     * \brief Opens a grid of indexes from disk as required for a certain region
     *
     * \param disk_location The location of the dir of the grid indexes
     * \param region the corners of a cubic region to load
     */
    inline MultiIndex(const std::string& disk_location,
                      const Box3D& region)
        : disk_index_(disk_location) {
        load_region(region);
    }

    /** \brief The total number of (loaded) indexed elements
     */
    std::size_t size() const noexcept;


    /** \brief Find all objects within a certain box-delimited region
     */
    inline decltype(auto) find_within(const Box3D& shape) const;


    /** \brief Apply a function to every index, appending results to argument
     */
    template <typename ResultT, typename F>
    inline ResultT& apply(F func, ResultT& results) const;

    /** \brief Apply a function to every index in parallel using threads
     *   appending results to argument
     */
    template <typename ResultElemT, typename F>
    inline std::vector<ResultElemT>& apply_par(F func,
                                               std::vector<ResultElemT>& results) const;

    /** \brief return the vector (const-ref) of individual indexes
     */
    const std::vector<index_type>& indexes() const noexcept {
        return indexes_;
    }

  protected:
    using IndexDB = detail::IndexDB<index_type>;

    inline int& voxel_length() {
        return disk_index_.voxel_length_;
    }

    inline void load_region(const Box3D& region);

  private:
    IndexDB disk_index_;
    std::vector<index_type> indexes_;

};


}  // namespace spatial_index

#include "detail/index_grid.hpp"
