#pragma once

#include <unordered_map>

#include <boost/serialization/utility.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <nlohmann/json.hpp>

#include <spatial_index/geometries.hpp>
#include <spatial_index/index.hpp>
#include <spatial_index/sort_tile_recursion.hpp>
#include <spatial_index/util.hpp>

#if SI_MPI == 1
#include <spatial_index/mpi_wrapper.hpp>
#endif

namespace spatial_index {

/// \brief These filenames are used together with `NativeStorage`.
struct NativeFilenames {
    static inline std::string top_tree(const std::string& output_dir) {
        auto p = boost::filesystem::path(output_dir) / "index.rtree";
        return p.string();
    }

    static inline
    std::string subtree(const std::string& output_dir, size_t subtree_id) {
        auto dirname = boost::filesystem::path(output_dir);
        auto basename = std::string("index-l") + std::to_string(subtree_id) + ".rtree";
        auto p = dirname / basename;
        return p.string();
    }
};

/** \brief Interface for reading and writing (parts of) a multi index.
 *
 *  This is a CRTP base class which extends the API of the underlying
 *  implementation of reading and writing parts of the multi index.
 *
 *  \tparam Derived    The underlying implementation for reading and writing trees.
 *  \tparam RTree      The type of the subtrees of the multi index.
 *  \tparam Filenames  The filename conventions used to store the parts of the
 *                     multi index.
 */
template <class Derived, class TopTree, class SubTree, class Filenames>
class MultiIndexStorage {
  public:
    /// \brief The type of the top-level tree of the multi index.
    using toptree_type = TopTree;

    /// \brief The type of the subtrees of the multi index.
    using subtree_type = SubTree;

  public:
    MultiIndexStorage() = default;

    explicit MultiIndexStorage(std::string output_dir);

    inline void save_subtree(const SubTree& subtree, size_t subtree_id) const;
    inline static void save_subtree(const SubTree& subtree,
                             const std::string& output_dir,
                             size_t subtree_id);

    inline void save_top_tree(const TopTree& tree) const;
    inline static void save_top_tree(const TopTree& tree, const std::string& output_dir);

    inline SubTree load_subtree(size_t subtree_id) const;
    inline static SubTree load_subtree(const std::string& output_dir, size_t subtree_id);

    inline TopTree load_top_tree() const;
    inline static TopTree load_top_tree(const std::string& output_dir);

  private:
    std::string output_dir;
};


/** \brief Native Boost serialization.
 *
 *  This is storage policy for `UsageRateCache`. It delegates the saving the
 *  R-tree to Boost serialization.
 * 
 *  See, `NativeStorageT` for a version that selects the appropriate
 *  values of `TopTree` and `SubTree` for the common use case.
 *
 *  \tparam TopTree Type of the top-level index of a multi index.
 *  \tparam SubTree Type of the sub indices of a multi index.
 */
template <class TopTree, class SubTree>
class NativeStorage : public MultiIndexStorage<
                                NativeStorage<TopTree, SubTree>,
                                TopTree,
                                SubTree,
                                NativeFilenames> {
  private:
    using super = MultiIndexStorage<NativeStorage<TopTree, SubTree>,
                                    TopTree,
                                    SubTree,
                                    NativeFilenames>;

  public:
    explicit NativeStorage() = default;

    explicit NativeStorage(std::string output_dir);

    template <class RTree>
    inline static void save_tree(const RTree& rtree, const std::string& filename);

    template <class RTree>
    inline static RTree load_tree(const std::string& filename);

  private:
    template <class ...Args>
    inline static void load_tree_impl(bgi::rtree<Args...> &tree, const std::string& filename);

    template <class ...Args>
    inline static void save_tree_impl(const bgi::rtree<Args...> &tree, const std::string& filename);
};

using MultiIndexTopTreeT = bgi::rtree<IndexedSubtreeBox, bgi::linear<16, 2>>;

template<typename T>
using MultiIndexSubTreeT = IndexTreeBaseT<T>;

template<class T>
using NativeStorageT = NativeStorage<MultiIndexTopTreeT, MultiIndexSubTreeT<T>>;


/// \brief The parameters control the eviction policy of `UsageRateCache`.
struct UsageRateCacheParams {
    UsageRateCacheParams() = default;

    explicit UsageRateCacheParams(size_t max_cached_elements)
        : max_cached_elements(max_cached_elements) { }

