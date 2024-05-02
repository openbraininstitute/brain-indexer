#pragma once

#if SI_MPI == 1

#include <stdint.h>
#include <limits.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <limits>

#include <mpi.h>

#include <brain_indexer/util.hpp>

// The definition of `MPI_SIZE_T` is a slight renaming of
//  https://stackoverflow.com/a/40808411
// which is licensed CC BY-SA 3.0
#ifndef MPI_SIZE_T
#if SIZE_MAX == UCHAR_MAX
#define MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#define MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#define MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#error "Failed to determine MPI datatype of `std::size_t`."
#endif
#else
#error "The token 'MPI_SIZE_T' is already defined."
#endif

namespace brain_indexer { namespace mpi {

inline int rank(MPI_Comm comm);
inline int size(MPI_Comm comm);
inline void abort(const std::string &msg, MPI_Comm comm = MPI_COMM_WORLD, int exit_code=-1);

template<class T>
inline MPI_Datatype datatype();

template<>
inline MPI_Datatype datatype<float>() {
    return MPI_FLOAT;
}

template<>
inline MPI_Datatype datatype<double>() {
    return MPI_DOUBLE;
}

template<>
inline MPI_Datatype datatype<size_t>() {
    return MPI_SIZE_T;
}


/** \brief Convert counts into offsets.
 *
 * Computes the offsets into a buffer given counts. Essentially,
 * this simply a cumulative sum.
 *
 * Note, the offsets are length `counts.size() + 1`, i.e. the
 * last element in `offsets` is the size of the buffer.
 */
inline std::vector<int> offsets_from_counts(const std::vector<int> & counts);


/** \brief Check that these counts won't lead to `int` overflow.
 * 
 *  The counts represents the number of elements this MPI rank
 *  wants to either receive or send to the every MPI rank. For
 *  example by `MPI_Alltoallv` or `MPI_Gatherv`.
 * 
 *  This function checks that with these counts, no integer overflow
 *  will happen.
 */
inline bool check_counts_are_safe(const std::vector<int> &counts);

/** \brief Throws if the counts aren't safe.
 *  \sa `check_counts_are_safe`.
 */
inline void assert_counts_are_safe(const std::vector<int> &counts, const std::string &error_id);


/** \brief Gather counts for an `MPI_Gatherv`.
 */
inline std::vector<int> gather_counts(size_t exact_count, MPI_Comm comm);


/** \brief Exchange/Allgather `local_count`.
 *
 * This is useful when all MPI ranks need to know the size of the local
 * parts of a buffer.
 *
 * Example: Given the global index of an element inside a distributed array, one can find the MPI
 * rank and local index if one knows how many elements each part of the distributed array has. This
 * function can be used to communicate the size of the local part to all other ranks and receive the
 * size of their local parts.
 */
inline std::vector<size_t> exchange_local_counts(size_t local_count, MPI_Comm comm);


/** \brief Compute counts for an `MPI_Alltoallv`.
 *
 * In a typical usecase of MPI_Alltoallv each MPI rank knows how
 * many elements is want to send to every MPI rank (including
 * itself), but it doesn't know how much everyone else wants
 * to send to it.
 *
 * This routine computes the number of elements each MPI rank
 * receives from every MPI rank.
 *
 * Assuming that both the sent and received data is stored
 * contiguously one can use `offsets_from_counts` to compute
 * the offsets.
 * 
 * \param send_counts  Number of elements this MPI rank wants
 *                     to sent to every MPI rank, including itself.
 *                     These are assumed to be safe, w.r.t. to
 *                     `int` overflow.
 * 
 * \return The counts this rank will receive from all ranks. They
 *   are checked with `assert_counts_are_safe` to protect against
 *   `int` overflow in later routines.
 */
inline std::vector<int>
exchange_counts(const std::vector<int> &send_counts, MPI_Comm comm);


/** \brief Compute the number of elements to send for balancing.
 *
 * For a distributed array with local counts `counts_per_rank`, we would like
 * to redistribute the elements such that each MPI rank has a fair share of
 * the elements, see `balanced_chunk_sizes`. Furthermore, we'd like
 * that the natural order in which the elements are traversed doesn't change,
 * i.e., the elements aren't shuffled.
 *
 * In this context "natural order" means that all elements on rank `i` come
 * before any elements on rank `j` if `i` < `j` and within each MPI rank
 * they're traversed in order (of their indices).
 *
 * This routine computes the number of elements this MPI rank wants to send
 * to every MPI rank (including itself).
 *
 * Since this is most likely used in an `MPI_Alltoallv`, the integer types
 * are `int` not `size_t`. You must ensure that the sum of `counts_per_rank`
 * does not overflow, i.e. that the counts are safe.
 *
 * Useful routines: `mpi::exchange_counts`, `mpi::offsets_from_counts`.
 */
inline std::vector<int>
compute_balance_send_counts(const std::vector<size_t>& counts_per_rank, int mpi_rank);


/// RAII style wrapper for MPI handles.
/** Idiomatic C tends to expose handles to resources which need to be
 *  acquired (allocated, opened) and released (free, closed) manually. Idiomatic
 *  C++ on the other hand prefers the RAII approach to dealing with resources.
 *  RAII simply means that the life time of the resource is tied to the life
 *  time of the C++ object representing the resource.
 *
 *  The term *resource* shall refer to an object which needs to be
 *  acquired before use; must be released exactly once; and must never
 *  be used after it's been released.
 *
 *  The term *handle* shall refer to an object with shallow copy semantics that
 *  identifies the resource it's a handle for. Often either a pointer to an
 *  opaque object or an integer identifier.
 *
 *  Observations:
 *    - the details of acquiring the resource can often be
 *      configured by the user and is a very intentional act.
 *
 *    - there's only one way of releasing the resource, and it's easily
 *      forgotten.
 *
 *  Therefore, `Resource` doesn't provide means of acquiring the resource,
 *  only for taking over ownership of a resource from its handle.
 *
 *  This class offers a CRTP base class for wrapping MPI handles as
 *  resources. It implements the common API of resources:
 *
 *    - Take ownership of the resource through the constructor.
 *    - Forbid copying of the resource, but allow moving.
 *    - Provide an interface for dropping ownership.
 *    - Access to the raw handle, e.g., to be passed to other functions.
 *
 *  Note, `Resource` implements unique ownership. If shared ownership is needed
 *  one can maybe use a `std::shared_ptr<Derived>`.
 *
 * \tparam Derived  The type of the C++ resource. It must support two static
 *                  methods:
 *                    - `invalid_handle()` which returns a handle representing
 *                      the null handle.
 *
 *                    - `free(Handle &)` which releases the resource.
 *
 * \tparam Handle   The type of the C handle to be wrapped as a C++ resource.
 */
template <class Derived, class Handle>
class Resource {
public:
  /// Acquires ownership of the resource.
  explicit Resource(Handle handle);

