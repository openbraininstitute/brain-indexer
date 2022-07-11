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
// Sphere contain Point
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(SphereContainsPoint)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto test_cases = std::vector<std::tuple<Sphere, Point3D, bool>>{
        {
            Sphere{{0.0, 0.0, 0.0}, 1.0},
            Point3D{0.0, 0.0, 0.0},
            true
        }
    };

    auto register_pair = [&test_cases, eps](const Sphere& s, const Point3D& x, int dim) {
        test_cases.emplace_back(
            s,
            Point3D{
                x.get<0>() + (dim == 0) * eps,
                x.get<1>() + (dim == 1) * eps,
                x.get<2>() + (dim == 2) * eps
            },
            false
        );

        test_cases.emplace_back(
            s,
            Point3D{
                x.get<0>() - (dim == 0) * eps,
                x.get<1>() - (dim == 1) * eps,
                x.get<2>() - (dim == 2) * eps
            },
            true
        );
    };

    register_pair(Sphere{{1.0, 2.0, 3.0}, 3.0}, Point3D{4.0, 2.0, 3.0}, 0);
    register_pair(Sphere{{1.0, 2.0, 3.0}, 3.0}, Point3D{1.0, 5.0, 3.0}, 1);
    register_pair(Sphere{{1.0, 2.0, 3.0}, 3.0}, Point3D{1.0, 2.0, 6.0}, 2);

    for(const auto & tc: test_cases) {
        const auto &s = std::get<0>(tc);
        const auto &x = std::get<1>(tc);
        auto expected = std::get<2>(tc);

        BOOST_CHECK_MESSAGE(
            s.contains(x) == expected,
            s << ", " << Point3Dx(x) << ", " << expected
        );
    }
}
BOOST_AUTO_TEST_SUITE_END()

//////////////////////////////////////////////////////////////////
// Cylinder contains Point
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(CylinderContainsPoint)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto test_cases = std::vector<std::tuple<Cylinder, Point3D, bool>>{};

    auto register_pair = [&test_cases, eps](const Cylinder& c,
                                       const Point3D& x,
                                       int dim) {
        test_cases.emplace_back(
            c,
            Point3D{
                x.get<0>() + (dim == 0) * eps,
                x.get<1>() + (dim == 1) * eps,
                x.get<2>() + (dim == 2) * eps
            },
            false
        );

        test_cases.emplace_back(
            c,
            Point3D{
                x.get<0>() - (dim == 0) * eps,
                x.get<1>() - (dim == 1) * eps,
                x.get<2>() - (dim == 2) * eps
            },
            true
        );
    };

    // Touches center of the cap.
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 2.0},
        Point3D{1.0, 0.0, 0.0},
        0);

    // Touches cap off center.
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 6.0},
        Point3D{1.0, 3.0, 4.0},
        0);

    // Touches round part.
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 5.0},
        Point3D{0.125, 3.0, 4.0},
        1);

    for(const auto & tc: test_cases) {
        const auto& c = std::get<0>(tc);
        const auto& x = std::get<1>(tc);
        auto expected = std::get<2>(tc);

        BOOST_CHECK_MESSAGE(
            c.contains(x) == expected,
            c << ", " << Point3Dx(x) << ", " << expected
        );
    }
}
BOOST_AUTO_TEST_SUITE_END()


//////////////////////////////////////////////////////////////////
// Intersection between spheres
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(SphereSphereIntersection)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto test_cases = std::vector<std::tuple<Sphere, Sphere, bool>>{
        // Sphere on surface of sphere
        {
            Sphere{{0.0, 0.0, 0.0}, 3.0},
            Sphere{{-3.0, 0.0, 0.0}, 0.1},
            true
        },

        // Sphere inside  sphere
        {
            Sphere{{0.0, 0.0, 0.0}, 3.0},
            Sphere{{-1.0, 0.2, 0.3}, 0.1},
            true
        },

        // Sphere outside sphere
        {
            Sphere{{1.0, 0.0, 0.0}, 3.0},
            Sphere{{1.0, 3.0, 4.0}, CoordType(2.0) - eps},
            false
        },

        // Sphere touches sphere
        {
            Sphere{{1.0, 0.0, 0.0}, 3.0},
            Sphere{{1.0, 3.0, 4.0}, CoordType(2.0) + eps},
            true
        }
    };

    for(const auto & tc : test_cases) {
        const auto &s1 = std::get<0>(tc);
        const auto &s2 = std::get<1>(tc);
        auto expected = std::get<2>(tc);

        BOOST_CHECK_MESSAGE(
            s1.intersects(s2) == expected,
            s1 << ", " << s2 << ", " << expected
        );
    }
}

BOOST_AUTO_TEST_SUITE_END()

//////////////////////////////////////////////////////////////////
// Intersection of Sphere and Cylinder
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(SphereCylinderIntersection)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto test_cases = std::vector<std::tuple<Sphere, Cylinder, bool>>{
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
    for(const auto & tc: test_cases) {
        const auto & s = std::get<0>(tc);
        const auto & c = std::get<1>(tc);
        auto rc = Cylinder{c.p2, c.p1, c.radius};
        auto expected = std::get<2>(tc);

        BOOST_CHECK_MESSAGE(
            c.intersects(s) == expected,
            c << ", " << s << ", " << expected
        );
        BOOST_CHECK_MESSAGE(
            rc.intersects(s) == expected,
            rc << ", " << s << ", " << expected
        );
    }

}

