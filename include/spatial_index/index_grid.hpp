#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

#include "index.hpp"

namespace spatial_index {


template <int VoxelLength>
inline std::array<int, 3> point2voxel(const Point3D& value) {
    return {
        int(std::floor(value.get<0>() / VoxelLength)),
        int(std::floor(value.get<1>() / VoxelLength)),
        int(std::floor(value.get<2>() / VoxelLength))
    };
}


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
    inline void insert(const MorphoEntry& value) {
        auto bbox = boost::apply_visitor(
            [](const auto& elem) { return elem.bounding_box(); },
            value
        );
        auto vx1_i = point2voxel<VoxelLen>(bbox.min_corner());
        auto vx2_i = point2voxel<VoxelLen>(bbox.max_corner());
        // If the corners of the bbox fall in different voxels then we add the item to
        // both. This is a simplification of the very precise thing (since it may cross
        // up to 8 voxels!) but given that cylinders length is larger than width it
        // sounds as a reasonable approximation
        this->grid_[vx1_i].push_back(value);
        if (vx1_i != vx2_i) {
            this->grid_[vx2_i].push_back(value);
        }
    }

    template <int VoxelLen>
    inline void insert(identifier_t gid,
                       unsigned segment_i,
                       const Point3D& p1,
                       const Point3D& p2,
                       CoordType radius) {
        // This attempts at optimizing the common case of init segments from a
        // point array without temps
        const auto& vx1_i = point2voxel<VoxelLen>(p1);
        const auto& vx2_i = point2voxel<VoxelLen>(p2);
        auto& vec = this->grid_[vx1_i];
        vec.push_back(Segment{gid, segment_i, p1, p2, radius});
        if (vx1_i != vx2_i) {
            this->grid_[vx2_i].push_back(vec.back());
        }
    }
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

    inline int size() const noexcept {
        return std::accumulate(grid_.cbegin(), grid_.cend(), 0,
            [](int previous_size, const auto& pair) {
                return previous_size + pair.second.size();
            }
        );
    }

    inline auto voxels() const {
        std::vector<key_type> v;
        v.reserve(grid_.size());
        for (const auto& pair : grid_) {
            v.push_back(pair.first);
        }
        return v;
    }

    inline const auto& items() const noexcept {
        return grid_;
    }

    inline const std::vector<T>& operator[](const key_type& key) const {
        return grid_[key];
    }

    inline void print() const {
        for (const auto& tpl : grid_) {
            std::cout << tpl.first[0] << ", " << tpl.first[1] << ", " << tpl.first[2] <<
            std::endl;
            for (const auto& item : tpl.second) {
                std::cout << "   | " << item << std::endl;
            }
        }
    }

  protected:

    std::map<key_type, std::vector<T>> grid_;

    GridPlacementHelper<T> placer_{grid_};
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
