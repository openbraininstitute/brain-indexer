#pragma once

#include <array>
#include <cmath>

#include <spatial_index/mpi_wrapper.hpp>
#include <spatial_index/distributed_sorting.hpp>
#include <spatial_index/index.hpp>


namespace spatial_index {

/** \brief Parameters defining the Sort Tile Recursion.
 *
 * The parameters are simply the number of parts into which
 * each space dimension is subdivided into.
 *
 * See `SerialSortTimeRecursion` for a detailed explanation
 * of sort tile recursion.
 */
struct SerialSTRParams {
    /// Total number of points in the for which STR is being performed.
    size_t n_points;

    /// Number of parts per space dimension.
    std::array<size_t, 3> n_parts_per_dim;

    /// Overall number of parts after STR.
    size_t n_parts() const {
        return n_parts_per_dim[0] * n_parts_per_dim[1] * n_parts_per_dim[2];
    }

    SerialSTRParams(size_t n_points, const std::array<size_t, 3> &n_parts_per_dim);

    /** \brief Number of parts in a slice.
     *
     * A slice of a multi-dimensional array refers to
     * a selection of the array where certain axes are
     * unconstrained, e.g., in numpy notation:
     *     A[0, :, :]
     *
     * This method returns the number of parts in a slice
     * when the axes up to and including `dim` are fixed,
     * e.g., for `dim == 1`:
     *     A[i, j, :]  # for any `i`, `j`
     *
     */
    size_t n_parts_per_slice(size_t dim) const;

    /** \brief Boundaries of the parts after STR.
     *
     * Let `b` denote the boundaries returned by this method.
     * After performing STR to `array`, the elements of `array`
     * will be ordered such that
     *
     *     array[b[k]], array[b[k] + 1], ..., array[b[k+1] - 1]
     *
     * are in part `k` of the partitioning.
     *
     * Note the length is one more than the number of partitions.
     */
    std::vector<size_t> partition_boundaries() const;

    /** \brief Construct STR parameters from a heuristic.
     *
     * The aim of this heuristic is to provide suitable parameters when
     * computing a distributed R-Tree.
     */
    static SerialSTRParams from_heuristic(size_t n_points);
};

/// Parameters for `DistributedSortTileRecursion`.
struct DistributedSTRParams {
    size_t n_boxes;
    std::array<int, 3> n_ranks_per_dim;
};

/// Minimal description of the on node STR partitioning.
struct LocalSTRParams {
    std::array<size_t, 3> n_parts_per_dim;
};

/** \brief Performs single-threaded Sort Tile Recursion.
 *
 * Sort Tile Recursion (STR) is an algorithm for partitioning
 * n-dimensional points in an n-dimensional (axis-aligned) box
 * such that:
 *   - each part has roughly the same number of points;
 *   - the parts themselves have non-overlapping bounding boxes;
 *
 * The algorithm is a simple recursive procedure. First, the
 * points are sorted by their x[0]-coordinate. Next, the points
 * are split evenly into `n[0]` parts. For each of those parts
 * the points are sorted by their x[1]-coordinate and again split
 * into `m[1]` equal parts. In two dimension the procedure would
 * be complete. In three or more dimensions the steps are repeated
 * as needed.
 *
 * \sa `serial_sort_tile_recursion` for a more convenient interface.
 *
 * \tparam Value  The type of the element that is undergoing
 *                STR. This will at least contain an n-dimensional
 *                coordinate, but might also include a payload that
 *                is to be sorted together with the points.
 *
 * \tparam GetCoordinate  The coordinate is obtain from an object of
 *                        type `Value`, by
 *
 *                            GetCoordinate::apply<dim>(value)
 *
 * \tparam dim  The dimension of the current iteration.
 */
template<class Value, typename GetCoordinate, size_t dim>
class SerialSortTileRecursion {
private:
    template<size_t D>
    using STR = SerialSortTileRecursion<Value, GetCoordinate, D>;

public:
    static void apply(std::vector<Value> &values,
                      size_t values_begin,
                      size_t values_end,
                      const SerialSTRParams &str_params);
};

template<class Value, typename GetCoordinate>
class SerialSortTileRecursion<Value, GetCoordinate, 3ul> {
public:
    template<class ...Args>
    static void apply(Args&& ...) {
        // Only here to break the infinite recursion.
    }
};

/** \brief Single threaded Sort Tile Recursion.
 *
 * \sa `SerialSortTileRecursion`.
 */
template <typename Value, typename GetCoordinate>
void serial_sort_tile_recursion(std::vector<Value> &values, const SerialSTRParams&str_params);

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
  class Key {
  public:
      static auto apply(const Value &a) {
          return GetCoordinate::template apply<dim>(a);
      }
  };

  template<size_t D>
  using STR = DistributedSortTileRecursion<Value, GetCoordinate, D>;

public:
    static void apply(std::vector<Value> &values,
                      const DistributedSTRParams&str_params,
                      MPI_Comm mpi_comm);
};

template<typename Value, typename GetCoordinate>
class DistributedSortTileRecursion<Value, GetCoordinate, 3ul> {
public:
    template<class ...Args>
    static void apply(Args&& ...) {
        // Only here to break the infinite recursion.
    }
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


inline bool is_power_of_two(int n) { return (n & (n - 1)) == 0; }
inline int int_log2(int n) { return int(std::round(std::log2(n))); }
inline int int_pow2(int k) { return 1 << k; }

/* \brief Evenly distribute ranks across dimensions.
 *  Given `n` MPI ranks find `m[0]`, `m[1]`, `m[2]` such
 *  that `n == m[0] * m[1] * m[2]` and the difference between all
 *  `m[k]` is reasonably small.
 *
 *  \warning Current implementation only supports powers of two.
 */
std::array<int, 3> rank_distribution(int comm_size);

/// Uses `SerialSTRParams::from_heuristics` as a heuristic.
TwoLevelSTRParams two_level_str_heuristic(size_t n_elements, int comm_size);


/// Creates the top-level and all subtrees of the multi-index.
template <class GetCenterCoordinate, class Storage, class Value>
void distributed_partition(const Storage &storage,
                           std::vector<Value> &values,
                           const TwoLevelSTRParams &str_params,
                           MPI_Comm comm);

}


#include "detail/sort_tile_recursion.hpp"