BOOST_AUTO_TEST_CASE(NoBoundingBoxOverlap) {
    // To prevent regression: these are hard-coded test_cases from a
    // failing unit-test. The bounding box of these cylinders don't
    // intersect with the bounding box of the sphere.
    auto cylinders = std::vector<Cylinder>{
        Cylinder{{-2.01, -7.67, -3.78}, {-2.08, -7.76, -3.81}, 0.172 },
        Cylinder{{-5.07,  1.43, -3.31}, {-5.0 ,  1.26, -3.17}, 0.178 },
        Cylinder{{-2.69, -4.94, -7.3 }, {-2.58, -4.96, -7.36}, 0.101 },
        Cylinder{{-3.85,  1.49, -2.63}, {-3.97,  1.3 , -2.53}, 0.0317},
        Cylinder{{-4.02, -4.31, -7.79}, {-4.01, -4.45, -7.6 }, 0.168 }
    };

    auto s = Sphere{{-3.0, -3.0, -3.0}, 4.0};

    for(const auto & c: cylinders) {
        BOOST_CHECK(bg::intersects(c.bounding_box(), s.bounding_box()) == false);
        BOOST_CHECK(s.intersects(c) == false);
    }
}

BOOST_AUTO_TEST_SUITE_END()

//////////////////////////////////////////////////////////////////
// Intersection between Capsules
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(CapsuleCapsuleIntersection)

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto check = [](const Cylinder &c1, const Cylinder &c2, bool expected) {
        BOOST_CHECK_MESSAGE(
            c1.intersects(c2) == expected,
            c1 << ", " << c2 << ", " << expected
        );
    };

    auto test_cases = std::vector<std::tuple<Cylinder, Cylinder, bool>>{
        // Thin longer cylinder inside a cylinder with larger radius, axis aligned.
        {
            Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 2.0},
            Cylinder{{-3.0, 0.0, 0.0}, {3.0, 0.0, 0.0}, 1.0},
            true
        },

        // Thin cylinder completely contained, axis aligned.
        {
            Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 2.0},
            Cylinder{{-0.5, 0.1, 0.2}, {0.5, 0.1, 0.2}, 1.0},
            true
        },

        // Cylinder inside cap
        {
            Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 2.0},
            Cylinder{{1.2, 0.1, 0.1}, {1.3, -0.2, 0.1}, 0.1},
            true
        }
    };

    // Given two cylinders that have a non-empty, but zero volume
    // intersection (i.e. they 'touch') one can build a pair of test
    // cases by in- and de-flating the radius of one by a little
    // to either get a non-degenerate or an empty intersection.
    auto register_pair = [&test_cases, eps](const Cylinder &c1, const Cylinder &c2) {
        test_cases.emplace_back(c1, Cylinder{c2.p1, c2.p2, c2.radius + eps}, true);
        test_cases.emplace_back(c1, Cylinder{c2.p1, c2.p2, c2.radius - eps}, false);
    };


    // Useful perfect triangles:
    //  (3, 4,  5): 3^2 + 4^2 == 5^2
    //  (6, 8, 10): 2x the above

    // Axis aligned tubes, round/round intersection: both cases
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 2.0},
        Cylinder{{-0.5, 3.0, 4.0}, {0.5, 3.0, 4.0}, 3.0}
    );

    // Cylinder facing away from round part, round/cap intersection: both cases
    //   - axis are perpendicular
    register_pair(
        Cylinder{{-0.125, 3.0, 4.0}, {-0.125, 6.0, 8.0}, 1.0},
        Cylinder{{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 4.0}
    );

    // Again round/cap, but this time at an angle.
    // These will touch the cap at (3, 4, 0). The axis (-4, 3, 0) is
    // perpendicular to the cap at the point, therefore:
    //   (6, 8, 0) is at distance 10 from the origin; and
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 5.0},
        Cylinder{
            Point3Dx{6.0, 8.0, 0.0} - Point3Dx{-4.0, 3.0, 0.0},
            Point3Dx{6.0, 8.0, 0.0} + Point3Dx{-4.0, 3.0, 0.0},
            5.0
        }
    );

    // Cap/cap, similar to previous setup, but rotated.
    register_pair(
        Cylinder{{-1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 5.0},
        Cylinder{{6.0, 8.0, 0.0}, {12.0, 16.0, 0.0}, 5.0}
    );

    // Cap/cap, fully aligned.
    register_pair(
        Cylinder{{-6.0, 0.0, 0.0}, {-3.0, 0.0, 0.0}, 3.0},
        Cylinder{{ 6.0, 0.0, 0.0}, { 3.0, 0.0, 0.0}, 3.0}
    );


    for(const auto & tc: test_cases) {
        const auto &c1 = std::get<0>(tc);
        const auto &c2 = std::get<1>(tc);
        auto expected = std::get<2>(tc);

        auto rc1 = Cylinder{c1.p2, c1.p1, c1.radius};
        auto rc2 = Cylinder{c2.p2, c2.p1, c2.radius};

        check( c1,  c2, expected);
        check(rc1,  c2, expected);
        check( c1, rc2, expected);
        check(rc1, rc2, expected);
    }
}