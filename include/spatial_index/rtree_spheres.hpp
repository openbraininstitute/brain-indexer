
#pragma once

#include <vector>
#include <iostream>
#include <functional>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "common.hpp"
#include "entries.hpp"
#include "bounding_box.hpp"

namespace spatial_index {

// Helpful namespace shortcuts
namespace b = boost;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;


template<typename Entry>
inline bool collides(const Entry& sphere1, const Entry& sphere2)
{
    auto vec( sphere1.centroid );
    bg::subtract_point(vec, sphere2.centroid);

    const auto radii_sum = sphere1.radius + sphere2.radius;

    return bg::dot_product(vec, vec) <= radii_sum * radii_sum;
};


template<typename Entry, typename SizeType>
struct entry_indexable
{
    using result_type =
        bg::model::box< typename Entry::point_type >;

    inline result_type operator()(const std::pair<Entry, SizeType>& indexed_entry) const
    {
        return bounding_box(indexed_entry.first);
    }
};


template<typename Value>
struct entry_equal_to
{

    inline static bool apply(const Value& lhs_pair,
                             const Value& rhs_pair)
    {
        // first element is entry
        // second element index
        return lhs_pair.first == rhs_pair.first and
               lhs_pair.second == rhs_pair.second;
    }
};


template < typename CoordinateType,  typename SizeType >
class SphereRTree {
public:

    using coordinate_type = CoordinateType;
    using size_type = SizeType;

    // cartesian 3D point
    using point_type = bg::model::point<coordinate_type, 3, bg::cs::cartesian>;

    // bounding box for sphere
    using box_type = bg::model::box<point_type>;

    // type of objects that will be inserted
    using entry_type = sphere<point_type>;

    // indexed objects
    using value_type = std::pair<entry_type, size_type>;

    // determines how we compare our value_types
    using equal_to = entry_equal_to<value_type>;

    // determines how we can extract a bbox from the value_type
    using indexable_getter = entry_indexable<entry_type, size_type>;

    SphereRTree(): rtree_()
    {}

    SphereRTree(const std::vector<value_type>& indexed_entries): rtree_(indexed_entries)
    {}

    template<typename T>
    inline void insert(T&& entry)
    {
        // TODO: expose object id to the outside. Incremental ids are not necessary.
        rtree_.insert( std::make_pair(
                                        std::forward<T>(entry),
                                        rtree_.size()
                                      )
        );
    }

    inline void intersection(const entry_type& entry, std::vector<size_type>& results)
    {
        const auto bbox = bounding_box(entry);
        std::transform(
            bgi::qbegin(rtree_,
                        bgi::intersects(bbox) &&            // bounding box intersection
                        bgi::satisfies(                     // actual geometry intersection
                                        [&](const value_type& pair){ return collides<entry_type>(entry, pair.first); }
                                      )
                       ),
            bgi::qend(rtree_),
            std::back_inserter(results),
            [](const value_type& pair) -> size_t { return pair.second; } // extract index from entry

         );
    }

    inline bool is_intersecting(const entry_type& entry)
    {
        const auto bbox = bounding_box(entry);
        for(auto it = bgi::qbegin(rtree_, bgi::intersects(bbox)); it != bgi::qend(rtree_); ++it)
        {
            if (collides(entry, it->first))
                return true;
        }
        return false;
    }


    inline void nearest(const point_type& centroid, int k_neighbors, std::vector<size_type>& results)
    {
        int neighbors = 0;
        for (auto it = bgi::qbegin(rtree_, bgi::nearest(centroid, k_neighbors)); it != bgi::qend(rtree_); it++)
        {
            results.push_back(it->second);

            if (++neighbors == k_neighbors)
                return;
        }
    }

    inline void data(std::vector<coordinate_type>& flatten_data)
    {
        flatten_data.reserve(4 * size());
        for(auto const& entry: rtree_)
        {
            const auto& centroid = entry.first.centroid;
            const auto& radius = entry.first.radius;

            flatten_data.push_back( bg::get<0>(centroid) );
            flatten_data.push_back( bg::get<1>(centroid) );
            flatten_data.push_back( bg::get<2>(centroid) );
            flatten_data.push_back( radius );
        }
    }

    inline size_type size(){ return rtree_.size(); }

private:

    bgi::rtree<
                    value_type,
                    bgi::rstar<16, 4>,
                    indexable_getter,
                    equal_to
              > rtree_;

};

}



