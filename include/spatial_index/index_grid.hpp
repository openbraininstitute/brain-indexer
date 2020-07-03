#pragma once

#include <cmath>
#include <iostream>
#include <numeric>
#include <unordered_map>

#include "index.hpp"
#include "detail/grid_placement.hpp"


namespace spatial_index {


/**
 * A class holding a grid of spatially split objects
 * Voxels are the unitary regions, aligned at (0,0,0), each creating an RTree
 */
template <typename T, int VoxelLength>
class SpatialGrid {
  public:
    using value_type = T;
    using grid_type = detail::grid_type<T>;
    using key_type = std::array<int, 3>;

    /** \brief Insert a single value in the tree
     */
    inline void insert(const value_type & value) {
        GridPlacementHelper<T>{grid_}.template insert<VoxelLength>(value);
    }

    /** \brief Insert a range of elements defined by begin-end pointers
     */
    inline void insert(const value_type* begin, const value_type* end) {
        GridPlacementHelper<T> placer{grid_};
        while (begin < end)
            placer. template insert<VoxelLength>(*begin++);
    }

    /** \brief Insert a range of elements defined by begin-end pointers
     */
    inline void insert(const std::vector<value_type>& vec) {
        insert(&vec.front(), &vec.back() + 1);
    }

    /** \brief Retrieves the total number of elements in the grid
     */
    inline int size() const noexcept;

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


  protected:
    grid_type grid_;

     // Serialization
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) ;
};


// Specific for morphologies
template <int VoxelLength>
class MorphSpatialGrid : public SpatialGrid<MorphoEntry, VoxelLength> {
  public:

    inline void add_soma(identifier_t gid, const Point3D& pt, CoordType r) {
        this->placer_. template insert<VoxelLength>(Soma{gid, pt, r});
    }

    inline void add_branches(identifier_t gid,
                             int n_branches,
                             const Point3D *points,
                             const CoordType *radius,
                             const unsigned *offsets) {
        auto placer = GridPlacementHelper<MorphoEntry>{this->grid_};
        unsigned segment_i = 1;
        for (int branch_i = 0; branch_i < n_branches; branch_i++) {
            const unsigned branch_end = offsets[branch_i + 1] - 1;
            for (unsigned i = offsets[branch_i]; i < branch_end; i++) {
                placer. template insert<VoxelLength>(
                    gid, segment_i++, points[i], points[i + 1], radius[i]);
            }
        }
    }
};

}  // namespace spatial_index

#include "detail/index_grid.hpp"
