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
const Point3D tcenter1{5., 0., 0.};   // non-intersecting
const Point3D tcenter2{0., -3., 0.};  // intersecting sphere only
const Point3D tcenter3{0., 6., 0.};   // intersecting cylinder only




template<typename T, typename... Args>
void fill_vec(std::vector<T>& vec, int count, Args... args) {
    vec.reserve(count);
    for(int i = 0; i< count; i++) {
        vec.emplace_back(T{args[i]...});
    }
}


#define TESTS_INTERSECTING_CHECKS(t1_result, t2_result, t3_result, t4_result) \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter0, tradius}) == t1_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter1, tradius}) == t2_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter2, tradius}) == t3_result ); \
    BOOST_TEST( rtree.is_intersecting(Sphere{tcenter3, tradius}) == t4_result );



BOOST_AUTO_TEST_CASE(BasicSphereTree) {
    std::vector<Sphere> spheres;
    fill_vec(spheres, sizeof(radius)/sizeof(CoordType), centers, radius);

    IndexTree<Sphere> rtree(spheres);

    TESTS_INTERSECTING_CHECKS(true, false, true, false);
}


BOOST_AUTO_TEST_CASE(BasicCylinderTree) {
    std::vector<Cylinder> cyls;
    fill_vec(cyls, sizeof(radius)/sizeof(CoordType), centers, centers2, radius);

    IndexTree<Cylinder> rtree(cyls);

    TESTS_INTERSECTING_CHECKS(true, false, false, true);
}
