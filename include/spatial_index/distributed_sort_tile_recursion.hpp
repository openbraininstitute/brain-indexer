#pragma once

#if SI_MPI == 1

#include <spatial_index/mpi_wrapper.hpp>
#include <spatial_index/distributed_sorting.hpp>
#include <spatial_index/index.hpp>
#include <spatial_index/sort_tile_recursion.hpp>

namespace spatial_index {

/// Parameters for `DistributedSortTileRecursion`.
struct DistributedSTRParams {
    size_t n_boxes;
    std::array<int, 3> n_ranks_per_dim;

    template<size_t dim>
    int n_ranks_in_subslice() const {
        if(dim == 0) {
            return n_ranks_per_dim[1] * n_ranks_per_dim[2];
        }
        if(dim == 1) {
            return n_ranks_per_dim[2];
        }
        
        throw std::runtime_error("Invalid dim.");
    }
};


/// Minimal description of the on node STR partitioning.
struct LocalSTRParams {
    std::array<size_t, 3> n_parts_per_dim;
};


/** \brief MPI-parallel version of Sort Tile Recursion.
 *
 * Please refer to `SerialSortTileRecursion` for a detailed
 * explanation of the algorithm and the template parameters.
 *
 * The first thing to observe is that if we had a distributed
 * sorting algorithm, then STR can be computed in an MPI-parallel
 * in a straightforward manner.
 *
 * Assume that `n` is the number of parts per dimension and that
 * there are `product(n)` MPI ranks, i.e. one per part. Now,
 * distributed STR consists of: First perform a distributed sort
 * w.r.t to coordinate `x[0]`, redistribute the array such that
 * every MPI rank has roughly the same number of elements. Now
 * compute successive groups of MPI ranks of size `n[1]*n[2]*...`;
 * and continue with STR recursively.
 *
 * \sa `distributed_sort_tile_recursion` for a more convenient interface.
 * \sa `DistributedMemorySorter` for an implementation of a
 * distributed and balanced sorting algorithm.
 */
template<typename Value, typename GetCoordinate, size_t dim>
class DistributedSortTileRecursion {
private:
  using Key = STRKey<GetCoordinate, dim>;

  template<size_t D>
  using STR = DistributedSortTileRecursion<Value, GetCoordinate, D>;

public:
    static void apply(std::vector<Value> &values,
                      const DistributedSTRParams&str_params,
                      MPI_Comm mpi_comm);
};


/** \brief  MPI-parallel Sort Tile Recursion.
 *
 * \sa `DistributedSortTileRecursion`.
 */
template <typename Value, typename GetCoordinate>
void distributed_sort_tile_recursion(std::vector<Value> &values,
                                     const DistributedSTRParams&str_params,
                                     MPI_Comm mpi_comm);


std::vector<IndexedSubtreeBox> gather_bounding_boxes(
    const std::vector<IndexedSubtreeBox> &local_bounding_boxes,
    MPI_Comm comm);


/** \brief Parameters for a combined distributed and local STR.
 *  It can be convenient to perform STR first in a distributed
 *  manner, creating one large region per MPI rank. Then
 *  in a second step these region can be partitioned again using
 *  local STR.
 */
struct TwoLevelSTRParams {
    DistributedSTRParams distributed;
    LocalSTRParams local;
};


LocalSTRParams infer_local_str_params(
    const SerialSTRParams &overall_str_params,
    const DistributedSTRParams &distributed_str_params);


/* \brief Evenly distribute ranks across dimensions.
 *  Given `n` MPI ranks find `m[0]`, `m[1]`, `m[2]` such
 *  that `n == m[0] * m[1] * m[2]` and the difference between all
 *  `m[k]` is reasonably small.
 *
 *  \warning Current implementation only supports powers of two.
 */
std::array<int, 3> rank_distribution(int comm_size);


/// Uses `SerialSTRParams::from_heuristics` as a heuristic.
TwoLevelSTRParams two_level_str_heuristic(size_t n_elements,
                                          size_t max_elements_per_part,
                                          int comm_size);


/// Creates the top-level and all subtrees of the multi-index.
template <class GetCenterCoordinate, class Storage, class Value>
void distributed_partition(const Storage &storage,
                           std::vector<Value> &values,
                           const TwoLevelSTRParams &str_params,
                           MPI_Comm comm);


}

#include "detail/distributed_sort_tile_recursion.hpp"

#endif // SI_MPI
