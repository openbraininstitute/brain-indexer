#pragma once

#include <boost/serialization/serialization.hpp>

#include "point3d.hpp"


namespace spatial_index {


struct Cylinder;  // FWDecl
struct Box3Dx;  // FWDecl


/**
 * \brief A Sphere represention. Base abstraction for somas
 *
 * For compat with indexing Geometries must implement bounding_box() and intersects()
 */
struct Sphere {
    using box_type = Box3D;

    Point3D centroid;
    CoordType radius;

    inline Box3D bounding_box() const {
        return Box3D(Point3Dx(centroid) - radius, Point3Dx(centroid) + radius);
    }

    inline bool intersects(Sphere const& other) const {
        CoordType radii_sum = radius + other.radius;
        return (radii_sum * radii_sum) >= Point3Dx(centroid).dist_sq(other.centroid);
    }

    inline bool intersects(Cylinder const& c) const;
    inline bool intersects(Box3Dx const& b) const;

    inline bool contains(Point3D const& p) const;

    inline void translate(Point3D const& vec) {
        bg::add_point(centroid, vec);
    }

    inline const Point3D& get_centroid() const noexcept {
        return centroid;
    }

    template<size_t dim>
    inline CoordType get_centroid_coord() const noexcept {
        return centroid.get<dim>();
    }

  private:
    friend class boost::serialization::access;

    template <class Archive>
    inline void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & centroid;
        ar & radius;
    }
};

// Poorly named.
struct Box3Dx : public Box3D {
    using box_type = Box3D;

    using Box3D::Box3D;
    Box3Dx(const Box3D &b) : Box3D(b) {}

    inline const Box3D& bounding_box() const {
        return static_cast<const Box3D&>(*this);
    }

    inline Point3D get_centroid() const noexcept {
        return Point3D{
            get_centroid_coord<0>(),
            get_centroid_coord<1>(),
            get_centroid_coord<2>()
        };
    }

    template<size_t dim>
    inline CoordType get_centroid_coord() const noexcept {
        return (min_corner().get<dim>() + max_corner().get<dim>())/2;
    }

    inline bool intersects(Box3D const& other) const {
        return bg::intersects(bounding_box(), other);
    }

    inline bool intersects(Sphere const& s) const {
        // FIXME this isn't accurate, but the only usecase, allows this.
        return bg::intersects(bounding_box(), s.bounding_box());
    }

    inline bool intersects(Cylinder const& c) const;

  private:
    friend class boost::serialization::access;

    template <class Archive>
    inline void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & static_cast<Box3D&>(*this);
    }
};



/**
 * \brief A Cylinder represention. Base abstraction for Segments
 */
struct Cylinder {
    using box_type = Box3D;

    Point3D p1, p2;
    CoordType radius;

    inline CoordType length() const {
        return static_cast<CoordType>(bg::distance(p1, p2));
    }

    inline Box3D bounding_box() const {
        Point3Dx vec{Point3Dx(p2) - p1};
        Point3Dx x = vec * vec / vec.dot(vec);
        Point3Dx e = (1.0 - x).sqrt() * radius;
        return Box3D(min(p1 - e, p2 - e), max(p1 + e, p2 + e));
    }

    /**
     * \brief Approximately checks whether a cylinder intersects another cylinder.
     *
     *  Note: For performance and simplicity reasons, detection considers cylinders as
     *    capsules, and therefore they have "bumped" caps. As long as the length / radius
     *    ratio is high, or we have lines of segments, the approximation works sufficently well.
     */
    inline bool intersects(Cylinder const& c) const;

    inline bool intersects(Sphere const& s) const {
        return s.intersects(*this);  // delegate to sphere
    }

    inline bool intersects(Box3Dx const& b) const {
        return b.intersects(*this);  // delegate to EnhancedBox
    }

    inline bool contains(Point3D const& p) const;

    inline void translate(Point3D const& vec) {
        bg::add_point(p1, vec);
        bg::add_point(p2, vec);
    }

    inline Point3D get_centroid() const {
        Point3D centroid;
        centroid.set<0>(get_centroid_coord<0>());
        centroid.set<1>(get_centroid_coord<1>());
        centroid.set<2>(get_centroid_coord<2>());
        return centroid;
    }

    template<size_t dim>
    inline CoordType get_centroid_coord() const noexcept {
        return (p1.get<dim>() + p2.get<dim>()) / 2;
    }

  private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & p1;
        ar & p2;
        ar & radius;
    }
};


// Generic API for getting intersection among geometries

template <typename T1, typename T2>
inline bool geometry_intersects(const T1& geom1, const T2& geom2) {
    // Geometries should know how to intersect each other
    return geom1.intersects(geom2);
}

inline std::ostream& operator<<(std::ostream& os, const Sphere& s);
inline std::ostream& operator<<(std::ostream& os, const Cylinder& c);

}  // namespace spatial_index


#include "detail/geometries.hpp"
