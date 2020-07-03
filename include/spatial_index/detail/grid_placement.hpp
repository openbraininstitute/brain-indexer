#pragma once

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
inline std::array<int, 3> point2voxel(const Point3D& value) {
    return {
        int(std::floor(value.get<0>() / VoxelLen)),
        int(std::floor(value.get<1>() / VoxelLen)),
        int(std::floor(value.get<2>() / VoxelLen))
    };
}



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


template <int VoxelLen>
inline void GridPlacementHelper<MorphoEntry>::insert(const MorphoEntry& value) {
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
inline void GridPlacementHelper<MorphoEntry>::insert(
        identifier_t gid, unsigned segment_i,
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


}  // namespace spatial_index