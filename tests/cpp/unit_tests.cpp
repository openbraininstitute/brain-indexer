#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/included/unit_test.hpp>

#include <vector>

#include <spatial_index/index.hpp>

// We need unit tests for each kind of tree

// 1. Bare Spheres / Cylinders
// 2. Somas / Segments
// 3. variant<Spheres, Cylinders>
// 4. variant<Somas / Segments>

using namespace spatial_index;


const Point3D centers[]{{0., 0., 0.}, {10., 0., 0.}, {20., 0., 0.}};
const CoordType radius[] = {2., 2.5, 4.};

// for Cylinders
const Point3D centers2[]{{0., 5., 0.}, {10., 5., 0.}, {20., 5., 0.}};

const CoordType tradius = 2.;
const Point3D tcenter0{15., 0., 0.};  // intersecting
const Point3D tcenter1{ 5., 0., 0.};  // non-intersecting
const Point3D tcenter2{ 0.,-3., 0.};  // intersecting sphere only
const Point3D tcenter3{ 0., 6., 0.};  // intersecting cylinder only

constexpr int N_ITEMS = sizeof(radius)/sizeof(CoordType);


template<typename T, typename... Args>
void fill_vec(std::vector<T>& vec, int count, Args... args) {
    vec.reserve(count);
    for(int i = 0; i< count; i++) {
        vec.emplace_back(T{args[i]...});
    }
}

template <unsigned X>
struct all {
    inline unsigned operator[](int) const { return X; }
};

struct identity {
    inline unsigned operator[](int x) const { return unsigned(x); }
};

template <typename T, typename S>
bool test_intesecting_ids(IndexTree<T> const& tree, S const& shape, std::vector<identifier_t> expected) {
    int cur_i = 0;
    for (const auto& item : tree.find_intersecting(shape)) {
        if( cur_i >= expected.size() ) return false;
        if( item->gid() != expected[cur_i] ) {
            std::cout << "Error: " << item->gid() << " != " << expected[cur_i] << std::endl;
            return false;
        }
        cur_i++;
    }
    return true;
}
template <typename S, typename... T>
bool test_intesecting_ids(IndexTree<boost::variant<T...>> const& tree, S const& shape, std::vector<identifier_t> expected) {
    int cur_i = 0;
    for (const auto& item : tree.find_intersecting(shape)) {
        if( cur_i >= expected.size() ) return false;
        identifier_t gid = boost::apply_visitor([](const auto& t){return t.gid();}, *item);
        if( gid != expected[cur_i] ) {
            std::cout << "Error: " << gid << " != " << expected[cur_i] << std::endl;
            return false;
        }
        cur_i++;
    }
    return true;
}


#define TESTS_INTERSECTING_CHECKS(t1_result, t2_result, t3_result, t4_result) \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter0, tradius}) == t1_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter1, tradius}) == t2_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter2, tradius}) == t3_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter3, tradius}) == t4_result );

#define TEST_INTERSECTING_IDS(t1_result, t2_result, t3_result, t4_result) \
    BOOST_TEST( test_intesecting_ids(rtree, Sphere{tcenter0, tradius}, t1_result) ); \
    BOOST_TEST( test_intesecting_ids(rtree, Sphere{tcenter1, tradius}, t2_result) ); \
    BOOST_TEST( test_intesecting_ids(rtree, Sphere{tcenter2, tradius}, t3_result) ); \
    BOOST_TEST( test_intesecting_ids(rtree, Sphere{tcenter3, tradius}, t4_result) );



BOOST_AUTO_TEST_CASE(BasicSphereTree) {
    std::vector<Sphere> spheres;
    fill_vec(spheres, N_ITEMS, centers, radius);

    IndexTree<Sphere> rtree(spheres);

    TESTS_INTERSECTING_CHECKS(true, false, true, false);
}


BOOST_AUTO_TEST_CASE(BasicCylinderTree) {
    std::vector<Cylinder> cyls;
    fill_vec(cyls, N_ITEMS, centers, centers2, radius);

    IndexTree<Cylinder> rtree(cyls);

    TESTS_INTERSECTING_CHECKS(true, false, false, true);
}


BOOST_AUTO_TEST_CASE(SomaTree) {
    std::vector<ISoma> somas;
    fill_vec(somas, N_ITEMS, identity(), centers, radius);

    IndexTree<ISoma> rtree(somas);

    TESTS_INTERSECTING_CHECKS(true, false, true, false);

    TEST_INTERSECTING_IDS({2}, {}, {0}, {});
}


BOOST_AUTO_TEST_CASE(SegmentTree) {
    std::vector<ISegment> segs;
    fill_vec(segs, N_ITEMS, identity(), all<0>(), centers, centers2, radius);

    IndexTree<ISegment> rtree(segs);

    TESTS_INTERSECTING_CHECKS(true, false, false, true);

    TEST_INTERSECTING_IDS({2}, {}, {}, {0});
}


BOOST_AUTO_TEST_CASE(VariantGeometries) {
    std::vector<Sphere> spheres;
    fill_vec(spheres, N_ITEMS, centers, radius);

    IndexTree<GeometryEntry> rtree(spheres);
    rtree.insert(Cylinder{centers[0], centers2[0], radius[0]});

    TESTS_INTERSECTING_CHECKS(true, false, true, true);
}


BOOST_AUTO_TEST_CASE(VariantNeuronPieces) {
    std::vector<ISoma> somas;
    fill_vec(somas, N_ITEMS, identity(), centers, radius);

    IndexTree<MorphoEntry> rtree(somas);
    rtree.insert(ISegment{10ul, 0, centers[0], centers2[0], radius[0]});

    TESTS_INTERSECTING_CHECKS(true, false, true, true);

    TEST_INTERSECTING_IDS({2}, {}, {0}, {10});

    // Extra test... add a segment that spans across all test geometries
    rtree.insert(ISegment{20ul, 0, centers[0], centers[2], 10});

    TESTS_INTERSECTING_CHECKS(true, true, true, true);

    // Macros are dumb when splitting arguments :/
    std::vector<identifier_t> resu[]= { {2, 20}, {20}, {0, 20}, {10, 20} };
    TEST_INTERSECTING_IDS(resu[0], resu[1], resu[2], resu[3]);

}