    size_t max_cached_elements = 1ul;
    size_t max_evict = 1ul;
    size_t current_cached_subtrees = 0ul;
};

/** \brief A cache for loading and keeping R-trees in memory.
 *
 *  When using a multi-index a cache is needed to incrementally load more
 *  subtrees as they are needed and decide which trees should be evicted once
 *  there is insufficient memory to load further subtrees.
 *
 *  The eviction policy is the following: For each subtree the number of times
 *  the subtree is accessed per query that occurs while this tree is loaded can
 *  be computed. This number is called "usage rate". The subtrees with the lowest
 *  usage rate is evicted.
 * 
 *  See `UsageRateCacheT` for a convenient alias in the context of building a
 *  `MultiIndexTree`.
 *
 *  \tparam Storage  A policy for loading subtrees from disk.
 */
template <class Storage>
class UsageRateCache {
    /** \brief The meta data required to compute usage rate.
     *
     * The assumption is that there's a global query counter. It increases on
     * every query of the spatial index.
     *
     * The current value query counter at time of loading the subtree is stored as
     * the `load_generation`. The `current_access_count` is increased everytime
     * the subtree is requested.
     *
     * On eviction `previous_*` are increased such that they reflect the historic usage
     * rate.
     */
    class MetaData {
      public:
        double usage_rate(size_t query_count) const;
        size_t access_count() const;
        size_t eviction_count() const;
        size_t incache_count(size_t query_count) const;


        /// \brief To be called every time the subtree is queries while residing cache.
        inline void on_query();

        /// \brief To be called every time the subtree is loaded into cache.
        inline void on_load(size_t query_count);

        /// \brief To be called immediately before evicting the subtree.
        inline void on_evict(size_t query_count);

      private:
        size_t load_generation_ = 0;
        size_t current_access_count_ = 0;

        size_t previous_access_count_ = 0;
        size_t previous_age_ = 0;

        size_t eviction_count_ = 0;
    };

  public:
    using storage_type = Storage;
    using subtree_type = typename storage_type::subtree_type;

  public:
    UsageRateCache() = default;

    UsageRateCache(const UsageRateCacheParams& cache_params, Storage storage)
        : storage(std::move(storage))
        , cache_params(cache_params) { }

    ~UsageRateCache();

    /** \brief Return the subtree with id `subtree_id`.
     *
     * \param query_count The query count increses on every query to the spatial index.
     */
    template<class SubtreeID>
    inline const subtree_type& load_subtree(const SubtreeID& subtree_id, size_t query_count);

  protected:
    /// \brief Total number of elements across all subtrees loaded.
    size_t cached_elements() const;

    template<class SubtreeID>
    inline void evict_subtrees(const SubtreeID& subtree_id, size_t query_count);

    inline std::vector<size_t> subtree_ids_sorted_by_usage_rate(size_t query_count);


  private:
    Storage storage;

    std::unordered_map<size_t, subtree_type> subtrees;
    std::unordered_map<size_t, MetaData> meta_data;
    UsageRateCacheParams cache_params;

    size_t most_recent_query_count = 0;
};

template<typename T>
using UsageRateCacheT = UsageRateCache<NativeStorageT<T>>;


/** \brief Implements core querying functionality of a spatial index.
 *
 * This class only provides the core functionality for loading parts of a multi
 * index as they are needed. Please consult the high-level API REF for a
 * detailed explanation of the multi index.
 *
 * \tparam SubtreeCache  A policy for maintaining a cache of in-memory subtrees.
 */
template <class SubtreeCache>
class MultiIndexTreeBase {
  public:
    using storage_type = typename SubtreeCache::storage_type;
    using toptree_type = typename storage_type::toptree_type;
    using subtree_type = typename storage_type::subtree_type;

  public:
    MultiIndexTreeBase() = default;
    MultiIndexTreeBase(const storage_type& storage, SubtreeCache subtree_cache);

    template <class Predicates, class OutIt>
    inline void query(const Predicates& predicates, const OutIt& it) const;

  protected:
    template <class SubtreeID, class Predicates, class OutIt>
    inline void query_subtree(const SubtreeID& subtree_id,
                       const Predicates& predicates,
                       const OutIt& it) const;

    template <class SubtreeID>
    inline auto load_subtree(const SubtreeID& subtree_id) const -> const subtree_type&;

