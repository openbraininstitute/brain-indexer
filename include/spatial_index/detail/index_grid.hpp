#pragma once

#include "../index_grid.hpp"


#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>


namespace spatial_index {

namespace detail {

template <typename T, std::size_t N>
inline std::size_t hash_array<T, N>::operator()(const std::array<T, N>& arr) const {
    std::size_t out = 0;
    for (const auto& item : arr)
        out = 127 * out + std::hash<T>{}(item);
    return out;
}

}  // namespace detail




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
inline bool SpatialGrid<T, VL>::operator==(const SpatialGrid<T, VL>& rhs) {
    if (grid_.size() != rhs.grid_.size()) {
        return false;
    }
    for (const auto& rhs_item : rhs.grid_) {
        const auto& lhs_item = grid_.find(rhs_item.first);
        if (lhs_item == grid_.end()) {
            return false;
        }
        // We delegate comparison to the std::vector implementation
        // Elements must implement operator==
        return lhs_item->second == rhs_item.second;
    }
    return true;
}


template <typename T, int VL>
inline std::ostream& operator<<(std::ostream& os,
                                const spatial_index::SpatialGrid<T, VL>& obj) {
    using spatial_index::operator<<;
    os << boost::format("SpatialGrid<%d>({\n") % VL;
    for (const auto& item : obj.items()) {
        const auto& idx = item.first;
        os << boost::format(" (%d %d %d): [\n") % idx[0] % idx[1] % idx[2];
        for (const auto& entry : item.second) {
            os << "    " << entry << '\n';
        }
        os << " ],\n";
    }
    return os << "})";
}


}  // namespace spatial_index
