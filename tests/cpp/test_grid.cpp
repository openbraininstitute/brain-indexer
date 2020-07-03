#define BOOST_TEST_MODULE SpatialIndex_Benchmarks

#include <algorithm>

#include <boost/test/unit_test.hpp>
#include <spatial_index/index_grid.hpp>


// To test in an independent & easy way, specialize the Grid for plain int's
namespace spatial_index {

template <>
struct GridPlacementHelper<int> : public GridPlacementHelperBase<int>{
    using GridPlacementHelperBase<int>::GridPlacementHelperBase;

    template <int VoxelLen>
    inline void insert(int value) {
        this->grid_[point2voxel<VoxelLen>(Point3D{float(value), 0, 0})]
            .push_back(value);
    }

};

}  // namespace spatial_index;


using namespace spatial_index;


BOOST_AUTO_TEST_CASE(BasicTest) {
    SpatialGrid<int, 5> grid;

    grid.insert(1);
    grid.insert(3);
    grid.insert(6);
    grid.insert(-1);

    std::cout << grid << std::endl;

    const auto& v0 = grid[{0,0,0}];
    const auto& v1 = grid[{1,0,0}];
    const auto& v2 = grid[{-1,0,0}];
    BOOST_CHECK_EQUAL(v0.size(), 2);
    BOOST_CHECK_EQUAL(v1.size(), 1);
    BOOST_CHECK_EQUAL(v2.size(), 1);
    BOOST_CHECK(v0[0] == 1);
    BOOST_CHECK(v1[0] == 6);
    BOOST_CHECK(v2[0] == -1);
}


BOOST_AUTO_TEST_CASE(MorphoEntryTest) {
    SpatialGrid<MorphoEntry, 5> grid;

    grid.insert(Soma(0ul, Point3D{2, 2, 2}, 1.f));  // goes to voxel 0

    // gets in the middle of two voxels -1/0
    grid.insert(Soma(1ul, Point3D{1, 2, 2}, 2.f));

    // Bulk insert
    grid.insert({
        Soma(2ul, Point3D{-2, 2, 2}, 1.f),  // to voxel -1
        Segment(3ul, 1u, Point3D{-2, -2, 2}, Point3D{1, -2, 2}, 1.f)  // split in voxels [-1/0, -1, 0]
    });

    std::cout << grid << std::endl;

    const auto& v0 = grid[{0,0,0}];
    const auto& v1 = grid[{-1,0,0}];
    const auto& v2 = grid[{-1,-1,0}];
    const auto& v3 = grid[{0,-1,0}];

    BOOST_CHECK_EQUAL(v0.size(), 2);
    BOOST_CHECK(detail::get_id_from(v0[0]) == 0);
    BOOST_CHECK(detail::get_id_from(v0[1]) == 1);

    BOOST_CHECK_EQUAL(v1.size(), 2);
    BOOST_CHECK(detail::get_id_from(v1[0]) == 1);
    BOOST_CHECK(detail::get_id_from(v1[1]) == 2);

    BOOST_CHECK_EQUAL(v2.size(), 1);
    BOOST_CHECK(detail::get_id_from(v2[0]) == 3);
    BOOST_CHECK_EQUAL(v3.size(), 1);
    BOOST_CHECK(detail::get_id_from(v3[0]) == 3);
}


BOOST_AUTO_TEST_CASE(OptimizedMorphoGrid) {
    MorphSpatialGrid<5> grid;

    // 5 points, 2 branches, 3 segments
    std::vector<CoordType> points{1,1,1, 2,2,2, 3,3,3, 3,2,2, 7,7,7};
    std::vector<CoordType> radius{1, 1, 1, 1, 1};
    std::vector<unsigned> offsets{0, 3, 5};

    auto raw_points = reinterpret_cast<const Point3D*>(points.data());

    grid.add_branches(9, int(offsets.size()) - 1, raw_points, radius.data(), offsets.data());

    std::cout << grid << std::endl;

    const auto& v0 = grid[{0,0,0}];
    const auto& v1 = grid[{1,1,1}];
    BOOST_CHECK_EQUAL(v0.size(), 3);
    BOOST_CHECK_EQUAL(v1.size(), 1);

    auto check_ids = [](auto v, std::vector<unsigned> ids_expected) {
        std::vector<gid_segm_t> ids;
        std::copy(v.cbegin(), v.cend(), iter_gid_segm_getter{ids});
        unsigned i = 0;
        for (const auto& id : ids) {
            if (id.segment_i != ids_expected[i++])
                return false;
        }
        return true;
    };

    BOOST_CHECK(check_ids(v0, {1, 2, 3}));
    BOOST_CHECK(check_ids(v1, {3}));

}
