#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <brain_indexer/mpi_wrapper.hpp>
#include <brain_indexer/distributed_sorting.hpp>
#include <brain_indexer/sort_tile_recursion.hpp>

#include <array>
#include <vector>
#include <random>

using namespace brain_indexer;

struct Sortable {
    double value;
    std::array<int, 2> payload;
};

std::vector<Sortable> random_values(size_t n, int tag) {
    auto gen = std::default_random_engine{};
    std::uniform_int_distribution<int> int_dist(0, 1000);
    std::uniform_real_distribution<double> real_dist(-100.0, 100.0);

    auto values = std::vector<Sortable>{};
    values.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        values.push_back(Sortable{real_dist(gen), {tag, int(i)}});
    }

    return values;
}

class GetValue {
public:
    static double apply(const Sortable &v) {
        return v.value;
    }

    static bool compare(const Sortable &v, const Sortable &w) {
        return v.value < w.value;
    }

};

BOOST_AUTO_TEST_CASE(DistributedSortingTests) {
    int n_required_ranks = 2;
    auto comm = mpi::comm_shrink(MPI_COMM_WORLD, n_required_ranks);

    if(*comm == comm.invalid_handle()) {
        return;
    }

    auto mpi_rank = mpi::rank(*comm);

    size_t n_r0 = 100ul; // size on rank 0
    size_t n_r1 = 2 * n_r0; // size on rank 1

    auto m_r0 = (n_r0 + n_r1 + 1)/2;
    auto m_r1 = (n_r0 + n_r1)/2;

    auto unsorted = random_values((mpi_rank == 0 ? n_r0 : n_r1), mpi_rank);
    auto sorted = unsorted;  // force a copy.

    using DMS = brain_indexer::DistributedMemorySorter<Sortable, GetValue>;
    DMS::sort_and_balance(sorted, *comm);

    // Round up division on rank 0, but not on rank 1.
    auto m_expected = (mpi_rank == 0 ? m_r0 : m_r1);

    std::cout << "n_expected = " << sorted.size() << " / " << m_expected << "\n" << std::flush;
    if(m_expected != sorted.size()) {
        throw std::runtime_error("Incorrect size.");
    }

    for(size_t i = 0; i < m_expected - 1; ++i) {
        if(sorted[i].value > sorted[i+1].value) {
            throw std::runtime_error("Incorrectly sorted.");
        }
    }

    auto combined = std::vector<Sortable>(n_r0 + n_r1);
    auto mpi_sortable = mpi::Datatype(mpi::create_contiguous_datatype<Sortable>());
    if(mpi_rank == 0) {
        MPI_Send(sorted.data(),          m_r0, *mpi_sortable, 1, 0, *comm);
        MPI_Recv(combined.data() + m_r0, m_r1, *mpi_sortable, 1, 0, *comm, MPI_STATUS_IGNORE);
        std::copy(sorted.begin(), sorted.end(), combined.begin());
    }
    else {
        MPI_Recv(combined.data(), m_r0, *mpi_sortable, 0, 0, *comm, MPI_STATUS_IGNORE);
        MPI_Send(sorted.data(),   m_r1, *mpi_sortable, 0, 0, *comm);
        std::copy(sorted.begin(), sorted.end(), combined.begin() + m_r0);
    }

    auto is_unchanged = [](const Sortable &a, const Sortable &b) {
        return a.value == b.value && a.payload == b.payload;
    };

    auto sorted_counts = std::vector<int>(mpi_rank == 0 ? n_r0 : n_r1, 0);
    for(const auto &p : combined) {
        if(p.payload[0] == mpi_rank) {
            ++sorted_counts[p.payload[1]];

            const auto &expected = unsorted[p.payload[1]];

            if(!is_unchanged(p, expected)) {
                throw std::runtime_error("Value or payload changed.");
            }
        }
    }

    for(const auto &k : sorted_counts) {
        if(k != 1) {
            throw std::runtime_error("Something got lost.");
        }
    }
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
