#pragma once

#if SI_MPI == 1

#include <iostream>
#include <spatial_index/logging.hpp>

namespace spatial_index {
namespace mpi {

int rank(MPI_Comm comm) {
    int rank = -1;
    MPI_Comm_rank(comm, &rank);
    return rank;
}

int size(MPI_Comm comm) {
    int size = -1;
    MPI_Comm_size(comm, &size);
    return size;
}

void abort(const std::string& msg, MPI_Comm comm, int exit_code) {
    log_error(msg);
    MPI_Abort(comm, exit_code);
}

std::vector<int> offsets_from_counts(const std::vector<int>& counts) {
    std::vector<int> offsets(counts.size()+1);
    offsets[0] = 0;
    std::partial_sum(counts.begin(), counts.end(), offsets.begin() + 1);

    return offsets;
}

bool check_count_is_safe(size_t count) {
    return count <= util::safe_integer_cast<size_t>(std::numeric_limits<int>::max());
}

void assert_count_is_safe(size_t count, const std::string &error_id) {
    if(!check_count_is_safe(count)) {
        throw std::runtime_error("Count is too large and will overflow `int`. [" + error_id + "]");
    }
}

bool check_counts_are_safe(const std::vector<int>& counts) {
    auto total_elements = std::accumulate(counts.begin(), counts.end(), size_t(0));
    return check_count_is_safe(total_elements);
}


void assert_counts_are_safe(const std::vector<int>& counts, const std::string &error_id) {
    if(!check_counts_are_safe(counts)) {
        throw std::runtime_error("Counts are too large and will overflow `int`. [" + error_id + "]");
    }
}


std::vector<int> gather_counts(size_t exact_count, MPI_Comm comm) {
    auto int_count = util::safe_integer_cast<int>(exact_count);

    auto recv_counts = std::vector<int>(size(comm));
    MPI_Gather(
        (void *) &int_count,         1, MPI_INT,
        (void *) recv_counts.data(), 1, MPI_INT,
        /* root = */ 0,
        comm
    );

    assert_counts_are_safe(recv_counts, "vneoq");
    return recv_counts;
}


std::vector<size_t> exchange_local_counts(size_t local_count, MPI_Comm comm) {
    auto comm_size = mpi::size(comm);
    std::vector<size_t> count_per_rank(util::safe_integer_cast<size_t>(comm_size));

    MPI_Allgather(&local_count, 1, MPI_SIZE_T, count_per_rank.data(), 1, MPI_SIZE_T, comm);

    return count_per_rank;
}


std::vector<int> exchange_counts(const std::vector<int>& send_counts, MPI_Comm comm) {
    std::vector<int> recv_counts(send_counts.size());
    MPI_Alltoall(
        send_counts.data(), 1, MPI_INT,
        recv_counts.data(), 1, MPI_INT,
        comm
    );

    // The `send_counts` should be safe, however one rank might
    // be receiving all the big slabs. Hence, we need to check
    // `recv_counts`.
    assert_counts_are_safe(recv_counts, "pieww");
    return recv_counts;
}


std::vector<int>
compute_balance_send_counts(const std::vector<size_t>& counts_per_rank, int mpi_rank) {
    auto comm_size = counts_per_rank.size();
    auto global_count = std::accumulate(
        counts_per_rank.begin(), counts_per_rank.end(), 0ul
    );

    // global index of beginning local part of the array.
    size_t local_start = std::accumulate(
        counts_per_rank.begin(),
        counts_per_rank.begin() + mpi_rank,
        0ul
    );
    size_t local_end = local_start + counts_per_rank[mpi_rank]; // (exclusive)

    auto balanced_count_per_rank = util::balanced_chunk_sizes(global_count, comm_size);

    // Stores the number of values to be sent to each MPI rank.
    auto send_counts = std::vector<int>(comm_size, 0);

    // Global index of beginning & end (exclusive) of the balanced chunk
    // of MPI rank `i`.
    size_t balanced_start = 0ul, balanced_end = 0ul;

    // For every MPI rank compute if the balanced index interval overlaps
    // with the current index interval stored on this MPI rank. The element
    // be sent are the intersection of the two intervals
    //    [local_start, local_end)
    //    [balanced_start, balanced_end)
    for (size_t i = 0ul; i < comm_size; i++) {
        balanced_end += balanced_count_per_rank[i];

        if (balanced_start < local_end && local_start < balanced_end) {
            send_counts[i] = util::safe_integer_cast<int>(
                std::min(balanced_end, local_end)
                - std::max(balanced_start, local_start)
            );
        }

        balanced_start = balanced_end;
    }

    assert_counts_are_safe(send_counts, "kdwoi");
    return send_counts;
}


template <class Derived, class Handle>
Resource<Derived, Handle>::Resource(Handle handle)
    : handle_(handle) {}


template <class Derived, class Handle>
Resource<Derived, Handle>::Resource(Resource&& other) noexcept
    : handle_(other.drop_ownership()) {}


template <class Derived, class Handle>
Resource<Derived, Handle>::~Resource() {
    if(handle_ != Derived::invalid_handle()) {
        Derived::free(handle_);
    }
}


template <class Derived, class Handle>
Resource<Derived, Handle>&
Resource<Derived, Handle>::operator=(Resource&& other) noexcept {
    handle_ = other.drop_ownership();
    return (*this);
}


template <class Derived, class Handle>
Handle Resource<Derived, Handle>::operator*() const {
    return handle_;
}


template <class Derived, class Handle>
Handle Resource<Derived, Handle>::drop_ownership() {
    auto tmp = handle_;
    handle_ = Derived::invalid_handle();

    return tmp;
}

void comm_free(MPI_Comm& comm) {
    MPI_Comm_free(&comm);
}


Comm comm_split(MPI_Comm comm, int color, int order) {
    MPI_Comm new_comm;
    MPI_Comm_split(comm, color, order, &new_comm);

    return Comm{new_comm};
}

Comm comm_shrink(MPI_Comm old_comm, int n_ranks) {
    assert(0 < n_ranks);
    assert(n_ranks <= size(old_comm));

    auto r = rank(old_comm);
    auto new_comm = comm_split(old_comm, r < n_ranks, r);

    if(r >= n_ranks) {
        return Comm{MPI_COMM_NULL};
    }

    return new_comm;
}

void Datatype::free(MPI_Datatype& datatype) {
    MPI_Type_free(&datatype);
}


MPI_Datatype Datatype::invalid_handle() noexcept {
    return MPI_DATATYPE_NULL;
}


void Comm::free(MPI_Comm comm) {
    comm_free(comm);
}


MPI_Comm Comm::invalid_handle() noexcept {
    return MPI_COMM_NULL;
}


template <class T>
Datatype create_contiguous_datatype() {
    MPI_Datatype datatype_;
    MPI_Type_contiguous(sizeof(T), MPI_BYTE, &datatype_);
    MPI_Type_commit(&datatype_);

    return Datatype{datatype_};
}


}
}

#endif
