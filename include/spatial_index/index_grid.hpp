#pragma once

#include <map>
#include <iostream>
#include <cmath>

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
        helper_.template insert<VoxelLength>(value);
    }

    inline void insert(const value_type* begin, const value_type* end) {
        while (begin++ < end) insert(*begin);
    }

    inline void insert(const std::vector<value_type>& vec) {
        insert(&vec.front(), &vec.back());
    }

    inline int size(){}

    inline bool empty(){}

    inline void clear(){}

    inline void print() {
        for (const auto& tpl : grid_) {
            std::cout << tpl.first[0] << ", " << tpl.first[1] << ", " << tpl.first[2] <<
            std::endl;
            for (const auto& item : tpl.second) {
                std::cout << "   : " << item << std::endl;
            }
        }
    };

  protected:
    std::map<std::array<int, 3>, std::vector<T>> grid_;
    GridPlacementHelper<T> helper_{grid_};
};


}  // namespace spatial_index
