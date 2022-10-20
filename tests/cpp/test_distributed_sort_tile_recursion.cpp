#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <spatial_index/distributed_sort_tile_recursion.hpp>

using namespace spatial_index;

BOOST_AUTO_TEST_CASE(IsValidCommunicatorSize) {
    BOOST_REQUIRE(is_valid_comm_size(1));
    BOOST_REQUIRE(is_valid_comm_size(2));
    BOOST_REQUIRE(is_valid_comm_size(3));
    BOOST_REQUIRE(is_valid_comm_size(4));
    BOOST_REQUIRE(is_valid_comm_size(5));
    BOOST_REQUIRE(is_valid_comm_size(6));
    BOOST_REQUIRE(is_valid_comm_size(8));
    BOOST_REQUIRE(is_valid_comm_size(12));
    BOOST_REQUIRE(is_valid_comm_size(20));
    BOOST_REQUIRE(is_valid_comm_size(80));

    BOOST_REQUIRE(!is_valid_comm_size(7));
    BOOST_REQUIRE(!is_valid_comm_size(13));
}

BOOST_AUTO_TEST_CASE(FactorizeNumber) {
    auto primes = std::vector<int>{5, 3, 2};
    {
        auto decomp = factorize(1, primes);
        BOOST_REQUIRE(decomp[0] == 0);
        BOOST_REQUIRE(decomp[1] == 0);
        BOOST_REQUIRE(decomp[2] == 0);
    }

    {
        auto decomp = factorize(2, primes);
        BOOST_REQUIRE(decomp[0] == 0);
        BOOST_REQUIRE(decomp[1] == 0);
        BOOST_REQUIRE(decomp[2] == 1);
    }

    {
        auto decomp = factorize(3, primes);
        BOOST_REQUIRE(decomp[0] == 0);
        BOOST_REQUIRE(decomp[1] == 1);
        BOOST_REQUIRE(decomp[2] == 0);
    }

    {
        auto decomp = factorize(5, primes);
        BOOST_REQUIRE(decomp[0] == 1);
        BOOST_REQUIRE(decomp[1] == 0);
        BOOST_REQUIRE(decomp[2] == 0);
    }

    {
        auto decomp = factorize(80, primes);
        BOOST_REQUIRE(decomp[0] == 1);
        BOOST_REQUIRE(decomp[1] == 0);
        BOOST_REQUIRE(decomp[2] == 4);
    }
}

BOOST_AUTO_TEST_CASE(RankDistribution) {
    auto test_cases = std::vector<int>{2, 3, 4, 5, 6, 12, 20, 80, 160, 16*72};
    auto primes = std::vector<int>{2, 3, 5};

    for(auto comm_size : test_cases) {
        auto ranks = rank_distribution(comm_size);
        BOOST_REQUIRE(ranks[0] * ranks[1] * ranks[2] == comm_size);

        auto m = std::min(ranks[0], std::min(ranks[1], ranks[2]));
        auto M = std::max(ranks[0], std::max(ranks[1], ranks[2]));

        for(auto p : primes) {
            if(M > m * p) {
                BOOST_REQUIRE(M % p != 0);
            }
        }
    }

}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
