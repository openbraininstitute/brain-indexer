#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <memory>
#include <random>

#include <brain_indexer/multi_index.hpp>
#include <brain_indexer/distributed_sorting.hpp>

using namespace brain_indexer;


struct SubtreeState {
    std::unordered_map<size_t, size_t> n_loaded;
    std::unordered_map<size_t, size_t> n_evicted;
    std::unordered_map<size_t, size_t> n_elements;
};


struct MockRTree {
    MockRTree() = default;
    MockRTree(const MockRTree &) = default;

    MockRTree(std::shared_ptr<SubtreeState> subtree_state, size_t subtree_id)
        : subtree_id(subtree_id),
          subtree_state(std::move(subtree_state)) {
        ++(*this->subtree_state).n_loaded[subtree_id];
    }

    ~MockRTree() {
        if(subtree_state != nullptr) {
        ++(*subtree_state).n_evicted[subtree_id];
        }
    }

    MockRTree & operator=(MockRTree &&other) = default;

    template<class Predicate, class OutIt>
    void query(const Predicate, OutIt) {
        // Not needed.
    }

    size_t size() const {
        return subtree_state->n_elements[subtree_id];
    }

    size_t subtree_id;
    std::shared_ptr<SubtreeState> subtree_state;
};


class MockStorage {
  public:
    using toptree_type = MockRTree;
    using subtree_type = MockRTree;

  public:
    explicit MockStorage(std::shared_ptr<SubtreeState> subtree_state)
        : subtree_state(std::move(subtree_state)) {}

    MockRTree load_subtree(size_t subtree_id) {
        return MockRTree(subtree_state, subtree_id);
    }

  private:
    std::shared_ptr<SubtreeState> subtree_state;
};


struct SubtreeID {
    size_t id;
    size_t n_elements;
};


BOOST_AUTO_TEST_CASE(MultiIndexLoad) {
    auto subtree_state = std::make_shared<SubtreeState>();
    subtree_state->n_elements[42ul] = 4ul;
    subtree_state->n_elements[24ul] = 7ul;
    subtree_state->n_elements[30ul] = 9ul;
    subtree_state->n_elements[0ul] = 1ul;

    auto params = UsageRateCacheParams(20ul);
    auto storage = MockStorage(subtree_state);

    auto cache = UsageRateCache(params, storage);

    cache.load_subtree(SubtreeID{42ul, 4ul}, /* query_count */ 0ul);
    cache.load_subtree(SubtreeID{42ul, 4ul}, /* query_count */ 0ul);
    cache.load_subtree(SubtreeID{42ul, 4ul}, /* query_count */ 0ul);
    BOOST_TEST((*subtree_state).n_loaded[42ul] == 1ul);

    cache.load_subtree(SubtreeID{24ul, 7ul}, /* query_count */ 10ul);
    cache.load_subtree(SubtreeID{24ul, 7ul}, /* query_count */ 11ul);
    BOOST_TEST((*subtree_state).n_loaded[24ul] == 1ul);

    cache.load_subtree(SubtreeID{30ul, 9ul}, /* query_count */ 20ul);
    BOOST_TEST((*subtree_state).n_loaded[30ul] == 1ul);

    BOOST_TEST((*subtree_state).n_evicted[42ul] == 0ul);
    BOOST_TEST((*subtree_state).n_evicted[24ul] == 0ul);
    BOOST_TEST((*subtree_state).n_evicted[30ul] == 0ul);

    cache.load_subtree(SubtreeID{0ul, 1ul}, /* query_count */ 21ul);
    BOOST_TEST((*subtree_state).n_evicted[42ul] == 1ul);
    BOOST_TEST((*subtree_state).n_evicted[24ul] == 0ul);
    BOOST_TEST((*subtree_state).n_evicted[30ul] == 0ul);
    BOOST_TEST((*subtree_state).n_loaded[0ul] == 1ul);
    BOOST_TEST((*subtree_state).n_evicted[0ul] == 0ul);
}


BOOST_AUTO_TEST_CASE(MultiIndexCompiles) {
    auto synapse_index = MultiIndexTree<Synapse>{};
    auto morpho_index = MultiIndexTree<MorphoEntry>{};
}

BOOST_AUTO_TEST_CASE(TwoLevelParamsCutoff) {
    if(mpi::rank(MPI_COMM_WORLD) != 0) {
        return;
    }

    auto n_elements = size_t(45e9);
    auto max_elements = size_t(1e6);
    auto params = SerialSTRParams::from_heuristic(n_elements, max_elements);

    BOOST_CHECK(params.n_points / params.n_parts() <= max_elements);
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
