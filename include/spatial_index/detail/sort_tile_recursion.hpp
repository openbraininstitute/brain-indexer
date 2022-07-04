#pragma once

namespace spatial_index {

template <class Value, typename GetCoordinate, size_t dim>
void SerialSortTileRecursion<Value, GetCoordinate, dim>::apply(std::vector<Value>& values,
                                                               size_t values_begin,
                                                               size_t values_end,
                                                               const SerialSTRParams& str_params) {

    std::sort(
        values.data() + values_begin,
        values.data() + values_end,
        [](const Value &a, const Value &b) {
            auto xa = GetCoordinate::template apply<dim>(a);
            auto xb = GetCoordinate::template apply<dim>(b);
            return xa < xb;
        }
    );

    auto n_parts_per_dim = str_params.n_parts_per_dim;

    for (size_t i = 0; i < n_parts_per_dim[dim]; ++i) {
        auto range = mpi::balanced_chunks(values_end - values_begin,
                                          n_parts_per_dim[dim],
                                          i
        );

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


template <typename Value, typename GetCoordinate>
void serial_sort_tile_recursion(std::vector<Value>& values, const SerialSTRParams& str_params) {

    using STR = SerialSortTileRecursion<Value, GetCoordinate, 0ul>;
    STR::apply(values, 0ul, values.size(), str_params);
}

template <typename Value, typename GetCoordinate>
void distributed_sort_tile_recursion(std::vector<Value>& values,
                                     const DistributedSTRParams& str_params,
                                     MPI_Comm mpi_comm) {
    using STR = DistributedSortTileRecursion<Value, GetCoordinate, 0ul>;
    return STR::apply(values, str_params, mpi_comm);
}


template <typename Value, typename GetCoordinate, size_t dim>
void DistributedSortTileRecursion<Value, GetCoordinate, dim>::apply(
    std::vector<Value>& values,
    const DistributedSTRParams& str_params,
    MPI_Comm mpi_comm) {

    DistributedMemorySorter<Value, Key>::sort_and_balance(values, mpi_comm);

    // 1. Create a comm for everyone in this slice.
    auto k_rank_in_slice = mpi::rank(mpi_comm);
    int color = k_rank_in_slice % int(str_params.n_ranks_per_dim[dim]);
    auto sub_comm = mpi::comm_split(mpi_comm, color, k_rank_in_slice);

    // 2. Let them do STR.
    STR<dim+1>::apply(values, str_params, *sub_comm);
}


}
