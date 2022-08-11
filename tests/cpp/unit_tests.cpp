#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>

#include <random>
#include <vector>
#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

// We need unit tests for each kind of tree

// 1. Bare Spheres / Cylinders
// 2. Somas / Segments
// 3. Indexed Sphere
// 4. variant<Spheres, Cylinders>
// 5. variant<Somas / Segments>

using namespace spatial_index;


const Point3D centers[]{{0., 0., 0.}, {10., 0., 0.}, {20., 0., 0.}};
const CoordType radius[] = {2., 2.5, 4.};
const identifier_t post_gids[] = {1, 2, 2};
const identifier_t pre_gids[] = {0, 1, 1};

// for Cylinders
const Point3D centers2[]{{0., 5., 0.}, {10., 5., 0.}, {20., 5., 0.}};

const CoordType tradius = 2.;
const Point3D tcenter0{15., 0., 0.};  // intersecting
const Point3D tcenter1{ 5., 0., 0.};  // non-intersecting
const Point3D tcenter2{ 0.,-3., 0.};  // intersecting sphere only
const Point3D tcenter3{ 0., 6., 0.};  // intersecting cylinder only

constexpr int N_ITEMS = sizeof(radius) / sizeof(CoordType);


template <typename T, typename S>
bool test_intersecting_ids(IndexTree<T> const& tree,
                           S const& shape,
                           std::vector<identifier_t> expected) {
    size_t cur_i = 0;
    for (const T& item : tree.find_intersecting_objs(shape)) {
        if (cur_i >= expected.size())
            return false;
        identifier_t id = detail::get_id_from(item);
        if (id != expected[cur_i]) {
            std::cout << "Error: " << id << " != " << expected[cur_i] << std::endl;
            return false;
        }
        cur_i++;
    }
    return true;
}


#define TESTS_INTERSECTING_CHECKS(t1_result, t2_result, t3_result, t4_result)  \
    BOOST_TEST(rtree.is_intersecting(Sphere{tcenter0, tradius}) == t1_result); \
    BOOST_TEST(rtree.is_intersecting(Sphere{tcenter1, tradius}) == t2_result); \
    BOOST_TEST(rtree.is_intersecting(Sphere{tcenter2, tradius}) == t3_result); \
    BOOST_TEST(rtree.is_intersecting(Sphere{tcenter3, tradius}) == t4_result)

#define TEST_INTERSECTING_IDS(t1_result, t2_result, t3_result, t4_result)          \
    BOOST_TEST(test_intersecting_ids(rtree, Sphere{tcenter0, tradius}, t1_result)); \
    BOOST_TEST(test_intersecting_ids(rtree, Sphere{tcenter1, tradius}, t2_result)); \
    BOOST_TEST(test_intersecting_ids(rtree, Sphere{tcenter2, tradius}, t3_result)); \
    BOOST_TEST(test_intersecting_ids(rtree, Sphere{tcenter3, tradius}, t4_result))


BOOST_AUTO_TEST_CASE(BasicSphereTree) {
    auto spheres = util::make_vec<Sphere>(N_ITEMS, centers, radius);
    IndexTree<Sphere> rtree(spheres);

    TESTS_INTERSECTING_CHECKS(true, false, true, false);
}


BOOST_AUTO_TEST_CASE(BasicCylinderTree) {
    auto cyls = util::make_vec<Cylinder>(N_ITEMS, centers, centers2, radius);
    IndexTree<Cylinder> rtree(cyls);

    TESTS_INTERSECTING_CHECKS(true, false, false, true);
}


BOOST_AUTO_TEST_CASE(IndexedSphereTree) {
    auto spheres = util::make_vec<IndexedSphere>(N_ITEMS, util::identity<>(), centers, radius);
    IndexTree<IndexedSphere> rtree(spheres);

    TESTS_INTERSECTING_CHECKS(true, false, true, false);
    TEST_INTERSECTING_IDS({2}, {}, {0}, {});

    // Dump and load
    rtree.dump("sphere_index");
    IndexTree<IndexedSphere> rtree_loaded("sphere_index");
    BOOST_CHECK(rtree.all_ids() == rtree_loaded.all_ids());

}


BOOST_AUTO_TEST_CASE(SynapseTree) {
    auto synapses = util::make_vec<Synapse>(N_ITEMS, util::identity<>(), post_gids, pre_gids, centers);
    IndexTree<Synapse> rtree(synapses);

    auto n_elems_within = rtree.count_intersecting(Box3D{{-1., -1., -1.}, {11., 1., 1.}});
    BOOST_CHECK_EQUAL(n_elems_within, 2);

    auto aggregated_per_gid = rtree.count_intersecting_agg_gid(Box3D{{-1., -1., -1.}, {11., 1., 1.}});
    BOOST_CHECK(aggregated_per_gid[1] == 1);
    BOOST_CHECK(aggregated_per_gid[2] == 1);

    auto aggregated_per_gid_round2 =
            rtree.count_intersecting_agg_gid(Box3D{{-1., -1., -1.}, {21., 1., 1.}});
    BOOST_CHECK(aggregated_per_gid_round2[1] == 1);
    BOOST_CHECK(aggregated_per_gid_round2[2] == 2);
}


