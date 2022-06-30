#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>

#include <random>
#include <vector>
#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

using namespace spatial_index;

BOOST_AUTO_TEST_CASE(ClampTest) {
    BOOST_CHECK(clamp(0.34, 0.4, 0.5) == CoordType(0.4));
    BOOST_CHECK(clamp(0.84, 0.4, 0.5) == CoordType(0.5));
    BOOST_CHECK(clamp(0.42, 0.4, 0.5) == CoordType(0.42));
}


//////////////////////////////////////////////////////////////////
// Project Point onto line/segment
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(PointProjection)
BOOST_AUTO_TEST_CASE(ProjectPointOntoLine) {
    auto base = Point3Dx{CoordType(1.0), CoordType(2.0), CoordType(3.0)};
    auto x = Point3Dx{CoordType(0.125), CoordType(0.25), CoordType(0.5)};

    for(int dim = 0; dim < 3; ++dim) {
        auto dir = Point3Dx{
            CoordType(dim == 0 ? 1.234 : 0.0),
            CoordType(dim == 1 ? 0.324 : 0.0),
            CoordType(dim == 2 ? -1.324 : 0.0)
        };

        auto actual = project_point_onto_line(base, dir, x);

        auto expected = Point3Dx{
            (dim == 0 ? x : base).get<0>(),
            (dim == 1 ? x : base).get<1>(),
            (dim == 2 ? x : base).get<2>()
        };

        auto eps = std::numeric_limits<CoordType>::epsilon();
        auto delta = actual - expected;
        BOOST_CHECK(delta.dot(delta) < 8*eps);
    }
}

BOOST_AUTO_TEST_CASE(ProjectPointOntoSegment) {
    auto p1 = Point3Dx{1.0, 1.0, 0.0};
    auto p2 = Point3Dx{3.0, 3.0, 0.0};
    auto d = p2 - p1;

    auto test_cases = std::vector<std::pair<Point3Dx, Point3Dx>>{
        // Projects onto somewhere near the first thrid.
        { {1.5, 1.5,  0.2345}, {1.5, 1.5, 0.0} },
        { {1.5, 1.5, -3.3245}, {1.5, 1.5, 0.0} },

        // These are far out to either side.
        { {-1.0, -1.5, 23.45}, p1 },
        { { 5.0,  6.0, 23.45}, p2 },

        // Check the end points.
        { {-1.0, -1.0, 23.45}, p1 },
        { { 3.0,  3.0,  0.0 }, p2 }
    };

    for(const auto &tc : test_cases) {
        const auto &expected = tc.second;
        auto actual = project_point_onto_segment(p1, d, tc.first);

        auto eps = std::numeric_limits<CoordType>::epsilon();
        auto delta = actual - expected;
        BOOST_CHECK(delta.dot(delta) < 8*eps);
    }
}
BOOST_AUTO_TEST_SUITE_END()


//////////////////////////////////////////////////////////////////
// Intersection of Sphere and Cylinder
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(SphereCylinderIntersection)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    // These cases are symmetric in the end points of the cylinder.
    auto symmetric_cases = std::vector<std::tuple<Sphere, Cylinder, bool>>{
        // Cylinder inside sphere
        {
            Sphere{{0.0, 0.0, 0.0}, 3.0},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 0.123},
            true
        },

        // Sphere inside cylinder.
        {
            Sphere{{0.0, 0.0, 0.0}, 0.1},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            true
        },

        // Cylinder faces away, just far enough:
        {
            Sphere{{0.0, 0.0, 0.0}, 3.0},
            Cylinder{{ 0.0, CoordType(3.0) + eps, 0.0}, { 0.0, 5.0, 0.0}, 100.0},
            false
        },

        // Cylinder faces away, barely touches:
        {
            Sphere{{0.0, 0.0, 0.0}, 3.0},
            Cylinder{{ 0.0, CoordType(3.0) - eps, 0.0}, { 0.0, 5.0, 0.0}, 100.0},
            true
        },

        // Sphere inside the cap, but not the cylinder.
        {
            Sphere{{1.5, 0.2, 0.0}, CoordType(0.5) - eps},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            false
        },

        // Sphere inside the cap, touches the cylinder.
        {
            Sphere{{1.5, 0.2, 0.0}, CoordType(0.5) + eps},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            true
        },

        // Touches the rim of the cap.
        {
            Sphere{{1.1, 2.0, 1.0}, 1.2},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            true
        },

        // Sphere misses the cylinder from above:
        {
            Sphere{{0.4, 3.0, 0.0}, CoordType(1.0 - eps)},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            false
        },

        // Sphere hits cylinder from straight above:
        //   (3, 4, 5) are the side lengths of a perfect triangle.
        {
            Sphere{{-0.4, 3.0, 4.0}, CoordType(5.0 + eps)},
            Cylinder{{-1.0, 0.0, 0.0}, { 1.0, 0.0, 0.0}, 2.0},
            true
        }
    };

    // TODO Needs C++17 modernization.
    for(const auto &c: symmetric_cases) {
        const auto &sph = std::get<0>(c);
        const auto &cyl = std::get<1>(c);
        auto rcyl = Cylinder{cyl.p2, cyl.p1, cyl.radius};
        auto expected = std::get<2>(c);

        BOOST_CHECK(cyl.intersects(sph) == expected);
        BOOST_CHECK(rcyl.intersects(sph) == expected);
    }

}

BOOST_AUTO_TEST_CASE(NoBoundingBoxOverlap) {
    // To prevent regression: these are hard-coded cases from a
    // failing unit-test. The bounding box of these cylinder don't
    // intersect with the bounding box of the sphere.
    auto cylinders = std::vector<Cylinder>{
        Cylinder{{-2.01, -7.67, -3.78}, {-2.08, -7.76, -3.81}, 0.172 },
        Cylinder{{-5.07,  1.43, -3.31}, {-5.0 ,  1.26, -3.17}, 0.178 },
        Cylinder{{-2.69, -4.94, -7.3 }, {-2.58, -4.96, -7.36}, 0.101 },
        Cylinder{{-3.85,  1.49, -2.63}, {-3.97,  1.3 , -2.53}, 0.0317},
        Cylinder{{-4.02, -4.31, -7.79}, {-4.01, -4.45, -7.6 }, 0.168 }
    };

    auto sphere = Sphere{{-3.0, -3.0, -3.0}, 4.0};

    for(const auto &cylinder: cylinders) {
        BOOST_CHECK(bg::intersects(cylinder.bounding_box(), sphere.bounding_box()) == false);
        BOOST_CHECK(sphere.intersects(cylinder) == false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
