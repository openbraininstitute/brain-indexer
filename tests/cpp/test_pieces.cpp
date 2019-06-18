#include <spatial_index/entries.hpp>
#include <spatial_index/index.hpp>

namespace si = spatial_index;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;


using CoordType = float;
using Point3D = bg::model::point<CoordType, 2, bg::cs::cartesian>;
using BBox = bg::model::box<Point3D>;


struct IndexableShape
{
	virtual BBox bounding_box() const = 0;
};

struct ShapeIndex
{
    using coordinate_type = CoordType;
	using point_type = Point3D;

	const BBox bbox;
	const IndexableShape& obj;

	ShapeIndex(const IndexableShape & obj_)
		: bbox(obj_.bounding_box()),
		  obj(obj_)
	{
		std::cout << "Created bbox: "
				  << bg::wkt<BBox>(bbox) << std::endl;
	}

	bool operator==(const ShapeIndex & other) const {
		return &obj == &(other.obj);
	}
};


namespace boost { namespace geometry { namespace index {

template <>
struct indexable<ShapeIndex>
{
    using result_type = BBox;
    inline result_type operator()(const ShapeIndex& o) const {
        return o.bbox;
    }
};

}}} // namespace boost::geometry::index



struct Sphere2 : public IndexableShape
{
    const Point3D centroid;
    const CoordType radius;

    Sphere2(const Point3D& centroid_, const CoordType& radius_)
        : centroid(centroid_)
        , radius(radius_)
    {}

    BBox bounding_box() const
	{
	    // copy them
	    Point3D min_corner(centroid);
	    Point3D max_corner(centroid);

	    bg::add_value(max_corner, radius);
	    bg::subtract_value(min_corner, radius);

	    return BBox(min_corner, max_corner);
	}

};


int main()
{
	bgi::rtree<ShapeIndex,
               bgi::rstar<16, 4>
               > rtree;


    Sphere2 spheres[] = {Sphere2({.0, .0}, 1.), Sphere2({4., 0.}, 1.5)};
    for (auto const& x : spheres)  {
    	rtree.insert(ShapeIndex(x));
    }

    BBox query_box(Point3D(0, 0), Point3D(1, 0));
    std::vector<ShapeIndex> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    printf("Num objects: %lu\n", result_s.size());

}
