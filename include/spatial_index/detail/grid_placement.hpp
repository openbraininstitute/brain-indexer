#pragma once

#include <boost/container/small_vector.hpp>

#include "../point3d.hpp"


namespace spatial_index {

namespace detail {

/// \brief A hash array function for std::array
template <typename T, std::size_t N>
struct hash_array {
    std::size_t operator()(const std::array<T, N>& key) const {
        std::size_t out = 0;
        for (const auto& item : key)
            out = 127 * out + std::hash<T>{}(item);
        return out;
    }
};

/// \brief The underlying type of the voxel id
using voxel_type = std::array<int, 3>;

/// \brief The fundamental type of the grid
template <typename T>
using grid_type = std::unordered_map<voxel_type,
                                     std::vector<T>,
                                     hash_array<int, 3>>;

/// Type for a vector containing voxels a geometry crosses
using VoxelSet = boost::container::small_vector<voxel_type, 4>;


/// \brief A Generic translator from 3d points to voxel
template <int VoxelLen>
inline voxel_type point2voxel(const Point3D& value) {
    return {
        int(std::floor(value.get<0>() / VoxelLen)),
        int(std::floor(value.get<1>() / VoxelLen)),
        int(std::floor(value.get<2>() / VoxelLen))
    };
}


template <int Dim>
inline Point3Dx point_offset(const Point3Dx& p, CoordType offset) {
    return Point3Dx(p).setx<Dim>(p.get<Dim>() + offset);
}


template <int VoxelLen>
inline static bool voxels_add(const Point3D& point, VoxelSet& voxels) {
    auto v = point2voxel<VoxelLen>(point);
    for (const auto& cur_v : voxels) {
        if (cur_v == v) {
            return false;
        }
    }
    voxels.push_back(v);
    return true;
}

}  // namespace detail


/// \brief A base class for the several specializations of GridPlacementHelper's
template <typename T>
struct GridPlacementHelperBase {
    GridPlacementHelperBase() = delete;

    inline GridPlacementHelperBase(detail::grid_type<T>& grid)
        : grid_(grid) {}

  protected:

    detail::grid_type<T>& grid_;

    /**
     * Find the voxels intersected by a sphere
     */
    template <int VoxelLen>
    inline detail::VoxelSet intersected_voxels(const Sphere& sphere) {
        using namespace detail;
        VoxelSet voxels;

        // Consider voxels containing center and 6 extremes of sphere
        const auto& c = sphere.centroid;
        voxels_add<VoxelLen>(c, voxels);
        voxels_add<VoxelLen>(point_offset<0>(c, sphere.radius), voxels);
        voxels_add<VoxelLen>(point_offset<0>(c, -sphere.radius), voxels);
        voxels_add<VoxelLen>(point_offset<1>(c, sphere.radius), voxels);
        voxels_add<VoxelLen>(point_offset<1>(c, -sphere.radius), voxels);
        voxels_add<VoxelLen>(point_offset<2>(c, sphere.radius), voxels);
        voxels_add<VoxelLen>(point_offset<2>(c, -sphere.radius), voxels);
        return voxels;
    }

    /**
     * Find the voxels intersected by a cylinder
     *
     * NOTE; this is a simplified version since in principle radius << VoxelLen
     * and therefore we only check for different voxels along the cylinder
     */
    template <int VoxelLen>
    inline detail::VoxelSet intersected_voxels(const Cylinder& cyl) {
        using namespace detail;
        VoxelSet voxels;
        const auto mid_p = (Point3Dx(cyl.p2) + cyl.p1) / 2.0;

        voxels_add<VoxelLen>(cyl.p1, voxels);
        voxels_add<VoxelLen>(cyl.p2, voxels);
        voxels_add<VoxelLen>(mid_p, voxels);
        return voxels;
    }

    /**
     * Generic find voxels intersected by an object.
     * Add the object to all those voxels.
     */
    template <int VoxelLen, typename CallObjT>
    inline void add(const CallObjT& obj) {
        for (const auto& voxel_id : intersected_voxels<VoxelLen>(obj)) {
            this->grid_[voxel_id].push_back(obj);
        }
    }

};


// Default implementation
template <typename T>
struct GridPlacementHelper : public GridPlacementHelperBase<T> {
    using GridPlacementHelperBase<T>::GridPlacementHelperBase;

    /// Attempt using the low-level add() routines via overloading
    template <int VoxelLen>
    inline void insert(const T& obj) {
        this->template add<VoxelLen>(obj);
    }
};


template <>
struct GridPlacementHelper<MorphoEntry> : public GridPlacementHelperBase<MorphoEntry>{
    using GridPlacementHelperBase<MorphoEntry>::GridPlacementHelperBase;

    template <int VoxelLen>
    void insert(const MorphoEntry& value) {
        boost::apply_visitor(
            [this](const auto& elem) { this-> template add<VoxelLen>(elem); },
            value
        );
    }

    template <int VoxelLen>
    inline void insert(identifier_t gid, const Point3D& center, CoordType radius) {
        this-> template add<VoxelLen>(Soma{gid, center, radius});
    }

    template <int VoxelLen>
    inline void insert(identifier_t gid, unsigned segment_i,
                       const Point3D& p1, const Point3D& p2, CoordType radius) {
        this-> template add<VoxelLen>(Segment{gid, segment_i, p1, p2, radius});
    }
};

}  // namespace spatial_index