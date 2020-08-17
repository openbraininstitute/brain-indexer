#define BOOST_TEST_MODULE SpatialIndex_Benchmarks

#include <algorithm>
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <spatial_index/index_grid.hpp>


// To test in an independent & easy way, specialize the Grid for plain int's
namespace spatial_index {

template <>
struct GridPlacementHelper<int> : public GridPlacementHelperBase<int>{
    using GridPlacementHelperBase<int>::GridPlacementHelperBase;

    template <int VoxelLen>
    inline void insert(int value) {
        this->grid_[detail::point2voxel<VoxelLen>(Point3D{float(value), 0, 0})]
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

    // std::cout << grid << std::endl;

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

    //std::cout << grid << std::endl;

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


BOOST_AUTO_TEST_CASE(TestOptimizedMorphoGrid) {
    MorphSpatialGrid<5> grid;

    // 5 points, 2 branches, 3 segments
    std::vector<CoordType> points{1,1,1, 2,2,2, 3,3,3, 3,2,2, 7,7,7};
    std::vector<CoordType> radius{1, 1, 1, 1, 1};
    std::vector<unsigned> offsets{0, 3, 5};

    auto raw_points = reinterpret_cast<const Point3D*>(points.data());

    grid.add_branches(9, int(offsets.size()) - 1, raw_points, radius.data(), offsets.data());

    //std::cout << grid << std::endl;

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


// Print Helper
template <typename T>
std::ostream& operator<<(std::ostream& output, std::vector<T> const& values) {
    output << "std::vector{";
    for (auto const& value : values) {
        output << value << ", ";
    }
    return output << '}';
}


BOOST_AUTO_TEST_CASE(TestCrossCornerIndexing) {
    {
        SpatialGrid<IndexedSphere, 10> grid;
        grid.insert(IndexedSphere{1u,  Point3D{0, 0, 0}, 1.f});

        BOOST_CHECK_EQUAL(grid.voxels().size(), 4);  // falls in 4 voxels
        BOOST_CHECK_EQUAL((grid[{ 0, 0, 0}].size()), 1);
        BOOST_CHECK_EQUAL((grid[{-1, 0, 0}].size()), 1);
        BOOST_CHECK_EQUAL((grid[{ 0,-1, 0}].size()), 1);
        BOOST_CHECK_EQUAL((grid[{ 0, 0,-1}].size()), 1);
    }


    std::vector<Point3D> points_in{{5, 5, 0.5}, {5, 0.5, 5}, {0.5, 5, 5}};
    std::vector<std::array<std::array<int, 3>, 2>> expected_voxels {
        {{{0,0,0}, {0,0,-1}}},
        {{{0,0,0}, {0,-1,0}}},
        {{{0,0,0}, {-1,0,0}}}
    };
    for (unsigned i=0; i < 3; i++){
        SpatialGrid<IndexedSphere, 10> grid;
        grid.insert(IndexedSphere{1u,  points_in[i], 1.f});
        BOOST_CHECK_EQUAL(grid.voxels().size(), 2);  // falls in 2 voxels
        BOOST_CHECK_EQUAL((grid[expected_voxels[i][0]].size()), 1);
        BOOST_CHECK_EQUAL((grid[expected_voxels[i][1]].size()), 1);
    }
}

namespace std {

}

BOOST_AUTO_TEST_CASE(TestMultiIndex) {
    {   // Create the multi index in disk
        SpatialGrid<IndexedSphere, 10> grid;

        // Weird but each index must have at least 2 objects
        // Use y=1 so that it doesnt lie on an edge
        grid.insert(IndexedSphere{1u,  Point3D{1, 1, 0}, .9f});
        grid.insert(IndexedSphere{2u,  Point3D{2, 1, 0}, .9f});
        grid.insert(IndexedSphere{3u,  Point3D{7, 1, 0}, .9f});
        grid.insert(IndexedSphere{34u, Point3D{7, 1, 0}, .9f});
        grid.insert(IndexedSphere{4u,  Point3D{-1, 1, 0}, .9f});
        grid.insert(IndexedSphere{44u, Point3D{-1, 1, 0}, .9f});
        std::cout << grid;
        grid.create_indexes_disk("my_index");

    }

    {   // Open whole multi-index
        MultiIndex<IndexedSphere> mi("my_index");
        BOOST_CHECK_EQUAL(mi.indexes().size(), 4);  // (0 0 0), (0 0 -1), (-1 0 0), (-1 0 -1),

        // Doe to mirroring (into z=-1), we have double the elements
        int total_elems = std::accumulate(mi.indexes().cbegin(), mi.indexes().cend(), 0,
                                          [](int prev_sum, const auto& elem){
                                                return prev_sum + elem.size();
                                          });
        BOOST_CHECK_EQUAL(total_elems, 12);

        auto ids = mi.find_within(Box3D({0,0,0}, {10,10,10}));
        BOOST_CHECK_EQUAL(ids.size(), 4);
        BOOST_CHECK((ids == std::vector<size_t>{1, 2, 3, 34}));

        ids = mi.find_within(Box3D({-10, 0, 0}, {0, 10, 10}));
        BOOST_CHECK_EQUAL(ids.size(), 2);
        BOOST_CHECK((ids == std::vector<size_t>{4, 44}));

        ids = mi.find_within(Box3D({-10, -10, -10}, {10, 10, 10}));
        BOOST_CHECK_EQUAL(ids.size(), 6);
        BOOST_CHECK((ids == std::vector<size_t>{1, 2, 3, 4, 34, 44}));

    }

    {   // Open a region. Same selection as before, less results
        MultiIndex<IndexedSphere> mi("my_index", {{-100, 0, 0}, {-0.1f, 100, 100}});
        auto ids = mi.find_within(Box3D({-10, -10, -10}, {10, 10, 10}));
        // std::cout << ids << std::endl;
        BOOST_CHECK_EQUAL(ids.size(), 2);
        BOOST_CHECK((ids == std::vector<size_t>{4, 44}));
    }
}
