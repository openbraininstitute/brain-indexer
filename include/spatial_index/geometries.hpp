#pragma once

#include "point3d.hpp"


namespace spatial_index {


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

private:
    friend class boost::serialization::access;

    template<class Archive> inline
    void serialize(Archive &ar, const unsigned int version) {
        ar & centroid;
        ar & radius;
    }

};



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

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & p1;
        ar & p2;
        ar & radius;
    }

};


// Generic API for getting intersection among geometries

template <typename T1, typename T2>
bool geometry_intersects(const T1& geom1, const T2& geom2) {
    // Geometries should know how to intersect each other
    return geom1.intersects(geom2);
}



}
