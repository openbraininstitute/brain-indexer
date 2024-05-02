#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <memory>
#include <random>

#include <brain_indexer/distributed_sorting.hpp>
#include <brain_indexer/multi_index.hpp>
#include <brain_indexer/sort_tile_recursion.hpp>
#include <brain_indexer/util.hpp>

#include <boost/geometry/geometries/adapted/std_array.hpp>
#include <boost/geometry/geometries/point_xyz.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using namespace brain_indexer;


BOOST_AUTO_TEST_CASE(CheckIntLog2) {
    BOOST_CHECK(int_log2(1) == 0);
    BOOST_CHECK(int_log2(2) == 1);
    BOOST_CHECK(int_log2(1024) == 10);
}

BOOST_AUTO_TEST_CASE(CheckIntPow2) {
    BOOST_CHECK(int_pow2(0) == 1);
    BOOST_CHECK(int_pow2(1) == 2);
    BOOST_CHECK(int_pow2(10) == 1024);
}


struct Value {
    Point3D point;
    std::array<size_t, 2> payload;
};


struct GetCoordFromValue {
    template<size_t dim>
    static double apply(const Value &value) {
        return value.point.get<dim>();
    }
};


void extend(Box3D &box, const Point3D &x) {
    auto &lower = box.min_corner();
    auto &upper = box.max_corner();

    lower.set<0>(std::min(lower.get<0>(), x.get<0>()));
    upper.set<0>(std::max(upper.get<0>(), x.get<0>()));

    lower.set<1>(std::min(lower.get<1>(), x.get<1>()));
    upper.set<1>(std::max(upper.get<1>(), x.get<1>()));

    lower.set<2>(std::min(lower.get<2>(), x.get<2>()));
    upper.set<2>(std::max(upper.get<2>(), x.get<2>()));
}


template<class BeginIt, class EndIt>
Box3D bounding_box(BeginIt begin, EndIt end) {
    auto bb = Box3D(
        {
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        },
        {
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        }
    );

    for(BeginIt it = begin; it != end; ++it) {
        extend(bb, it->point);
    }

    return bb;
}


void check_nothing_got_lost(const std::vector<Value> &values,
                    size_t n_local_values,
                    int mpi_rank) {

    std::vector<int> counts(n_local_values, 0);
    for(const auto &v : values) {
        if(v.payload[0] == util::integer_cast<size_t>(mpi_rank)) {
            ++counts[v.payload[1]];
        }
    }

    for(const auto &c : counts) {
        BOOST_REQUIRE(c == 1);
    }
}

void check_bounding_boxes(std::vector<Value> &values,
                          std::vector<size_t> &boundaries,
                          const SerialSTRParams &str_params,
                          const std::array<float, 2> &domain) {

    auto bounding_boxes = std::vector<Box3D>{};
    for(size_t i = 0; i < str_params.n_parts(); ++i) {
        auto i_begin = boundaries[i];
        auto i_end = boundaries[i+1];

        bounding_boxes.push_back(
            bounding_box(values.data() + i_begin, values.data() + i_end)
        );
    }

    for(size_t i = 0; i < str_params.n_parts(); ++i) {
        auto side_length = domain[1] - domain[0];
        const auto &n_parts_per_dim = str_params.n_parts_per_dim;

        auto bb = bounding_boxes[i];
        auto lower = bb.min_corner();
        auto upper = bb.max_corner();

        auto check = [&values](const auto &lower, const auto &upper, auto expected_length) {
            auto actual_length = upper - lower;
            BOOST_REQUIRE(std::abs(actual_length - expected_length) < 0.2);
        };

        check(lower.get<0>(), upper.get<0>(), side_length / n_parts_per_dim[0]);
        check(lower.get<1>(), upper.get<1>(), side_length / n_parts_per_dim[1]);
        check(lower.get<2>(), upper.get<2>(), side_length / n_parts_per_dim[2]);
    }

    auto rtree = bgi::rtree<Box3D, bgi::linear<16>>(
        bounding_boxes.begin(),
        bounding_boxes.end()
    );

    for(const auto bb : bounding_boxes) {
        std::vector<Box3D> results;
        rtree.query(bgi::intersects(bb), std::back_inserter(results));

        BOOST_REQUIRE(results.size() == 1ul);
    }
}

