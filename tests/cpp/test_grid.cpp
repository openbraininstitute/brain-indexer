#define BOOST_TEST_MODULE SpatialIndex_Benchmarks
#include <boost/test/unit_test.hpp>
#include <spatial_index/index_grid.hpp>


using namespace spatial_index;

namespace spatial_index {

template <>
struct GridPlacementHelper<int> : public GridPlacementHelperBase_<int>{
    using GridPlacementHelperBase_<int>::GridPlacementHelperBase_;

    template <int VoxelLen>
    inline void insert(int value) {
        this->grid_[point2voxel<VoxelLen>(Point3D{value, 0, 0})].push_back(value);
    }

};

}  // namespace spatial_index;


BOOST_AUTO_TEST_CASE(BasicTest) {
    SpatialGrid<int, 5> grid;

    grid.insert(1);
    grid.insert(3);
    grid.insert(6);
    grid.insert(-1);

    grid.print();
}


BOOST_AUTO_TEST_CASE(MorphoEntryTest) {
    SpatialGrid<MorphoEntry, 5> grid;

    grid.insert(Soma(0ul, Point3D{2, 2, 2}, 1.f));

    // gets in the middle of two voxels
    grid.insert(Soma(1ul, Point3D{1, 2, 3}, 2.f));

    grid.insert({
        Soma(1ul, Point3D{-2, 2, 2}, 1.f),
        Segment(2ul, 1, Point3D{-2, -2, 2}, Point3D{0,-2, 2}, 1.f)
    });

    grid.print();
}
