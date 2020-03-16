#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

#include "index.hpp"

namespace spatial_index {


template <int VoxelLength>
inline std::array<int, 3> point2voxel(const Point3D& value);


template <typename T>
struct GridPlacementHelperBase_ {
    using grid_type = std::map<std::array<int, 3>, std::vector<T>>;

    inline GridPlacementHelperBase_(grid_type& grid)
        : grid_(grid) {}

  protected:
    grid_type& grid_;
};


template <typename T>
struct GridPlacementHelper : public GridPlacementHelperBase_<T> {
    using GridPlacementHelperBase_<T>::GridPlacementHelperBase_;

    template <int VoxelLen>
    inline void insert(const T& value) {
        this->grid_[point2voxel<VoxelLen>(value)].push_back(value);
    }
};


template <>
struct GridPlacementHelper<MorphoEntry> : public GridPlacementHelperBase_<MorphoEntry>{
    using GridPlacementHelperBase_<MorphoEntry>::GridPlacementHelperBase_;

    template <int VoxelLen>
    void insert(const MorphoEntry& value);

    template <int VoxelLen>
    inline void insert(identifier_t gid, unsigned segment_i,
                       const Point3D& p1, const Point3D& p2, CoordType radius);
};


/**
 * A class holding a grid of spatially split objects
 * Voxels are the unitary regions, aligned at (0,0,0), each creating an RTree
 */
template <typename T, int VoxelLength>
class SpatialGrid {
  public:
    using value_type = T;
    using key_type = std::array<int, 3>;

    inline void insert(const value_type & value) {
        placer_.template insert<VoxelLength>(value);
    }

    inline void insert(const value_type* begin, const value_type* end) {
        while (begin < end) insert(*begin++);
    }

    inline void insert(const std::vector<value_type>& vec) {
        insert(&vec.front(), &vec.back());
    }

    inline int size() const noexcept;

    inline auto voxels() const;

    inline const auto& items() const noexcept { return grid_; }

    inline const auto& operator[](const key_type& key) const {
        return grid_[key];
    }

  protected:

    std::map<key_type, std::vector<T>> grid_;
    GridPlacementHelper<T> placer_{grid_};

     // Serialization
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) ;
};


// Specific for morphologies
template <int VoxelLength>
class MorphSpatialGrid : public SpatialGrid<MorphoEntry, VoxelLength> {
  public:
    inline void add_neuron(identifier_t gid,
                           int n_branches,
                           const Point3D *points,
                           const CoordType *radius,
                           const unsigned *offsets) {
        unsigned segment_i = 1;
        for (int branch_i = 0; branch_i < n_branches; branch_i++) {
            const unsigned branch_end = offsets[branch_i + 1] - 1;
            for (unsigned i = offsets[branch_i]; i < branch_end; i++) {
                this->placer_.template insert<VoxelLength>(
                    gid, segment_i++, points[i], points[i + 1], radius[i]);
            }
        }

    }
};

}  // namespace spatial_index

#include "detail/index_grid.hpp"
