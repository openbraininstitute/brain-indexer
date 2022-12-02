#pragma once

namespace spatial_index {

/// \brief Compute the total number of elements across all MPI ranks.
inline size_t sum_local_counts(size_t local_count, MPI_Comm comm) {
    size_t global_count;
    MPI_Allreduce(&local_count, &global_count, 1, MPI_SIZE_T, MPI_SUM, comm);

    return global_count;
}


template<class BeginIt, class EndIt, class Comparison>
void node_local_sort(const BeginIt &begin_it,
                     const EndIt &end_it,
                     const Comparison &comparison) {
    std::sort(begin_it, end_it, comparison);
}


template<class BeginIt, class EndIt>
void node_local_sort(const BeginIt &begin_it, const EndIt &end_it) {
    std::sort(begin_it, end_it);
}


template<class T, class Key>
DistributedMemorySorter<T, Key>::DistributedMemorySorter() {
    MPI_Type_contiguous(sizeof(Value), MPI_BYTE, &mpi_value_);
    MPI_Type_commit(&mpi_value_);
}


template<class T, class Key>
DistributedMemorySorter<T, Key>::~DistributedMemorySorter() {
    MPI_Type_free(&mpi_value_);
}


template<class T, class Key>
void DistributedMemorySorter<T, Key>::sort_and_balance(Values &values,
                                                       MPI_Comm comm) {
    DistributedMemorySorter <T, Key> dms;
    dms.sort(values, comm);
    values = dms.balance(values, comm);
}


template<class T, class Key>
auto DistributedMemorySorter<T, Key>::balance(const Values &values, MPI_Comm comm)
-> Values {

    auto comm_size = mpi::size(comm);
    auto mpi_rank = mpi::rank(comm);

    // We now start working toward exchanging the data using an
    // `MPI_Alltoallv`. Which means first compute the number of
    // elements each MPI rank want to send to every MPI rank. The
    // underlying idea is that we want to redistribute the elements
    // without changing their order.

    // Step 1: Find out how many elements each MPI rank has.
    auto counts_per_rank = mpi::exchange_local_counts(values.size(), comm);

    // Step 2: Figure out how many elements this MPI rank needs to
    //         send to every MPI rank:
    auto send_counts = mpi::compute_balance_send_counts(counts_per_rank, mpi_rank);

    // Obtain the number of elements this MPI rank receives.
    auto recv_counts = mpi::exchange_counts(send_counts, comm);

    // Final preparatory step: convert counts to offsets.
    auto send_offsets = mpi::offsets_from_counts(send_counts);
    auto recv_offsets = mpi::offsets_from_counts(recv_counts);

    // Now `MPI_Alltoallv` redistributes the elements:
    Values received(recv_offsets[comm_size]);
    MPI_Alltoallv(
        values.data(), send_counts.data(), send_offsets.data(), mpi_value_,
        received.data(), recv_counts.data(), recv_offsets.data(), mpi_value_,
        comm
    );

    return received;
}


template<class T, class Key>
void DistributedMemorySorter<T, Key>::sort(Values &values,
                                           MPI_Comm comm) {
    size_t count = values.size();

    auto mpi_size = mpi::size(comm);
    auto mpi_rank = mpi::rank(comm);

    if (mpi_size <= 0) {
        // This probably means that MPI hasn't been initiated. One
        // could simply use `std::sort` if this function must work
        // even without MPI.
        throw std::runtime_error("Invalid MPI communicator size.");
    }

    if (values.empty()) {
        throw std::runtime_error("sort failed: value.size == 0.");
    }

    auto cmp = [=](const Value &a, const Value &b) {
        return Key::compare(a, b);
    };
    node_local_sort(values.begin(), values.end(), cmp);

    // For the locally sorted values select equally spaced samples.
    //
    // NOTE The choice of having exactly `mpi_size` many sample on
    //      every MPI rank is a little arbitrary.
    std::vector<KeyedType> samples(mpi_size);
    for (int i = 0; i < mpi_size; i++) {
        size_t pos = ((count-1) * (i+1)) / mpi_size;
        samples[i] = Key::apply(values[pos]);
    }

    // Since `gather_and_subsample` requires `O(mpi_size**2)` memory,
    // OOM can occur. To prevent this, use a two stage approach.
    if (mpi_size > SUB_SAMPLING_MINIMUM_CPUS) {
        int mpi_rank_local = mpi_rank % SUB_SAMPLING_GROUP_SIZE;
        int mpi_group_local = mpi_rank / SUB_SAMPLING_GROUP_SIZE;

        // split the original communicator into subgroups to gather and subsample on
        // MPI rank 0 on each of the sub communicators.
        MPI_Comm intra_group_comm;
        MPI_Comm_split(comm, mpi_group_local, mpi_rank_local, &intra_group_comm);
        gather_and_subsample(samples, intra_group_comm);
        MPI_Comm_free(&intra_group_comm);

        // The first phase of sub sampling is now complete, so masters of sub groups
        // will do the same step with master of the original comm group (i.e., here,
        // we filter each master rank of each group, and we use the group number as
        // rank number)
        MPI_Comm inter_group_comm;
        MPI_Comm_split(comm, mpi_rank_local, mpi_group_local, &inter_group_comm);
        if (mpi_rank_local == 0) {  // if it was a master before
            gather_and_subsample(samples, inter_group_comm);
        }
        MPI_Comm_free(&inter_group_comm);
    } else {
        gather_and_subsample(samples, comm);
    }

    // On MPI rank 0 in `comm` there are `mpi_size` sorted samples. These,
    // will now become the buckets. More precisely, they store the right boundary of each bucket.
    // Therefore, we first ensure that the final bucket is unbounded and then broadcast the buckets
    // to every rank.
    samples.back() = std::numeric_limits<KeyedType>::max();
    MPI_Bcast(samples.data(), mpi_size, mpi::datatype<KeyedType>(), 0, comm);

    // We now start working toward exchanging the data using an
    // `MPI_Alltoallv`. First, compute the number of local elements
    // in each bucket.
    std::vector<int> send_counts(mpi_size);

    // Since `values` is sorted, the `bucketIndex` can only increase
    // and the counts can be computed in a single sweep as follows:
    size_t bucketIndex = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        for (; Key::apply(values[i]) >= samples[bucketIndex]; ++bucketIndex) {
        }
        ++send_counts[bucketIndex];
    }

