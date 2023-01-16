#pragma once

#include <vector>
#include <boost/optional.hpp>

namespace spatial_index {

template<class Value>
class IndexBulkBuilderBase {
  public:
    template<class BeginIt, class EndIt>
    inline void insert(BeginIt begin, EndIt end);

    inline void insert(const Value &value);

    /// \brief Resize the internal buffer.
    inline void reserve(size_t n_local_elements);

    /// \brief The total number of elements in the created index.
    ///
    /// This method can only be called after `finalize()`, since only then is the
    /// total number of elements in the tree known.
    inline size_t size() const;

  protected:
    std::vector<Value> values_;
    boost::optional<size_t> n_total_values_ = boost::none;
};

/// \brief Bulk builder for in-memory indexes.
template<class Index, class Value = typename Index::value_type>
class IndexBulkBuilder : public IndexBulkBuilderBase<Value> {
  public:
    inline void finalize();

    /// \brief Obtain the index after it's been built.
    inline Index index() const;

  protected:
    boost::optional<Index> index_ = boost::none;
};

}

#include "detail/index_bulk_builder.hpp"