#pragma once

#include "../index_bulk_builder.hpp"

namespace spatial_index {

template <class Value>
template <class BeginIt, class EndIt>
inline void IndexBulkBuilderBase<Value>::insert(BeginIt begin, EndIt end) {
    values_.insert(values_.end(), begin, end);
}


template <class Value>
inline void IndexBulkBuilderBase<Value>::insert(const Value& value) {
    values_.push_back(value);
}


template <class Value>
inline void IndexBulkBuilderBase<Value>::reserve(size_t n_local_elements) {
    values_.reserve(n_local_elements);
}


template <class Value>
inline size_t IndexBulkBuilderBase<Value>::size() const {
    if(!n_total_values_) {
        throw std::runtime_error("Total number of elements not yet known.");
    }

    return *n_total_values_;
}


template <class Index, class Value>
inline void IndexBulkBuilder<Index, Value>::finalize() {
    size_t n_values = this->values_.size();
    this->n_total_values_ = n_values;

    index_ = Index(this->values_);
}

template <class Index, class Value>
inline Index IndexBulkBuilder<Index, Value>::index() const {
    if(!index_) {
        throw std::runtime_error("The index hasn't been finalized yet.");
    }

    return *index_;
}



}