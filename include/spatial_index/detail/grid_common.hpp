#pragma once

#include <array>
#include <unordered_map>

#include <boost/container/small_vector.hpp>


namespace spatial_index {

// Fw decl
template <typename T>
class MultiIndex;

namespace detail {

/// \brief A hash array function for std::array
template <typename T, std::size_t N>
struct hash_array {
    std::size_t operator()(const std::array<T, N>& key) const {
        std::size_t out{};
        for (const auto& item : key)
            out = 127 * out + std::hash<T>{}(item);
        return out;
    }
};

/// \brief The underlying type of the voxel id
using voxel_id_type = std::array<int, 3>;

/// \brief The fundamental type of the grid
template <typename T>
using grid_type = std::unordered_map<voxel_id_type,
                                     std::vector<T>,
                                     hash_array<int, 3>>;

/// Type for a vector containing voxels a geometry crosses
using VoxelSet = boost::container::small_vector<voxel_id_type, 4>;

}  // namespace detail
}  // namespace spatial_index
