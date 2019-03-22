#pragma once

#include <vector>
#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>


namespace spatial_index{

namespace b = boost;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template<typename Point>
struct sphere
{
        using point_type = Point;
        using coordinate_type = typename bg::coordinate_type<point_type>::type;

        point_type centroid;
        coordinate_type radius;

        sphere(const point_type& centroid_, const coordinate_type& radius_):
        centroid(centroid_),
        radius(radius_)
        {}

        sphere(point_type&& centroid_, coordinate_type&& radius_):
        centroid(std::move(centroid_)),
        radius(std::move(radius_))
        {}

        static inline sphere< bg::model::point<coordinate_type, 3, bg::cs::cartesian> >
        from_raw_data(coordinate_type cx, coordinate_type cy, coordinate_type cz, coordinate_type r)
        {
            using point_type = bg::model::point<coordinate_type, 3, bg::cs::cartesian>;
            using result_type = sphere<point_type>;

            auto&& centroid = point_type(cx, cy, cz);

            return result_type( std::move(centroid), std::move(r) );
        }

        inline bool operator==(const sphere<point_type>& other)
        {
            const auto& other_centroid = other.centroid;

            return  bg::get<0>(centroid) == bg::get<0>(other_centroid) and
                    bg::get<1>(centroid) == bg::get<1>(other_centroid) and
                    bg::get<2>(centroid) == bg::get<2>(other_centroid) and
                    radius == other.radius;
        }

};


}

