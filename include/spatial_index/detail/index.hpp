#pragma once

#include "result_iterators.hpp"


namespace spatial_index {

// Specialization of geometry_intersects for variant geometries

template <typename T, typename... VarT>
bool geometry_intersects(const boost::variant<VarT...>& geom1, const T& geom2) {
    return boost::apply_visitor([&geom2](const auto& g1){
            // In case T is another variant, can be matched
            return geometry_intersects(g1, geom2);
        },
        geom1
    );
}

template <typename T, typename... VarT>
bool geometry_intersects(const T& geom1, const boost::variant<VarT...>& geom2) {
    return geometry_intersects(geom2, geom1);
}


// IndexTree

template <typename T>
template <typename Shap>
inline std::vector<const T*> IndexTree<T>::find_intersecting(const Shap& shape) const {
    std::vector<const T*> results;  // To avoid copies we keep pointers
    bgi::indexable<Shap> box_from;

    this->query(bgi::intersects(box_from(shape)), iter_callback<T>(
        [&shape, &results](const T& v) {
            if (geometry_intersects(shape, v)) {
                results.push_back(&v);
            }
        }));
    return results;
}


template <typename T>
template <typename Shap>
inline bool IndexTree<T>::is_intersecting(const Shap& shape) const {
    bgi::indexable<Shap> box_from;
    for(auto it = this->qbegin(bgi::intersects(box_from(shape))); it != this->qend(); ++it) {
        if (geometry_intersects(shape, *it)) {
            return true;
        }
    }
    return false;
}


template <typename... T>
inline void index_dump(const bgi::rtree<T...>& rtree, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    boost::archive::binary_oarchive oa(ofs);
    oa << rtree;
}


template <typename T>
inline IndexTree<T> index_load(const std::string& filename) {
    IndexTree<T> loaded_tree;
    std::ifstream ifs(filename, std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> loaded_tree;
    return loaded_tree;
}



}