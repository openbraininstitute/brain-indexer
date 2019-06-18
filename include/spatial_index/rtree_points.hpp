
#pragma once

#include <vector>
#include <iostream>
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

#include "common.hpp"

namespace spatial_index {

// Helpful namespace shortcuts
namespace b = boost;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;


template <typename CoordinateType, typename SizeType>
class PointRTree {
public:

    using size_type = SizeType;
    using coordinate_type = CoordinateType;

    using point_type = bg::model::point<CoordinateType, 3, bg::cs::cartesian>;

    using box_type = bg::model::box <point_type>;
    using value_type = std::pair<point_type, size_type>;


    PointRTree(): rtree_()
    {}

    PointRTree(const std::vector<point_type>& points):
    rtree_(points | boost::adaptors::indexed()
                  | boost::adaptors::transformed( pair_maker<point_type, size_type>() )
          )
    {}

    void intersection(const point_type& min_corner,
                      const point_type& max_corner,
                      std::vector<size_type>& idx)
    {
        const auto query_box = box_type(min_corner, max_corner);
        for(auto it =  bgi::qbegin(rtree_, bgi::intersects(query_box));
                 it != bgi::qend(rtree_); ++it)
        {
            idx.push_back(it->second); // index
        }
    }

    void intersection(const point_type& min_corner,
                      const point_type& max_corner,
                      std::vector<coordinate_type>& coordinates)
    {
        const auto query_box = box_type(min_corner, max_corner);
        for(auto it =  bgi::qbegin(rtree_, bgi::intersects(query_box));
                 it != bgi::qend(rtree_); ++it)
        {
            const auto& p = it->first;
            coordinates.push_back(bg::get<0>(p)); // x
            coordinates.push_back(bg::get<1>(p)); // y
            coordinates.push_back(bg::get<2>(p)); // z
        }
    }

    inline size_type size(){ return rtree_.size(); }

private:

    bgi::rtree< value_type, bgi::rstar<16, 4> > rtree_;

};


} // namespace spatial_index



