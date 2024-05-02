#pragma once

#include <brain_indexer/util.hpp>

namespace brain_indexer {


SerialSTRParams::SerialSTRParams(size_t n_points, const std::array<size_t, 3>& n_parts_per_dim)
    : n_points(n_points), n_parts_per_dim(n_parts_per_dim) {}


size_t SerialSTRParams::n_parts_per_slice(size_t dim) const {
    return   (dim < 1 ? n_parts_per_dim[1] : 1)
           * (dim < 2 ? n_parts_per_dim[2] : 1);
}


std::vector<size_t> SerialSTRParams::partition_boundaries() const {
    auto partition_boundaries = std::vector<size_t>(n_parts() + 1);

    auto linear_id = [&](size_t i, size_t j, size_t k) {
        return k + n_parts_per_dim[2] * (j + n_parts_per_dim[1] * i);
    };

    for (size_t i = 0; i < n_parts_per_dim[0]; ++i) {
        auto i_chunk = util::balanced_chunks(n_points, n_parts_per_dim[0], i);

        for (size_t j = 0; j < n_parts_per_dim[1]; ++j) {
            auto j_chunk = balanced_chunks(i_chunk, n_parts_per_dim[1], j);

            for (size_t k = 0; k < n_parts_per_dim[2]; ++k) {
                auto k_chunk = balanced_chunks(j_chunk, n_parts_per_dim[2], k);

                auto ijk = linear_id(i, j, k);
                partition_boundaries[ijk] = k_chunk.low;
                partition_boundaries[ijk + 1] = k_chunk.high;
            }
        }
    }

    return partition_boundaries;
}


SerialSTRParams SerialSTRParams::from_heuristic(size_t n_points, size_t max_elements_per_part) {
    auto n_parts_approx_large = double(n_points) / double(max_elements_per_part);
    auto n_parts_approx_small = std::pow(double(n_points), 1.0 / 3.0);
    auto n_parts_approx = std::max(n_parts_approx_small, n_parts_approx_large);
    auto n_parts_per_dim_approx = std::pow(n_parts_approx, 1.0 / 3.0);
    auto n_parts_per_dim = size_t(std::ceil(n_parts_per_dim_approx));

    return {n_points, {n_parts_per_dim, n_parts_per_dim, n_parts_per_dim}};
}



template <class Value, typename GetCoordinate, size_t dim>
void SerialSortTileRecursion<Value, GetCoordinate, dim>::apply(std::vector<Value>& values,
                                                               size_t values_begin,
                                                               size_t values_end,
                                                               const SerialSTRParams& str_params) {

    if constexpr(dim < 3) {
        util::check_signals();
        std::sort(
            values.data() + values_begin,
            values.data() + values_end,
            [](const Value &a, const Value &b) {
                return Key::compare(a, b);
            }
        );

        auto n_parts_per_dim = str_params.n_parts_per_dim;

        for (size_t i = 0; i < n_parts_per_dim[dim]; ++i) {
            auto range = util::balanced_chunks(values_end - values_begin,
                                               n_parts_per_dim[dim],
                                               i);

            auto subvalues_begin = std::min(values_begin + range.low, values_end);
            auto subvalues_end = std::min(values_begin + range.high, values_end);

            STR<dim+1>::apply(
                values,
                subvalues_begin,
                subvalues_end,
                str_params
            );
        }
    }
}


template <typename Value, typename GetCoordinate>
void serial_sort_tile_recursion(std::vector<Value>& values, const SerialSTRParams& str_params) {

    using STR = SerialSortTileRecursion<Value, GetCoordinate, 0ul>;
    STR::apply(values, 0ul, values.size(), str_params);
}

}
