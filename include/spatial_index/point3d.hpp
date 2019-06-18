
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>


namespace spatial_index {

namespace b = boost;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;


# ifdef BBPSPATIAL_DOUBLE_PRECISION
using CoordType = double;
# else
using CoordType = float;
# endif

using Point3D = bg::model::point<CoordType, 3, bg::cs::cartesian>;

using Box3D = bg::model::box<Point3D>;



/**
 * \brief An OO augmented point to improve code readability
 *         with sequences of operations
 */
struct Point3Dx
{

    inline Point3Dx(Point3D const& p)
        : point_(p) {}

    /// Vector component-wise

    inline Point3Dx operator+(Point3Dx other) const {
        bg::add_point(other.point_, point_);
        return other;
    }

    inline Point3Dx operator-(Point3Dx const& other) const {
        Point3Dx copy(point_);
        bg::subtract_point(copy.point_, other.point_);
        return copy;
    }

    inline Point3Dx operator*(Point3Dx other) const {
        bg::multiply_point(other.point_, point_);
        return other;
    }

    inline CoordType dot(Point3Dx const& p) const {
        return bg::dot_product(point_, p.point_);
    }

    inline Point3D cross(Point3Dx const& p) const {
        return bg::cross_product(point_, p.point_);
    }


    /// Operations with scalar

    inline Point3Dx operator+(CoordType val) const {
        Point3Dx copy(point_);
        bg::add_value(copy.point_, val);
        return copy;
    }

    inline Point3Dx operator-(CoordType val) const {
        Point3Dx copy(point_);
        bg::subtract_value(copy.point_, val);
        return copy;
    }

    inline Point3Dx operator*(CoordType val) const {
        Point3Dx copy(point_);
        bg::multiply_value(copy.point_, val);
        return copy;
    }

    inline Point3Dx operator/(CoordType val) const {
        Point3Dx copy(point_);
        bg::divide_value(copy.point_, val);
        return copy;
    }

    /// Self operands

    inline Point3Dx sqrt() const {
        return Point3D{std::sqrt(point_.get<0>()), std::sqrt(point_.get<1>()), std::sqrt(point_.get<2>())};
    }


    inline Point3D get() const {
        return point_;
    }


private:
    friend Point3Dx max(Point3Dx const& p1, Point3Dx const& p2);
    friend Point3Dx min(Point3Dx const& p1, Point3Dx const& p2);
    Point3D point_;
};


template<typename T> inline
Point3Dx operator+(const T& v, const Point3Dx& point) { return point + v; }
template<typename T> inline
Point3Dx operator-(const T& v, const Point3Dx& point) { return (point * -1.) + v; }
template<typename T> inline
Point3Dx operator*(const T& v, const Point3Dx& point) { return point * v; }

Point3Dx max(Point3Dx const& p1, Point3Dx const& p2) {
    return Point3D{std::max(p1.point_.get<0>(), p2.point_.get<0>()),
                   std::max(p1.point_.get<1>(), p2.point_.get<1>()),
                   std::max(p1.point_.get<2>(), p2.point_.get<2>())};
}

Point3Dx min(Point3Dx const& p1, Point3Dx const& p2) {
    return Point3D{std::min(p1.point_.get<0>(), p2.point_.get<0>()),
                   std::min(p1.point_.get<1>(), p2.point_.get<1>()),
                   std::min(p1.point_.get<2>(), p2.point_.get<2>())};
}

}  // namespace spatial_index
