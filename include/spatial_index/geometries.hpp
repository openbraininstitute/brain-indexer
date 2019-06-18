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
        Point3D vec((Point3Dx(p2) - p1).get());
        Point3Dx e = radius + (1.0 - Point3Dx(vec) * vec / bg::dot_product(vec, vec)).sqrt();
        return Box3D(min(p1 - e, p2 - e).get(),
                     max(p1 + e, p2 + e).get());
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

}