    toptree_type top_rtree;
    mutable SubtreeCache subtree_cache;
    mutable size_t query_count = 0;
};

template<class T>
using MultiIndexTreeBaseT = MultiIndexTreeBase<UsageRateCacheT<T>>;


/** \brief A spatial index consisting of multiple subtrees.
 * 
 *  The term multi index refers to the fact that this index is composed of two
 *  levels of spatial indices. A top-level R-tree and subtrees. The leaves of
 *  the top-level R-tree are the bounding boxes of the subtrees. The sub-trees
 *  are R-trees and their leaves are the elements to be indexed.
 * 
 *  The reason for breaking down a single R-tree into a two-level hierarchy is
 *  to enable loading subtrees only when they're needed. A typical (large) R-tree
 *  in the context of neurological circuits can be on the order of several TB.
 *  Easily exceeding the amount of available RAM. Therefore, the entire spatial
 *  index, in general, can't be fit in main memory. By only loading a subtree
 *  from persistent storage (HDD, GPFS, etc.) when it's needed by a query, the
 *  multi-index isn't restricted in size by the available memory.
 * 
 *  The multi index keeps loaded subtrees in memory until a user specified
 *  threshold is reached. Therefore, repeated queries are fast if they happen in
 *  a similar region of the indexed area.
 * 
 *  The available caches policies are:
 *   - `UsageRateCache` which evicts the least used subtree.
 */
template <typename T>
class MultiIndexTree: public IndexTreeMixin<MultiIndexTree<T>, T>,
                      public MultiIndexTreeBaseT<T> {
  private:
    using multi_index_base = MultiIndexTreeBaseT<T>;

  public:
    using value_type = T;

  public:
    inline MultiIndexTree() = default;
    using multi_index_base::multi_index_base;

    MultiIndexTree(const std::string& output_dir, size_t max_cached_bytes);

    MultiIndexTree(const NativeStorageT<T>& storage,
                   const UsageRateCacheParams& params);

    /// \brief Checks whether a given shape intersects any object in the tree
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline bool is_intersecting(const ShapeT& shape) const;


    /**
     * \brief Finds & return objects which intersect. To be used mainly with id-less objects
     * \returns A vector of references to tree objects
     */
    template <typename GeometryMode=BoundingBoxGeometry, typename ShapeT>
    inline auto find_intersecting_objs(const ShapeT& shape) const -> std::vector<value_type>;

    /** \brief Total number of index elements.
     */
    inline size_t size() const {
      size_t count = 0;

      for(auto it = this->top_rtree.begin(); it != this->top_rtree.end(); ++it) {
        count += it->n_elements;
      }

      return count;
    }
};

template<size_t dim, typename Value>
inline CoordType get_centroid_coordinate(const Value &value);


template<typename Value>
struct GetCenterCoordinate {
public:
    template<size_t dim>
    inline static CoordType apply(const Value &value) {
        return get_centroid_coordinate<dim>(value);
    }
};


#if SI_MPI == 1

/** \brief Build the multi index in bulk.
 *
 * This class offers an API which allows adding elements to the "index" one by one. However, no
 * index is created until `finalize()` is called.
 *
 * @tparam Value  The type of the elements in the index, e.g. `MorphoEntry`.
 */
template<class Value>
class MultiIndexBulkBuilder {
public:
    explicit MultiIndexBulkBuilder(std::string output_dir);

    template<class BeginIt, class EndIt>
    inline void insert(BeginIt begin, EndIt end);

    inline void insert(const Value &value);

    /** \brief Finalize the builder and build the index.
     *
     * Indicates that the user does not want to add anymore elements. Hence the
     * index can now be created (in parallel).
     *
     * \note This is an MPI collective operation and all ranks must participate.
     */
    inline void finalize(MPI_Comm comm = MPI_COMM_WORLD);

    /// \brief Resize the internal buffer.
    inline void reserve(size_t n_local_elements);

    /** \brief The total number of elements in the created index.
     *
     * This method can only be called after `finalize()`, since only then is the
     * total number of elements in the tree known.
     */
    inline size_t size() const;

    /** \brief The current number of elements on this MPI rank.
     */
    inline size_t local_size() const;

private:
    std::vector<Value> values_;
    std::string output_dir_;

    boost::optional<size_t> n_total_values_ = boost::none;
};

#endif

}  // namespace spatial_index

#include "detail/multi_index.hpp"
