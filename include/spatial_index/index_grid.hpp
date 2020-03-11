#pragma once

#include <map>
#include <iostream>
#include <cmath>

#include "index.hpp"

namespace spatial_index {

template <typename T>
inline std::array<int, 3> floor(const T& value, int divisor = 1) {
    return {
        int(std::floor(std::get<0>(value) / divisor)),
        int(std::floor(std::get<1>(value) / divisor)),
        int(std::floor(std::get<2>(value) / divisor))
    };
}

template <>
inline std::array<int, 3> floor(const Point3D& value, int divisor) {
    return {
        int(std::floor(value.get<0>() / divisor)),
        int(std::floor(value.get<1>() / divisor)),
        int(std::floor(value.get<2>() / divisor))
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
    inline int insert(const T& value) {
        this->grid_[floor(value, VoxelLen)].push_back(value);
        return 1;
    }
};


template <>
struct GridPlacementHelper<MorphoEntry> : public GridPlacementHelperBase_<MorphoEntry>{
    using GridPlacementHelperBase_<MorphoEntry>::GridPlacementHelperBase_;

    template <int VoxelLen>
    inline int insert(const MorphoEntry& value) {
        auto bbox = boost::apply_visitor(
            [](const auto& elem) { return elem.bounding_box(); },
            value
        );
        auto vx1_i = floor(bbox.min_corner(), VoxelLen);
        auto vx2_i = floor(bbox.max_corner(), VoxelLen);
        // If the corners of the bbox fall in different voxels then we add the item to
        // both. This is a simplification of the very precise thing (since it may cross
        // up to 8 voxels!) but given that cylinders length is larger than width it
        // sounds as a reasonable approximation
        this->grid_[vx1_i].push_back(value);
        if (vx1_i != vx2_i) {
            this->grid_[vx2_i].push_back(value);
            return 2;
        }
        return 1;
    }
};

/**
 * A class holding a grid of spatially split objects
 * Voxels are the unitary regions, aligned at (0,0,0), each creating an RTree
 */
template <typename T, int VoxelLength>
class SpatialGrid {
  public:
    typedef T value_type;

    inline void insert(const value_type & value) {
        GridPlacementHelper<T>(grid_).template insert<VoxelLength>(value);
    }

    inline int size(){}

    inline bool empty(){}

    inline void clear(){}

    inline void print() {
        for (const auto& tpl : grid_) {
            std::cout << tpl.first[0] << ", " << tpl.first[1] << ", " << tpl.first[2] <<
            std::endl;
            for (const auto& pt : tpl.second) {
                std::cout << "   : " << pt << std::endl;
            }
        }
    };

  protected:
    std::map<std::array<int, 3>, std::vector<T>> grid_;
};

}  // namespace spatial_index