  Resource(Resource &&other) noexcept;
  Resource(const Resource &) = delete;

  ~Resource();

  Resource &operator=(Resource &&other) noexcept;
  Resource &operator=(const Resource &) = delete;

  /// Return the C handle to the resource.
  /** This does not drop ownership of the resource. Therefore, the handle
   *  must not be used to release the resource. Use `drop_ownership` to
   *  gain ownership of the resource.
   */
  Handle operator*() const;

  /// Returns the handle to and drops ownership of the resource.
  /** Note that after dropping ownership, this object represents
   *  the null resource.
   */
  Handle drop_ownership();

private:
  Handle handle_;
};

/// RAII-style wrapper for `MPI_Comm`.
/** \sa `brain_indexer::mpi::Resource` for a detailed documentation.
 */
class Comm : public Resource<Comm, MPI_Comm> {
private:
    using super = Resource<Comm, MPI_Comm>;

public:
    using super::Resource;
    using super::operator=;

    static void free(MPI_Comm comm);
    static MPI_Comm invalid_handle() noexcept;
};


void comm_free(MPI_Comm &comm);


/// Shallow wrapper for `MPI_Comm_split`.
/**
 *  \param color Puts MPI ranks for which the value of `color` is the same
 *               into the same communicator.
 *
 *  \param order In MPI jargon this is `key`. MPI assigns the new ranks such
 *               that they are sorted w.r.t. `order`. Or put more simply the
 *               new MPI ranks will be in the order `order` suggests.
 */
inline Comm comm_split(MPI_Comm comm, int color, int order);


/** \brief Create a new comm with the desired size.
 *
 *  The first `n_ranks` ranks are put in one communicator. On all
 *  other ranks `MPI_COMM_NULL` is returned.
 *
 *  Common usecase: For testing purposes one may want to one use
 *  the first few MPI ranks and ignore the rest.
 */
inline Comm comm_shrink(MPI_Comm old_comm, int n_ranks);


/// A minimal RAII-style wrapper for `MPI_Datatype`.
class Datatype : public Resource<Datatype, MPI_Datatype> {
private:
    using super = Resource<Datatype, MPI_Datatype>;

public:
    using super::Resource;
    using super::operator=;

    static void free(MPI_Datatype &datatype);
    static MPI_Datatype invalid_handle() noexcept;
};

template<class T>
Datatype create_contiguous_datatype();

}} // brain_indexer::mpi

#include "detail/mpi_wrapper.hpp"

#endif // SI_MPI
