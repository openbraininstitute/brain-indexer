#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/timer.hpp>


namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;



using id_type = unsigned long;
using CoordType = float;
using Point3D = bg::model::point<CoordType, 2, bg::cs::cartesian>;
using Box3D = bg::model::box<Point3D>;



namespace boost { namespace geometry { namespace index {

template <typename... VariantArgs>
struct indexable< boost::variant<VariantArgs...> >
{
    typedef boost::variant<VariantArgs...> V;

    typedef Box3D const result_type;

    struct BoundingBoxVisitor : boost::static_visitor<result_type> {
        template<class T>
        inline result_type operator()(const T& t)const { return t.bounding_box(); }
    };

    inline result_type operator()(V const& v) const {
        return boost::apply_visitor(get_box_visitor, v);
        // Requires c++14
        // return boost::apply_visitor([](const auto& t){ return t.bounding_box(); }, v);
    }
private:
    BoundingBoxVisitor get_box_visitor;
};


}}} // namespace boost::geometry::index


struct NeuronGeometry
{
    id_type id;

    NeuronGeometry(id_type id_)
        : id(id_) {}

    inline id_type gid() const {
        return id;
    }

    inline bool operator==(const NeuronGeometry& other) const {
        return (id == other.id);
    }
};


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

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & centroid;
        ar & radius;
    }

};


// class point : public Point3D {
//     inline point operator+(point other) {
//         bg::add_point(other, *this);
//         return other;
//     }

//     inline point operator-(const point& other) {
//         point res(*this);
//         bg::subtract_point(res, other);
//         return res;
//     }
// };


struct Cylinder
{
    Point3D p1, p2;
    CoordType radius;

    // Cylinder(id_type gid, unsigned segment_index, Point3D p1_, Point3D p2_, CoordType radius_)
    //     : NeuronGeometry{(gid << 12) + segment_index}
    //     , p1(p1_)
    //     , p2(p2_)
    //     , radius(radius_) {}

    // inline id_type gid() const {
    //     return id >> 12;
    // }

    // inline unsigned sement_index() const {
    //     return id & 0x0ffful;
    // }

    inline CoordType length() const {
        return bg::distance(p1, p2);
    }

    inline Box3D bounding_box() const
    {
        Point3D vec(p2);
        bg::subtract_point(vec, p1);
        //CoordType e = radius * std::sqrt(1.0 - vec * vec / bg::dot_product(vec, vec));
        //return Box3D(min(p1 - e, p2 - e),
        //             max(p1 + e, p2 + e));
        return Box3D(p1, p2);  // TODO: fake
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





int main()
{
    typedef boost::variant<Sphere> Entry;

    // create the rtree using default constructor
    bgi::rtree< Entry, bgi::linear<16, 1> > rtree;

    std::cout << "filling index with objects" << std::endl;

    // fill the spatial index
    Sphere spheres[] = {Sphere{{.0, .0}, 1.}, Sphere{{4., 0.}, 1.5}};
    for (auto const& x : spheres)  {
        rtree.insert(x);
    }

    Box3D query_box(Point3D(2, 0), Point3D(3, 1));
    std::vector<Entry> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    printf("Num objects: %lu\n", result_s.size());
    printf("Selected object: %f\n", boost::get<Sphere>(result_s.front()).radius);

    {
        boost::timer t;
        std::ofstream ofs("serialized_vector.bin", std::ios::binary | std::ios::trunc);
        boost::archive::binary_oarchive oa(ofs);
        oa << rtree;
        std::cout << "tree saved to bin in: " << t.elapsed() << std::endl;
    }

    {
        // Replay on reconstructed tree
        bgi::rtree< Entry, bgi::linear<16, 1> > rtree2;
        boost::timer t;
        std::ifstream ifs("serialized_vector.bin", std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> rtree2;
        std::cout << "tree Loaded from bin in: " << t.elapsed() << std::endl;

        result_s.clear();
        rtree2.query(bgi::intersects(query_box), std::back_inserter(result_s));
        printf("Num objects: %lu\n", result_s.size());
        printf("Selected object: %f\n", boost::get<Sphere>(result_s.front()).radius);
    }

    return 0;
}