    // Exchange the number of elements to be sent to obtain how many
    // elements this rank receives.
    auto recv_counts = mpi::exchange_counts(send_counts, comm);

    // Convert counts to offsets, i.e., compute a cumulative sum.
    std::vector<int> send_offsets = mpi::offsets_from_counts(send_counts);
    std::vector<int> recv_offsets = mpi::offsets_from_counts(recv_counts);

    // The function `MPI_Alltoallv` is exactly what's needed
    // to exchange the data.
    Values sorted(recv_offsets[mpi_size]);
    MPI_Alltoallv(
        values.data(), send_counts.data(), send_offsets.data(), mpi_value_,
        sorted.data(), recv_counts.data(), recv_offsets.data(), mpi_value_,
        comm
    );

    // Perform the final local sort.
    node_local_sort(sorted.begin(), sorted.end(), cmp);

    values.swap(sorted);
}

template <class T>
void gather_and_subsample(std::vector<T>& samples, MPI_Comm comm) {
    static_assert(
        std::is_trivially_copyable<T>::value,
        "The samples can't be copied as bytes."
    );
    assert(!samples.empty());

    // Used as an argument for MPI functions. Hence, an `int`.
    const int n_samples = samples.size();

    auto mpi_rank = mpi::rank(comm);
    auto mpi_size = mpi::size(comm);

    std::vector<T> recv_samples;
    if (mpi_rank == 0) {
        recv_samples.resize(mpi_size * n_samples);
    }

    // Gather all samples from this group of MPI ranks on
    // rank 0 (in `comm`).
    MPI_Gather(
        samples.data(), n_samples, mpi::datatype<T>(),
        recv_samples.data(), n_samples, mpi::datatype<T>(),
        /* root = */ 0,
        comm);

    // On rank 0 samples are selected among all received samples.
    if (mpi_rank == 0) {
        node_local_sort(recv_samples.begin(), recv_samples.end());

        for (int i = 0; i < n_samples; i++) {
            samples[i] = recv_samples[(i+1) * n_samples - 1];
        }
    }
}

}
