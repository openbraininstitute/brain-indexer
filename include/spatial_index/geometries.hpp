#pragma once

#include "point3d.hpp"


namespace spatial_index {


struct Cylinder;  // FWDecl


/**
 * \brief A Sphere represention. Base abstraction for somas
 *
 * For compat with indexing Geometries must implement bounding_box() and intersects()
 */
struct Sphere
{
    Point3D centroid;
    CoordType radius;

    inline Box3D bounding_box() const {
        // copy them
        Point3D min_corner(centroid);
        Point3D max_corner(centroid);

        bg::add_value(max_corner, radius);
        bg::subtract_value(min_corner, radius);

        return Box3D(min_corner, max_corner);
    }


    inline bool intersects(Sphere const& other) const {
        CoordType radii_sum = radius + other.radius;
        return (radii_sum * radii_sum) > Point3Dx(centroid).dist_sq(other.centroid);
    }

    inline bool intersects(Cylinder const& c) const;

    inline void translate(Point3D const& vec) {
        bg::add_point(centroid, vec);
    };

private:
    friend class boost::serialization::access;

    template<class Archive> inline
    void serialize(Archive &ar, const unsigned int version) {
        ar & centroid;
        ar & radius;
    }

};


/**
 * \brief A Cylinder represention. Base abstraction for Segments
 */
struct Cylinder
{
    Point3D p1, p2;
    CoordType radius;

    inline CoordType length() const {
        return bg::distance(p1, p2);
    }

    inline Box3D bounding_box() const
    {
        Point3Dx vec{Point3Dx(p2) - p1};
        Point3Dx x = vec * vec / vec.dot(vec);
        Point3Dx e = radius * (1.0 - x).sqrt();
        return Box3D(min(p1 - e, p2 - e),
                     max(p1 + e, p2 + e));
    }

    inline bool intersects(Sphere const& s) const {
        return s.intersects(*this);  //delegate to sphere
    }

    inline void translate(Point3D const& vec) {
        bg::add_point(p1, vec);
        bg::add_point(p2, vec);
    };

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & p1;
        ar & p2;
        ar & radius;
    }

};


bool Sphere::intersects(Cylinder const& c) const {
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
    return max_proj < v_norm + radius;
}


// Generic API for getting intersection among geometries

template <typename T1, typename T2>
bool geometry_intersects(const T1& geom1, const T2& geom2) {
    // Geometries should know how to intersect each other
    return geom1.intersects(geom2);
}



}
