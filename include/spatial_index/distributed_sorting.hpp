// This file is a modified copy of
//     DistributedMemorySorter.h
// in TouchDetector.

#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <cassert>

#include <spatial_index/mpi_wrapper.hpp>

namespace spatial_index {

/** \brief Distributed (MPI-parallel) sample sort.
 *
 * Sampling sort is a variant of bucket sorting where the buckets are
 * determined by sampling the data.
 *
 * If there are more then `SUB_SAMPLING_MINIMUM_CPUS` CPUs the sampling
 * is a two-step procedure:
 *   * First, groups of `SUB_SAMPLING_GROUP_SIZE` consecutive ranks perform
 *   sampling on their data.
 *
 *   * Second, representatives of each group of MPI ranks perform another
 *   round of sampling to determine the final buckets.
 *
 * \tparam T     The type of the objects being sorted.
 * \tparam Key   The static method `Key::apply(const T&a)` is used to
 *               compute a scalar value, which is then used to for
 *               sorting with the comparison:
 *                   Key::apply(a) < Key::apply(b)
 *
 * \sa https://en.wikipedia.org/wiki/Samplesort
 */
template<class T, class Key>
class DistributedMemorySorter {
public:
    using Value = T;
    using Values = std::vector<Value>;
    using KeyedType = decltype(Key::apply(std::declval<Value>()));

    /**
     * \brief Sort and evenly re-distribute values.
     *
     * \note This method invalidates references to elements in `values`, since,
     * in general, it needs to resize or move into `values`.
     *
     * \param values  The values to be sorted. These may include a payload to
     *                that needs to be moved with the value, e.g., the index
     *                for an argsort. The operation is "inplace" in the sense
     *                that `values` will contain the sorted values assigned to
     *                this MPI rank.
     *
     * \param comm MPI  The MPI communicator used for all communication.
     */
    static void sort_and_balance(Values &values, MPI_Comm comm);

private:
    DistributedMemorySorter();
    ~DistributedMemorySorter();

    MPI_Datatype mpi_value_ = MPI_DATATYPE_NULL;

    constexpr static int SUB_SAMPLING_GROUP_SIZE = 128;
    constexpr static int SUB_SAMPLING_MINIMUM_CPUS = 4096;

    /**
     * \brief Performs a parallel Sample Sort.
     *
     * \note The argument `values` acts both as an in- and output argument. The
     *       function may shuffle the elements in the original `values` and also
     *       invalidates all references to `values`, e.g. by reallocating or
     *       moving into `values`.
     */
    void sort(Values &values, MPI_Comm mpiComm);

    /**
     * \brief Distributes elements evenly across all MPI ranks.
     *
     * The elements are rebalanced without changing their order. Each
     * MPI rank will have an equal (plus/minus one) number of elements.
     */
    auto balance(const Values &values, MPI_Comm comm) -> Values;
};


/** \brief Gather `samples` on MPI rank 0 in `comm` and subsample.
 *
 * The argument `samples` acts as an input on every MPI rank and
 * additionally as an output buffer on MPI rank 0. The output buffer
 * will contain a subsampling of the gathered samples.
 *
 * The assumption is that the length of `samples` is the same on
 * every MPI rank.
 *
 * Note, this is a collective MPI operation; all ranks in `comm`
 * must participate.
 *
 * Note, the output buffer is only meaningful on MPI rank 0.
 */
template<class T>
void gather_and_subsample(std::vector<T> &samples, MPI_Comm comm);

}

#include "detail/distributed_sorting.hpp"
