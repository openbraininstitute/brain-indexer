#pragma once

#include "../index_grid.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>



namespace spatial_index {


template <int VoxelLength>
inline std::array<int, 3> point2voxel(const Point3D& value) {
    return {
        int(std::floor(value.get<0>() / VoxelLength)),
        int(std::floor(value.get<1>() / VoxelLength)),
        int(std::floor(value.get<2>() / VoxelLength))
    };
}


template <int VoxelLen>
inline void GridPlacementHelper<MorphoEntry>::
insert(const MorphoEntry& value) {
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
inline void GridPlacementHelper<MorphoEntry>::
insert(identifier_t gid, unsigned segment_i,
       const Point3D& p1, const Point3D& p2, CoordType radius) {
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


template <typename T, int VoxelLength>
inline int SpatialGrid<T, VoxelLength>:: size() const noexcept {
    return std::accumulate(grid_.cbegin(), grid_.cend(), 0,
        [](int previous_size, const auto& pair) {
            return previous_size + pair.second.size();
        }
    );
}


template <typename T, int VoxelLength>
inline auto SpatialGrid<T, VoxelLength>::voxels() const {
    std::vector<key_type> v;
    v.reserve(grid_.size());
    for (const auto& pair : grid_) {
        v.push_back(pair.first);
    }
    return v;
}


template <typename T, int VL>
template <class Archive>
inline void SpatialGrid<T, VL>::serialize(Archive& ar, const unsigned int) {
    ar& grid_;
}


template <typename T, int VL>
inline SpatialGrid<T, VL>& SpatialGrid<T, VL>::operator+=(const SpatialGrid<T, VL>& rhs) {
    for (const auto& item : rhs.grid_) {
        auto& v = grid_[item.first];
        v.reserve(v.size() + item.second.size());
        v.insert(v.end(), item.second.begin(), item.second.end());
    }
    return *this;
}


template <typename T, int VL>
inline std::ostream& operator<<(std::ostream& os, const spatial_index::SpatialGrid<T, VL>& obj) {
    using spatial_index::operator<<;
    os << boost::format("<obj: SpatialGrid<%d> [") % VL << std::endl;
    for (const auto& tpl : obj.items()) {
        os << boost::format(" Vx[%d, %d, %d]") % tpl.first[0] % tpl.first[1] % tpl.first[2]
           << std::endl;
        for (const auto& item : tpl.second) {
            os << "   " << item << std::endl;
        }
    }
    os << "]>";
    return os;
}





}  // namespace spatial_index

