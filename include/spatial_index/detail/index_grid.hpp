#pragma once

#include "grid_db.hpp"
#include "../index_grid.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>
#include <map>
#include <numeric>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>


namespace spatial_index {


// class SpatialGrid
// -----------------

template <typename T, int VoxelLen>
inline std::size_t SpatialGrid<T, VoxelLen>::size() const noexcept {
    return std::accumulate(grid_.cbegin(), grid_.cend(), 0ul,
        [](std::size_t previous_size, const auto& pair) {
            return previous_size + pair.second.size();
        }
    );
}


template <typename T, int VoxelLen>
inline auto SpatialGrid<T, VoxelLen>::voxels() const {
    std::vector<key_type> v;
    v.reserve(grid_.size());
    for (const auto& pair : grid_) {
        v.push_back(pair.first);
    }
    return v;
}


template <typename T, int VoxelLen>
template <class Archive>
inline void SpatialGrid<T, VoxelLen>::serialize(Archive& ar, const unsigned int) {
    ar & grid_;
}


template <typename T, int VoxelLen>
inline SpatialGrid<T, VoxelLen>&
SpatialGrid<T, VoxelLen>::operator+=(const SpatialGrid<T, VoxelLen>& rhs) {
    for (const auto& item : rhs.grid_) {
        auto& v = grid_[item.first];
        v.reserve(v.size() + item.second.size());
        v.insert(v.end(), item.second.begin(), item.second.end());
    }
    return *this;
}


template <typename T, int VoxelLen>
inline bool SpatialGrid<T, VoxelLen>::operator==(const SpatialGrid<T, VoxelLen>& rhs) {
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


template <typename T, int VoxelLen>
inline std::ostream& operator<<(std::ostream& os,
                                const spatial_index::SpatialGrid<T, VoxelLen>& obj) {
    using spatial_index::operator<<;
    os << boost::format("SpatialGrid<%d>({\n") % VoxelLen;
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


template <typename T, int VoxelLen>
inline void SpatialGrid<T, VoxelLen>::create_indexes_disk(const std::string& location) {
    using IndexT = IndexTree<T>;
    detail::IndexDB<IndexT> disk_index_(location, detail::OpenMode::WriteTruncate,
                                        VoxelLen);
    for (const auto& voxel : grid_) {
        disk_index_.store(voxel.first,
                          IndexT(voxel.second.cbegin(), voxel.second.cend()));
    }
}



// class MultiIndex
// ----------------

template <typename T>
inline MultiIndex<T>::MultiIndex(const std::string& disk_location,
                                 const IndexPart& part)
    : disk_index_(disk_location) {
    const auto n_parts = disk_index_.voxels_avail().size();
    auto start = part.index * n_parts / part.total;
    auto end = (part.index + 1) * n_parts / part.total;
    for (auto i=start; i<end; i++) {
        disk_index_.load_into(disk_index_.voxels_avail()[i], indexes_);
    }
}


template <typename T>
inline std::size_t MultiIndex<T>::size() const noexcept {
    return std::accumulate(indexes_.cbegin(), indexes_.cend(), 0,
        [](int previous_size, const auto& tree) {
            return previous_size + tree.size();
        }
    );
}


template <typename T>
inline void MultiIndex<T>::load_region(const Box3D& region) {
    const auto& min_voxel = detail::point2voxel(region.min_corner(), voxel_length());
    const auto& max_voxel = detail::point2voxel(region.max_corner(), voxel_length());
    for (const auto& vx : disk_index_.voxels_avail()) {
        if (min_voxel[0] <= vx[0] && vx[0] <= max_voxel[0] &&
                min_voxel[1] <= vx[1] && vx[1] <= max_voxel[1] &&
                min_voxel[2] <= vx[2] && vx[2] <= max_voxel[2]) {
            disk_index_.load_into(vx, indexes_);
        }
    }
}


template <typename T>
inline decltype(auto) MultiIndex<T>::find_within(const Box3D& shape) const {
    using ids_getter = typename detail::id_getter_for<T>::type;
    std::vector<typename ids_getter::value_type> ids;
    apply(
        [&shape](const index_type& index, auto& ids_output) {
            index.find_intersecting(shape, ids_getter(ids_output));
        },
        ids
    );
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}


template <typename T>
template <typename ResultT, typename F>
inline ResultT& MultiIndex<T>::apply(F func, ResultT& results) const {
    for (const auto& index : indexes_) {
        func(index, results);
    }
    return results;
}


template <typename T>
template <typename ResultElemT, typename F>
inline std::vector<ResultElemT>&
MultiIndex<T>::apply_par(F func, std::vector<ResultElemT>& results) const  {
    std::vector<std::vector<ResultElemT>> subvectors;
    std::vector<std::future<std::vector<ResultElemT>>> futures;
    for (const auto& item : indexes_) {
        subvectors.emplace_back();
        futures.push_back(
            std::async(std::launch::async, func, item.second, subvectors.back()));
    }
    for(auto& fut : futures) {
        const auto& vec = fut.get();
        if (results.empty()) {
            results = std::move(vec);
        } else {
            results.reserve(results.size() + vec.size());
            results.insert(results.end(), vec.begin(), vec.end());
        }
    }
}


}  // namespace spatial_index
