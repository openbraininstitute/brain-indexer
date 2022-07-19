#pragma once
#if SI_MPI == 1

#include <spatial_index/distributed_sort_tile_recursion.hpp>

namespace spatial_index {


std::vector<IndexedSubtreeBox>
gather_bounding_boxes(const std::vector<IndexedSubtreeBox>& local_bounding_boxes,
                      MPI_Comm comm) {

    auto mpi_box = mpi::Datatype(mpi::create_contiguous_datatype<IndexedSubtreeBox>());

    auto recv_counts = mpi::gather_counts(local_bounding_boxes.size(), comm);

    // gather boxes.
    if (mpi::rank(comm) == 0) {
        auto recv_offsets = mpi::offsets_from_counts(recv_counts);

        size_t n_tl_boxes = std::accumulate(recv_counts.begin(), recv_counts.end(), 0ul);
        std::vector<IndexedSubtreeBox> bounding_boxes(n_tl_boxes);

        int n_send = util::safe_integer_cast<int>(local_bounding_boxes.size());
        MPI_Gatherv(
            (void *)local_bounding_boxes.data(), n_send, *mpi_box,
            (void *)bounding_boxes.data(), recv_counts.data(), recv_offsets.data(), *mpi_box,
            /* root = */ 0,
            comm
        );

        return bounding_boxes;
    } else {
        int n_send = util::safe_integer_cast<int>(local_bounding_boxes.size());
        MPI_Gatherv(
            (void *)local_bounding_boxes.data(), n_send, *mpi_box,
            nullptr, nullptr, nullptr, MPI_DATATYPE_NULL,
            /* root = */ 0,
            comm
        );

        return {};
    }
}


LocalSTRParams infer_local_str_params(const SerialSTRParams& overall_str_params,
                                      const DistributedSTRParams& distributed_str_params) {

    const auto &overall_parts = overall_str_params.n_parts_per_dim;

    const auto &distributed_parts = distributed_str_params.n_ranks_per_dim;
    auto local_parts = std::array<size_t, 3>{
        size_t(std::ceil(double(overall_parts[0]) / double(distributed_parts[0]))),
        size_t(std::ceil(double(overall_parts[1]) / double(distributed_parts[1]))),
        size_t(std::ceil(double(overall_parts[2]) / double(distributed_parts[2]))),
    };

    return LocalSTRParams{local_parts};
}


std::array<int, 3> rank_distribution(int comm_size) {
    assert(is_power_of_two(comm_size));

    auto dist = std::array<int, 3>{0, 0, 0};
    auto log2_n = int_log2(comm_size);
    for (int k = 0; k < log2_n; ++k) {
        dist[k % 3] += 1;
    }

    for(auto &dk : dist) {
        dk = int_pow2(dk);
    }

    assert(dist[0] * dist[1] * dist[2] == comm_size);
    return dist;
}


TwoLevelSTRParams two_level_str_heuristic(size_t n_elements,
                                          size_t max_elements_per_part,
                                          int comm_size) {

    auto distributed = DistributedSTRParams{n_elements, rank_distribution(comm_size)};
    auto overall_str_params = SerialSTRParams::from_heuristic(n_elements, max_elements_per_part);
    auto local = infer_local_str_params(overall_str_params, distributed);

    return {distributed, local};
}


template <typename Value, typename GetCoordinate>
void distributed_sort_tile_recursion(std::vector<Value>& values,
                                     const DistributedSTRParams& str_params,
                                     MPI_Comm mpi_comm) {
    using STR = DistributedSortTileRecursion<Value, GetCoordinate, 0ul>;
    return STR::apply(values, str_params, mpi_comm);
}


template <class GetCenterCoordinate, class Storage, class Value>
void distributed_partition(const Storage& storage,
                           std::vector<Value>& values,
                           const TwoLevelSTRParams& str_params,
                           MPI_Comm comm) {

    if(values.size() < 10ul * mpi::size(comm)) {
        // If needed we need to carefully check that this will work. A
        // likely reason this will fail is because there might be 0 elements
        // per part, which will cause distributed sort to throw.
        throw std::runtime_error("Too few elements.");
    }

    distributed_sort_tile_recursion<Value, GetCenterCoordinate>(
        values,
        str_params.distributed,
        comm
    );

    auto serial_str_params = SerialSTRParams{values.size(), str_params.local.n_parts_per_dim};
    serial_sort_tile_recursion<Value, GetCenterCoordinate>(values, serial_str_params);

    auto mpi_rank = mpi::rank(comm);

    auto n_serial_parts = serial_str_params.n_parts();
    auto local_boundaries = serial_str_params.partition_boundaries();
    auto local_bounding_boxes = std::vector<IndexedSubtreeBox>();
    local_bounding_boxes.reserve(n_serial_parts);

    for (size_t k = 0; k < n_serial_parts; ++k) {
        util::check_signals();
        auto subtree = typename Storage::subtree_type(
            values.data() + local_boundaries[k],
            values.data() + local_boundaries[k+1]
        );

        auto k_part = size_t(mpi_rank) * n_serial_parts + k;
        storage.save_subtree(subtree, k_part);

        local_bounding_boxes.push_back(IndexedSubtreeBox(k_part, subtree.size(), subtree.bounds()));
    }

    util::check_signals();
    auto bounding_boxes = gather_bounding_boxes(local_bounding_boxes, comm);

    if(mpi_rank == 0) {
        auto top_level_tree = typename Storage::toptree_type(
            bounding_boxes.begin(),
            bounding_boxes.end()
        );

        storage.save_top_tree(top_level_tree);
    }
}


template <typename Value, typename GetCoordinate, size_t dim>
void DistributedSortTileRecursion<Value, GetCoordinate, dim>::apply(
    std::vector<Value>& values,
    const DistributedSTRParams& str_params,
    MPI_Comm mpi_comm) {

    if constexpr (dim < 3) {
        util::check_signals();
        DistributedMemorySorter<Value, Key>::sort_and_balance(values, mpi_comm);

        if(dim == 2) {
            return;
        }

        // 1. Create a comm for everyone in this slice.
        auto k_rank_in_slice = mpi::rank(mpi_comm);
        int color = k_rank_in_slice / str_params.n_ranks_in_subslice<dim>();
        auto sub_comm = mpi::comm_split(mpi_comm, color, k_rank_in_slice);

        // 2. Let them do STR.
        STR<dim+1>::apply(values, str_params, *sub_comm);
    }
}



}
#endif
