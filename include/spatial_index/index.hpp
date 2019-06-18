#pragma once

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/variant.hpp>
#else
#error("SpatialIndex requires BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL")
#endif

#include <boost/geometry/index/rtree.hpp>
#include <boost/variant.hpp>

#include <fstream>
#include <iostream>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


#include "geometries.hpp"



// It's fundamental to specialize indexable for our new Value type
// on how to retrieve the bounding box
// In this case we have a boost::variant of the existing shapes
// where all classes shall implement bounding_box()

namespace boost { namespace geometry { namespace index {


template <typename... VariantArgs>
struct indexable< boost::variant<VariantArgs...> >
{
    typedef boost::variant<VariantArgs...> V;

    typedef spatial_index::Box3D const result_type;

    struct BoundingBoxVisitor : boost::static_visitor<result_type> {
        template<class T>
        inline result_type operator()(const T& t)const { return t.bounding_box(); }
    };

    inline result_type operator()(V const& v) const {
        return boost::apply_visitor(bbox_visitor_, v);
    }

private:
    BoundingBoxVisitor bbox_visitor_;
};


}}} // namespace boost::geometry::index



namespace spatial_index {

    namespace bgi = boost::geometry::index;

    typedef boost::variant<Sphere> IndexEntry;

    typedef bgi::rtree< IndexEntry, bgi::linear<16, 1> > IndexTree;

    template<typename... T>
    void index_dump(bgi::rtree<T...> rtree, const std::string& filename) {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        boost::archive::binary_oarchive oa(ofs);
        oa << rtree;
    }

    IndexTree index_load(const std::string& filename) {
        IndexTree loaded_tree;
        std::ifstream ifs(filename, std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> loaded_tree;
        return loaded_tree;
    }

}


// struct NeuronGeometry
// {
//     id_type id;

//     NeuronGeometry(id_type id_)
//         : id(id_) {}

//     inline id_type gid() const {
//         return id;
//     }

//     inline bool operator==(const NeuronGeometry& other) const {
//         return (id == other.id);
//     }
// };





