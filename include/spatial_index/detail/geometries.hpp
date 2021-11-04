#pragma once

#include "../geometries.hpp"

namespace spatial_index {

namespace detail {

/**
 * \brief get the 3D minimum distance between 2 segments
 * source: http://geomalgorithms.com/a07-_distance.html
 */
inline CoordType distance_segment_segment(Point3D const& s1_0, Point3D const& s1_1,
                                          Point3D const& s2_0, Point3D const& s2_1) {
    const Point3Dx u = Point3Dx(s1_1) - s1_0;
    const Point3Dx v = Point3Dx(s2_1) - s2_0;
    const Point3Dx w = Point3Dx(s1_0) - s2_0;
    const CoordType a = u.dot(u);  // always >= 0
    const CoordType b = u.dot(v);
    const CoordType c = v.dot(v);  // always >= 0
    const CoordType d = u.dot(w);
    const CoordType e = v.dot(w);
    const CoordType D = a * c - b * b;  // always >= 0
    const CoordType EPSILON = 1e-6;

    CoordType sc, sN, sD = D;  // sc = sN / sD, default sD = D >= 0
    CoordType tc, tN, tD = D;  // tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < EPSILON) {  // the lines are almost parallel
        sN = 0.0;       // force using point P0 on segment S1
        sD = 1.0;       // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    } else {  // get the closest points on the infinite lines
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0) {  // sc < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        } else if (sN > sD) {  // sc > 1  => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) {  // tc < 0 => the t=0 edge is visible
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0) {
            sN = 0.0;
        } else if (-d > a) {
            sN = sD;
        } else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {  // tc > 1  => the t=1 edge is visible
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0) {
            sN = 0;
        } else if ((-d + b) > a) {
            sN = sD;
        } else {
            sN = (-d + b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (abs(sN) < EPSILON ? 0.0 : sN / sD);
    tc = (abs(tN) < EPSILON ? 0.0 : tN / tD);

    // get the difference of the two closest points
    Point3Dx dP = w + (sc * u) - (tc * v);  // =  S1(sc) - S2(tc)

    return dP.norm();  // return the closest distance
}

}  // namespace detail


inline bool Sphere::intersects(Cylinder const& c) const {
    // First Assume infinite long cylinder
    Point3Dx u = Point3Dx(centroid) - c.p1;
    Point3Dx v = Point3Dx(c.p2) - c.p1;
    CoordType proj = u.dot(v);
    CoordType distance = std::sqrt(u.dot(u) - (proj * proj / v.dot(v)));
    CoordType radii_sum = radius + c.radius;

    if (distance > radii_sum) {
        return false;
    }
    // Now, check for caps. Calculate sphere distance from it using projections
    // They intersect if the largest is smaller than ||v|| + sphere_radius
    Point3Dx w = Point3Dx(centroid) - c.p2;
    CoordType v_norm = v.norm();
    CoordType max_proj = std::max(std::abs(proj), std::abs(w.dot(v))) / v_norm;
    return max_proj <= v_norm + radius;
}


inline bool Sphere::contains(Point3D const& p) const {
    const auto dist_sq = (Point3Dx(p) - centroid).norm_sq();
    return dist_sq <= radius * radius;
}


inline bool Cylinder::intersects(Cylinder const& c) const {
    CoordType min_dist = detail::distance_segment_segment(p1, p2, c.p1, c.p2);
    return min_dist <= radius + c.radius;
}


inline bool Cylinder::contains(Point3D const& p) const {
    // https://www.flipcode.com/archives/Fast_Point-In-Cylinder_Test.shtml
    const auto& cyl_axis = Point3Dx(p2) - p1;
    const auto& p1_ptest = Point3Dx(p) - p1;
    const auto dot_prod = p1_ptest.dot(cyl_axis);
    const auto axis_len_sq = cyl_axis.norm_sq();

    // Over the caps?
    if (dot_prod < .0f || dot_prod > axis_len_sq) {
        return false;
    }
    // outside radius?
    // three sides triangle: projection on axis, p1_ptest and distance to axis
    const auto dist_sq = p1_ptest.norm_sq() - (dot_prod * dot_prod / axis_len_sq);
    return dist_sq <= radius * radius;
}


// String representation

inline std::ostream& operator<<(std::ostream& os, const Sphere& s) {
    return os << "Sphere(centroid=" << s.centroid << ", "
                 "radius=" << boost::format("%.3g") % s.radius << ')';
}

inline std::ostream& operator<<(std::ostream& os, const Cylinder& c) {
    return os << "Cylinder(centroids=(" << c.p1 << ", " << c.p2 << "), "
                 "radius=" << boost::format("%.3g") % c.radius << ')';
}


}  // namespace spatial_index