BOOST_AUTO_TEST_CASE(SegmentTree) {
    auto segs = util::make_vec<Segment>(N_ITEMS, util::identity<>(), util::constant<unsigned>(0), util::constant<unsigned>(0),
                                        centers, centers2, radius);
    IndexTree<Segment> rtree(segs);

    TESTS_INTERSECTING_CHECKS(true, false, false, true);
    TEST_INTERSECTING_IDS({2}, {}, {}, {0});
}


BOOST_AUTO_TEST_CASE(VariantGeometries) {
    auto spheres = util::make_vec<Sphere>(N_ITEMS, centers, radius);
    IndexTree<GeometryEntry> rtree(spheres);
    rtree.insert(Cylinder{centers[0], centers2[0], radius[0]});

    TESTS_INTERSECTING_CHECKS(true, false, true, true);
}


BOOST_AUTO_TEST_CASE(VariantNeuronPieces) {
    auto somas = util::make_vec<Soma>(N_ITEMS, util::identity<>(), centers, radius);

    IndexTree<MorphoEntry> rtree(somas);
    rtree.insert(Segment{10ul, 0u, 0u, centers[0], centers2[0], radius[0]});

    TESTS_INTERSECTING_CHECKS(true, false, true, true);
    TEST_INTERSECTING_IDS({2}, {}, {0}, {10});

    // Extra test... add a segment that spans across all test geometries
    rtree.insert(Segment{20ul, 0u, 0u, centers[0], centers[2], 10.0f});

    TESTS_INTERSECTING_CHECKS(true, true, true, true);

    // Macros are dumb when splitting arguments :/
    std::vector<identifier_t> resu[] = {{2, 20}, {20}, {0, 20}, {10, 20}};
    TEST_INTERSECTING_IDS(resu[0], resu[1], resu[2], resu[3]);
}


//////////////////////////////////////////////////////////////////
// Advanced features
//////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(NonOverlapPlacement) {
    auto spheres = util::make_vec<Sphere>(N_ITEMS, centers, radius);

    IndexTree<Sphere> rtree(spheres);
    Sphere toplace{{0., 0., 0.}, 2.};

    BOOST_TEST(rtree.place(Box3D{{.0, .0, -2.}, {20., 5., 2.}}, toplace));
    BOOST_TEST(toplace.centroid.get<0>() > 1.0);

    // Next one will be even further
    Sphere toplace2{{0., 0., 0.}, 2.};
    BOOST_TEST(rtree.place(Box3D{{.0, .0, -2.}, {20., 5., 2.}}, toplace2));
    BOOST_CHECK(toplace2.centroid.get<0>() > toplace.centroid.get<0>());
}

BOOST_AUTO_TEST_CASE(IntegerConversion) {
    // Too small.
    BOOST_CHECK_THROW(util::safe_integer_cast<size_t>(-1),
        std::bad_cast
    );

    // Too large.
    BOOST_CHECK_THROW(
        util::safe_integer_cast<int>(std::numeric_limits<size_t>::max()),
        std::bad_cast
    );

    // Just right.
    BOOST_CHECK(42 == util::safe_integer_cast<int>(42ul));

#ifndef NDEBUG
    BOOST_CHECK_THROW(util::integer_cast<int>(size_t(-1)), std::bad_cast);
    BOOST_CHECK_THROW(util::integer_cast<size_t>(-1), std::bad_cast);
#else
    // In "fast mode" `interger_cast` must not check anything, and just
    // perform the C-style cast (and over- or underflow).
    BOOST_CHECK(util::integer_cast<int>(size_t(-1)) == -1);
    BOOST_CHECK(util::integer_cast<size_t>(-1) == size_t(-1));
#endif
}


//////////////////////////////////////////////////////////////////
// Internal assumptions
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(IdentifierIs64BitAndUnsigned) {
    // The code contains bit magic to pack the GID, section & segment id
    // all into a single 64 bit unsigned integer.
    BOOST_CHECK(sizeof(identifier_t) == 8);
    BOOST_CHECK(!std::is_signed_v<identifier_t>);
}

BOOST_AUTO_TEST_CASE(ReciproceOfSmallestFloat) {

    auto float_eps = std::numeric_limits<float>::min();
    auto double_eps = std::numeric_limits<double>::min();

    BOOST_CHECK(std::isfinite(1.0f/float_eps));
    BOOST_CHECK(std::isfinite(1.0/double_eps));
}
