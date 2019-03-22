#pragma once

#include <vector>
#include <iostream>
#include <functional>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "entries.hpp"

namespace spatial_index {

namespace b = boost;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template <typename Point>
inline bg::model::box<Point>
bounding_box(const sphere<Point>& entry)
{
    using result_type =  bg::model::box<Point>;

    // copy them
    Point min_corner(entry.centroid);
    Point max_corner(entry.centroid);

    bg::add_value(max_corner, entry.radius);
    bg::subtract_value(min_corner, entry.radius);

    return result_type(min_corner, max_corner);

}

}
