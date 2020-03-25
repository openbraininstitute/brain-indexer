#pragma once

#include <cmath>
#include <iostream>
#include <numeric>
#include <unordered_map>

#include "index.hpp"

namespace spatial_index {

namespace detail {

/// \brief A hash array function for std::array
template <typename T, std::size_t N>
struct hash_array {
    std::size_t operator()(const std::array<T, N>& key) const;
};

/// \brief The fundamental type of the grid
template <typename T>
using grid_type = std::unordered_map<std::array<int, 3>,
                                     std::vector<T>,
                                     hash_array<int, 3>>;

}  // namespace detail


/// \brief A Generic translator from 3d points to voxel
template <int VoxelLen>
inline std::array<int, 3> point2voxel(const Point3D& value);


/// \brief A base class for the several specializations of GridPlacementHelper's
template <typename T>
struct GridPlacementHelperBase {
    GridPlacementHelperBase() = delete;

    inline GridPlacementHelperBase(detail::grid_type<T>& grid)
        : grid_(grid) {}

    template <int VoxelLen>
    inline void insert(const T& value);  // implementation in subclasses

  protected:
    detail::grid_type<T>& grid_;
};


template <typename T>
struct GridPlacementHelper : public GridPlacementHelperBase<T> {
    using GridPlacementHelperBase<T>::GridPlacementHelperBase;

    template <int VoxelLen>
    inline void insert(const T& value) {
        this->grid_[point2voxel<VoxelLen>(value)].push_back(value);
    }
};


template <>
struct GridPlacementHelper<MorphoEntry> : public GridPlacementHelperBase<MorphoEntry>{
    using GridPlacementHelperBase<MorphoEntry>::GridPlacementHelperBase;

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
        insert(&vec.front(), &vec.back());
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
        return grid_[key];
    }

    /** \brief inplace add user for combining grids
     */
    inline SpatialGrid& operator+=(const SpatialGrid& rhs);

  protected:

    detail::grid_type<T> grid_;

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
