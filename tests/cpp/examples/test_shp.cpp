#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <iostream>
#include <vector>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;


using CoordType = float;
using Point3D = bg::model::point<CoordType, 2, bg::cs::cartesian>;
using Box3D = bg::model::box<Point3D>;


template <typename Box>
struct IndexableShape {
    virtual inline Box bounding_box() const = 0;
};

template <typename Box>
struct ShapeIndex {
    using type = ShapeIndex<Box>;
    using entry_type = IndexableShape<Box>;

    Box bbox;
    entry_type* obj;

    ShapeIndex(const entry_type& obj_)
        : bbox(obj_.bounding_box())
        , obj(const_cast<entry_type*>(&obj_)) {}

    bool operator==(const type& other) const {
        return obj == other.obj;
    }
};


namespace boost {
namespace geometry {
namespace index {

template <typename Box>
struct indexable<ShapeIndex<Box>> {
    typedef ShapeIndex<Box> V;

    typedef Box const& result_type;
    result_type operator()(V const& v) const {
        return v.bbox;
    }
};

}  // namespace index
}  // namespace geometry
}  // namespace boost


struct Sphere2: public IndexableShape<Box3D> {
    const Point3D centroid;
    const CoordType radius;

    Sphere2(const Point3D& centroid_, const CoordType& radius_)
        : centroid(centroid_)
        , radius(radius_) {}

    virtual inline Box3D bounding_box() const {
        // copy them
        Point3D min_corner(centroid);
        Point3D max_corner(centroid);

        bg::add_value(max_corner, radius);
        bg::subtract_value(min_corner, radius);

        return Box3D(min_corner, max_corner);
    }
};


int main() {
    typedef ShapeIndex<Box3D> Entry;

    // create the rtree using default constructor
    bgi::rtree<Entry, bgi::linear<16, 4>> rtree;

    std::cout << "filling index with objects" << std::endl;

    // fill the spatial index
    Sphere2 spheres[] = {Sphere2({.0, .0}, 1.), Sphere2({4., 0.}, 1.5)};
    for (auto const& x: spheres) {
        rtree.insert((Entry(x)));
    }


    Box3D query_box(Point3D(2, 0), Point3D(3, 1));
    std::vector<Entry> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    printf("Num objects: %lu\n", result_s.size());
    printf("Radius of the first result: %f\n", static_cast<Sphere2*>(result_s.front().obj)->radius);

    return 0;
}