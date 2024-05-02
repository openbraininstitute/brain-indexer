#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>

#include <random>
#include <vector>
#include <brain_indexer/index.hpp>
#include <brain_indexer/util.hpp>

using namespace brain_indexer;

BOOST_AUTO_TEST_CASE(ClampTest) {
    BOOST_CHECK(clamp(0.34, 0.4, 0.5) == CoordType(0.4));
    BOOST_CHECK(clamp(0.84, 0.4, 0.5) == CoordType(0.5));
    BOOST_CHECK(clamp(0.42, 0.4, 0.5) == CoordType(0.42));

    auto actual = clamp(
        Point3D{0.213, 0.5239, 0.789},
        Point3D{0.2,   0.3,    0.8},
        Point3D{0.3,   0.5,    0.9}
    );
    auto expected = Point3Dx{0.213, 0.5, 0.8};

    BOOST_CHECK(actual == expected);
}

BOOST_AUTO_TEST_CASE(Point3DtoFloatConversionTest) {
    auto points = std::vector<Point3D>{{1.0, 42.235, 96.813}, {420.874, 21.0, 25.8069}};
    auto ptr = (CoordType*) points.data();

    BOOST_REQUIRE(sizeof(Point3D) == 3*sizeof(CoordType));
    for(size_t i = 0; i < 2; ++i) {
        BOOST_REQUIRE(points[i].get<0>() == ptr[0 + 3*i]);
        BOOST_REQUIRE(points[i].get<1>() == ptr[1 + 3*i]);
        BOOST_REQUIRE(points[i].get<2>() == ptr[2 + 3*i]);
    }
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

    for(const auto& [x, expected] : test_cases) {
        auto actual = project_point_onto_segment(p1, d, x);

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

    for(const auto& [s, x, expected]: test_cases) {
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

    for(const auto& [c, x, expected] : test_cases) {
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

    for(const auto& [s1, s2, expected] : test_cases) {
        BOOST_CHECK_MESSAGE(
            s1.intersects(s2) == expected,
            s1 << ", " << s2 << ", " << expected
        );
    }
}
BOOST_AUTO_TEST_SUITE_END()


//////////////////////////////////////////////////////////////////
// Intersection of Sphere and Box
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(SphereBoxIntersection)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto generic_box = Box3D{{-1.0, -0.5, -4.0}, {2.0, 3.0, 4.0}};

    auto test_cases = std::vector<std::tuple<Box3D, Sphere, bool>>{
        // Tiny sphere inside the box
        {
            generic_box,
            Sphere{{0.1, 0.2, 0.3}, 0.01},
            true
        },

        // Sphere far away
        {
            generic_box,
            Sphere{{100.0, 0.2, 0.3}, 1.0},
            false
        }

    };

    auto register_pair = [&test_cases, eps](const Box3D& b, const Sphere& s) {
        test_cases.emplace_back(
            b,
            Sphere{s.centroid, s.radius + eps},
            true
        );

        test_cases.emplace_back(
            b,
            Sphere{s.centroid, s.radius - eps},
            false
        );
    };

    // Sphere at right, lower, front corner.
    register_pair(generic_box, Sphere{{5.0, -4.5, -4.0}, 5.0});

    // Sphere at lower side of the box
    register_pair(generic_box, Sphere{{0.0, 1.0, -6.0}, 2.0});

    // Sphere at front, lower edge of the box
    register_pair(generic_box, Sphere{{0.5, -3.5, -8.0}, 5.0});

    for(const auto& [b, s, expected]: test_cases) {
        BOOST_CHECK(s.intersects(b) == expected);
        BOOST_CHECK(Box3Dx{b}.intersects(s) == expected);
    }
}
BOOST_AUTO_TEST_SUITE_END()

//////////////////////////////////////////////////////////////////
// Intersection of Sphere and Cylinder
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(BoxCylinderIntersection)
BOOST_AUTO_TEST_CASE(SelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();
    auto generic_box = Box3D{{-1.0, -0.5, -4.0}, {2.0, 3.0, 4.0}};

    auto test_cases = std::vector<std::tuple<Box3D, Cylinder, bool>>{};

    // ------------------------------------------------------------------------
    // Checks that thin cylinders are detected:

    // The base is outside the box and dir point towards the box, such that
    // `base + dir` lands inside the box.
    auto register_stick_triples = [&test_cases, &generic_box]
        (const Point3Dx& base, const Point3Dx &dir) {

        test_cases.emplace_back(
            generic_box,
            Cylinder{base, base + dir, 0.001},
            true
        );

        test_cases.emplace_back(
            generic_box,
            Cylinder{base, base + 2*dir, 0.001},
            true
        );

        test_cases.emplace_back(
            generic_box,
            Cylinder{base, base - dir, 0.001},
            false
        );
    };

    // Axis aligned.
    register_stick_triples(Point3Dx{-10.0, 0.0, 0.0}, Point3Dx{10.0, 0.0, 0.0});
    register_stick_triples(Point3Dx{0.5, 10.0, 2.0}, Point3Dx{0.0, -8.0, 0.0});
    register_stick_triples(Point3Dx{0.5, 2.0, -10.0}, Point3Dx{0.0, 0.0, 6.0});

    // At an angle.
    register_stick_triples(Point3Dx{1.0, -1.0, 1.0}, Point3Dx{-1.25, 1.0, 0.0});
    register_stick_triples(Point3Dx{1.0, -1.0, 1.0}, Point3Dx{0.5, 3.0, 2.0});

    // ------------------------------------------------------------------------
    // Checks when the extreme points of the cylinder are closest.

    auto register_pair = [&test_cases, &generic_box, eps]
        (const Point3Dx& p1, const Point3Dx &p2, CoordType radius) {

        test_cases.emplace_back(generic_box, Cylinder{p1, p2, radius + eps}, true);
        test_cases.emplace_back(generic_box, Cylinder{p1, p2, radius - eps}, false);
    };

//    auto generic_box = Box3D{{-1.0, -0.5, -4.0}, {2.0, 3.0, 4.0}};
    // touches right side.
    register_pair({5, 1, 2}, {7, -1, 3}, 3.0);

    // toches right, top, front corner.
    register_pair({5, 7, -4}, {9, 7.5, -4.5}, 5.0);

    // toches left, top edge
    auto cyl_center = Point3Dx{-4, 7, -2};
    auto dir = Point3Dx{4.0, 3.0, -1.0};
    register_pair(cyl_center - dir, cyl_center + dir, 5.0);

    std::cout << detail::distance_segment_segment(
        {-1.0, 3.0, -4.0},
        {-1.0, 3.0,  4.0},
        cyl_center - dir,
        cyl_center + dir
    ) << std::endl;

    for(const auto& [b, c, expected]: test_cases) {
        // TODO update with message once `operator<<(const Box3D &)`
        // has been merged.
        BOOST_CHECK(c.intersects(b) == expected);
        BOOST_CHECK(Box3Dx{b}.intersects(c) == expected);
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

    for(const auto& [s, c, expected]: test_cases) {
        auto rc = Cylinder{c.p2, c.p1, c.radius};

        BOOST_CHECK_MESSAGE(
            c.intersects(s) == expected,
            c << ", " << s << ", " << expected
        );

        BOOST_CHECK_MESSAGE(
            rc.intersects(s) == expected,
            rc << ", " << s << ", " << expected
        );

        BOOST_CHECK_MESSAGE(
            s.intersects(c) == expected,
            s << ", " << c << ", " << expected
        );

        BOOST_CHECK_MESSAGE(
            s.intersects(rc) == expected,
            s << ", " << rc << ", " << expected
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

    for(const auto& c : cylinders) {
        BOOST_CHECK(bg::intersects(c.bounding_box(), s.bounding_box()) == false);
        BOOST_CHECK(s.intersects(c) == false);
        BOOST_CHECK(c.intersects(s) == false);
    }
}
BOOST_AUTO_TEST_SUITE_END()

//////////////////////////////////////////////////////////////////
// Intersection between Capsules
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(CapsuleCapsuleIntersection)
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


    for(auto [c1, c2, expected] : test_cases) {
        auto rc1 = Cylinder{c1.p2, c1.p1, c1.radius};
        auto rc2 = Cylinder{c2.p2, c2.p1, c2.radius};

        check( c1,  c2, expected);
        check(rc1,  c2, expected);
        check( c1, rc2, expected);
        check(rc1, rc2, expected);
    }
}
BOOST_AUTO_TEST_SUITE_END()


//////////////////////////////////////////////////////////////////
// Bounding Boxes
//////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_SUITE(BoundingBoxChecks)
BOOST_AUTO_TEST_CASE(CylinderSelectedCases) {
    auto eps = CoordType(1e3) * std::numeric_limits<CoordType>::epsilon();

    auto test_cases = std::vector<std::tuple<Cylinder, Box3D, double>>{};

    // Flips the endpoints of the Cylinder.
    auto register_pair = [&test_cases](const Cylinder &c, const Box3D&bb, double atol) {
        test_cases.emplace_back(c, bb, atol);
        test_cases.emplace_back(Cylinder{c.p2, c.p1, c.radius}, bb, atol);
    };

    auto a = Point3Dx{0.3, -0.1, 0.9};
    auto b = Point3Dx{0.12, 0.34, 0.8};

    auto eps_e0 = Point3Dx{std::numeric_limits<CoordType>::epsilon(), 0.0, 0.0};
    auto a_eps_e0 = a + eps_e0;

    auto eps_e1 = Point3Dx{0.0, std::numeric_limits<CoordType>::epsilon(), 0.0};
    auto a_eps_e1 = a + eps_e1;

    auto eps_e2 = Point3Dx{0.0, 0.0, std::numeric_limits<CoordType>::epsilon()};
    auto a_eps_e2 = a + eps_e2;

    CoordType radius = 0.1;

    register_pair(
        Cylinder{a, b, radius},
        Box3D{
            Point3Dx(min(a, b)) - CoordType(0.5*radius),
            Point3Dx(max(a, b)) + CoordType(0.5*radius)
        },
        (1.0 + eps) * radius
    );

    register_pair(
        Cylinder{a, b, 0.0},
        Box3D{min(a, b), max(a, b)},
        eps
    );

    register_pair(
        Cylinder{a, a, radius},
        Box3D{
            Point3Dx(min(a, a)) - CoordType(0.5*radius),
            Point3Dx(max(a, a)) + CoordType(0.5*radius)
        },
        (1.0 + eps) * radius
    );

    register_pair(
        Cylinder{a, a, 0.0},
        Box3D{a, a},
        eps
    );

    register_pair(
        Cylinder{a, a_eps_e0, radius},
        Box3D{a - radius, a + radius},
        eps
    );
    BOOST_CHECK((a_eps_e0 - a).maximum() > 0);

    register_pair(
        Cylinder{a, a_eps_e1, radius},
        Box3D{a - radius, a + radius},
        eps
    );
    BOOST_CHECK((a_eps_e0 - a).maximum() > 0);

    register_pair(
        Cylinder{a, a_eps_e2, radius},
        Box3D{a - radius, a + radius},
        eps
    );
    BOOST_CHECK((a_eps_e0 - a).maximum() > 0);

    // This time we assume that the radius is tiny compared
    // absolute position of `p1` and `p2`; but again `p1`
    // and `p2` are virtually the same.
    CoordType alpha = 1e9;
    register_pair(
        Cylinder{alpha*a, alpha*a_eps_e0, radius},
        Box3D{alpha*a - radius, alpha*a + radius},
        eps*alpha
    );
    BOOST_CHECK(((alpha*a_eps_e0) - (alpha*a)).maximum() > 0);

    register_pair(
        Cylinder{alpha*a, alpha*a_eps_e1, radius},
        Box3D{alpha*a - radius, alpha*a + radius},
        eps*alpha
    );
    BOOST_CHECK(((alpha*a_eps_e1) - (alpha*a)).maximum() > 0);

    register_pair(
        Cylinder{alpha*a, alpha*a_eps_e2, radius},
        Box3D{alpha*a - radius, alpha*a + radius},
        eps*alpha
    );
    BOOST_CHECK(((alpha*a_eps_e1) - (alpha*a)).maximum() > 0);


    auto almost_equal = [](const Point3D& a, const Point3D& b, double atol) {
        return std::abs(a.get<0>() - b.get<0>()) < atol &&
               std::abs(a.get<1>() - b.get<1>()) < atol &&
               std::abs(a.get<2>() - b.get<2>()) < atol;
    };

    for(const auto& [c, bb_expected, atol] : test_cases) {
        auto bb_actual = c.bounding_box();

        BOOST_CHECK_MESSAGE(
            almost_equal(bb_expected.min_corner(), bb_actual.min_corner(), atol),
            "min: expected = " << bb_expected.min_corner()
                << ", actual = " << bb_actual.min_corner()
                << ", atol = " << atol
        );

        BOOST_CHECK_MESSAGE(
            almost_equal(bb_expected.max_corner(), bb_actual.max_corner(), atol),
            "max: expected = " << bb_expected.max_corner()
                << ", actual = " << bb_actual.max_corner()
                << ", atol = " << atol
        );
    }
}
BOOST_AUTO_TEST_SUITE_END()
