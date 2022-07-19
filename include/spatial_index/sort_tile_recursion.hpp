#pragma once

#include <array>
#include <cmath>

#include <spatial_index/index.hpp>


namespace spatial_index {

template<class GetCoordinate, size_t dim>
struct STRKey {
    template<class Value>
    static auto apply(const Value &a) {
        return GetCoordinate::template apply<dim>(a);
    }

    template<class Value>
    static auto compare(const Value &a, const Value &b) {
        auto xa = STRKey<GetCoordinate, dim>::apply(a);
        auto xb = STRKey<GetCoordinate, dim>::apply(b);

        if constexpr (dim+1 < 3) {
            return xa == xb ? STRKey<GetCoordinate, dim+1>::compare(a, b) : xa < xb;
        } else {
            return xa < xb;
        }
    }
};


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
    static SerialSTRParams from_heuristic(size_t n_points, size_t max_elements_per_part);

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
    using Key = STRKey<GetCoordinate, dim>;

    template<size_t D>
    using STR = SerialSortTileRecursion<Value, GetCoordinate, D>;

public:
    static void apply(std::vector<Value> &values,
                      size_t values_begin,
                      size_t values_end,
                      const SerialSTRParams &str_params);
};

/** \brief Single threaded Sort Tile Recursion.
 *
 * \sa `SerialSortTileRecursion`.
 */
template <typename Value, typename GetCoordinate>
void serial_sort_tile_recursion(std::vector<Value> &values, const SerialSTRParams&str_params);

inline bool is_power_of_two(int n) { return (n & (n - 1)) == 0; }
inline int int_log2(int n) { return int(std::round(std::log2(n))); }
inline int int_pow2(int k) { return 1 << k; }


}
#include "detail/sort_tile_recursion.hpp"