BOOST_AUTO_TEST_CASE(SerialSTRTests) {
    if(mpi::rank(MPI_COMM_WORLD) != 0) {
        return;
    }

    int fake_mpi_rank = 32;

    size_t n_values = 1000ul;
    std::vector<Value> values;
    values.reserve(n_values);

    auto domain = std::array<float, 2>{-1.0, 1.0};
    auto gen = std::default_random_engine{};
    auto dist = std::uniform_real_distribution<float>(domain[0], domain[1]);

    for(size_t i = 0; i < n_values; ++i) {
        values.push_back(Value{
            {dist(gen), dist(gen), dist(gen)},
            {size_t(fake_mpi_rank), i}
        });
    }

    auto str_params = SerialSTRParams{n_values, {3ul, 2ul, 1ul}};
    serial_sort_tile_recursion<Value, GetCoordFromValue>(values, str_params);

    check_nothing_got_lost(values, n_values, fake_mpi_rank);

    auto partition_boundaries = str_params.partition_boundaries();
    check_bounding_boxes(values, partition_boundaries, str_params, domain);
}

std::vector<Value> random_values(size_t n_values,
                                 const std::array<float, 2> &domain,
                                 int comm_rank) {
    std::vector<Value> values;
    values.reserve(n_values);
    auto gen = std::default_random_engine{
      util::integer_cast<std::default_random_engine::result_type>(comm_rank+1)
    };
    auto dist = std::uniform_real_distribution<float>(domain[0], domain[1]);

    for(size_t i = 0; i < n_values; ++i) {
        values.push_back(
                Value{
                        {dist(gen), dist(gen), dist(gen)},
                        {size_t(comm_rank), i}
                }
        );
    }

    return values;
}


BOOST_AUTO_TEST_CASE(DistributedSTRTests) {
    int n_required_ranks = 4;
    auto comm = mpi::comm_shrink(MPI_COMM_WORLD, n_required_ranks);

    if(*comm == comm.invalid_handle()) {
        return;
    }

    auto comm_rank = mpi::rank(*comm);
    auto comm_size = mpi::size(*comm);

    size_t n_initial_values = 1000ul;

    auto domain = std::array<float, 2>{-1.0, 1.0};
    auto values = random_values(n_initial_values, domain, comm_rank);

    auto distr_params = DistributedSTRParams{comm_size * n_initial_values, {2, 2, 1}};
    distributed_sort_tile_recursion<Value, GetCoordFromValue>(values, distr_params, *comm);


    auto recv_counts = mpi::gather_counts(values.size(), *comm);
    auto recv_offsets = mpi::offsets_from_counts(recv_counts);

    int n_send = values.size();
    auto all_values = std::vector<Value>(comm_size * n_initial_values);

    auto mpi_value = mpi::Datatype(mpi::create_contiguous_datatype<Value>());

    MPI_Gatherv(
        (void *) values.data(), n_send, *mpi_value,
        (void *) all_values.data(), recv_counts.data(), recv_offsets.data(), *mpi_value,
        /* root = */ 0,
        *comm
    );

    if(comm_rank == 0) {
        std::array<size_t, 3> n_overall_parts_per_dim;
        std::copy(
            distr_params.n_ranks_per_dim.begin(),
            distr_params.n_ranks_per_dim.end(),
            n_overall_parts_per_dim.begin()
        );

        auto all_str_params = SerialSTRParams{
            all_values.size(),
            n_overall_parts_per_dim
        };

        auto boundaries = std::vector<size_t>(recv_offsets.size(), 0ul);
        std::copy(recv_offsets.begin(), recv_offsets.end(), boundaries.begin());
        check_bounding_boxes(all_values, boundaries, all_str_params, domain);
    }
}

BOOST_AUTO_TEST_CASE(DistributedPartitionTests) {
    // Currently, only checks that everything compiles and runs.
    using Value = MorphoEntry;

    int n_required_ranks = 2;
    auto comm = mpi::comm_shrink(MPI_COMM_WORLD, n_required_ranks);

    if(*comm == comm.invalid_handle()) {
        return;
    }

    auto comm_size = mpi::size(*comm);

    std::string output_dir = "tmp-wiowu";
    util::ensure_valid_output_directory(output_dir);

    auto storage = NativeStorageT<Value>(output_dir);

    auto n_values = 1000ul;
    auto n_total_values = comm_size * n_values;
    auto values = std::vector<Value>(n_values);
    auto str_params = two_level_str_heuristic(n_total_values, size_t(1e6), comm_size);

    distributed_partition<GetCenterCoordinate<Value>>(storage, values, str_params, *comm);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
