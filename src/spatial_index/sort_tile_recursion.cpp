#include <spatial_index/sort_tile_recursion.hpp>

namespace spatial_index {

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
        auto i_chunk = mpi::balanced_chunks(n_points, n_parts_per_dim[0], i);

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


SerialSTRParams SerialSTRParams::from_heuristic(size_t n_points) {
    auto n_parts_approx = std::pow(double(n_points), 1.0 / 3.0);
    auto n_parts_per_dim_approx = std::pow(n_parts_approx, 1.0 / 3.0);

    auto n_parts_per_dim = size_t(std::ceil(n_parts_per_dim_approx));

    return {n_points, {n_parts_per_dim, n_parts_per_dim, n_parts_per_dim}};
}


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


TwoLevelSTRParams two_level_str_heuristic(size_t n_elements, int comm_size) {
    auto distributed = DistributedSTRParams{n_elements, rank_distribution(comm_size)};
    auto overall_str_params = SerialSTRParams::from_heuristic(n_elements);
    auto local = infer_local_str_params(overall_str_params, distributed);

    return {distributed, local};
}

}
