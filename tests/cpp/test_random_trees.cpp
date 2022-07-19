#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <random>
#include <vector>

#include <spatial_index/index.hpp>
#include <spatial_index/multi_index.hpp>
#include <spatial_index/util.hpp>

using namespace spatial_index;


template<class Element>
identifier_t get_id(const Element &element) {
    return element.gid();
}

template<>
identifier_t get_id<IndexedSubtreeBox>(const IndexedSubtreeBox& element) {
    return element.id;
}


template<class Element>
static std::vector<Element>
random_elements(size_t n_elements,
                const std::array<CoordType, 2> &domain,
                size_t id_offset);

template<>
std::vector<Segment>
random_elements<Segment>(size_t n_elements,
                         const std::array<CoordType, 2> &domain,
                         size_t id_offset) {

    auto length = domain[1] - domain[0];

    auto gen = std::default_random_engine{};
    auto pos_dist = std::uniform_real_distribution<CoordType>(domain[0], domain[1]);
    auto offset_dist = std::uniform_real_distribution<CoordType>(-0.01*length, 0.01*length);
    auto radius_dist = std::uniform_real_distribution<CoordType>(0.001*length, 0.01*length);

    auto elements = std::vector<Segment>{};
    elements.reserve(n_elements);

    for(size_t i = 0; i < n_elements; ++i) {
        auto x = Point3Dx{pos_dist(gen), pos_dist(gen), pos_dist(gen)};
        auto dx = Point3Dx{offset_dist(gen), offset_dist(gen), offset_dist(gen)};
        auto r = radius_dist(gen);

        elements.emplace_back(id_offset + i, 0u, 0u, x, x + dx, r);
    }

    return elements;
}

template<class Element>
static std::vector<Element>
gather_elements(const std::vector<Element> &local_elements, MPI_Comm comm) {
    auto comm_size = mpi::size(comm);

    auto n_elements = util::safe_integer_cast<int>(local_elements.size());
    std::vector<Element> all_elements(comm_size*local_elements.size());

    auto mpi_type = mpi::Datatype(mpi::create_contiguous_datatype<Element>());

    MPI_Gather(
        (void *) local_elements.data(), n_elements, *mpi_type,
        (void *) all_elements.data(), n_elements, *mpi_type,
        /* root = */ 0,
        comm
    );

    return all_elements;
}

template<class Element, class Index, class QueryShape>
static void check_queries_against_geometric_primitives(
    const std::vector<Element> &elements,
    const Index &index,
    const QueryShape &query_shape) {

    using GeometryMode = BoundingBoxGeometry;

    std::vector<Element> found;
    index.template find_intersecting<GeometryMode>(query_shape, std::back_inserter(found));

    BOOST_CHECK(index.template count_intersecting<GeometryMode>(query_shape) == found.size());
    BOOST_CHECK(index.template is_intersecting<GeometryMode>(query_shape) == !found.empty());

    auto intersecting = std::unordered_map<identifier_t, bool>{};

    for(const auto &element : elements) {
        auto id = get_id(element);
        if(intersecting.find(id) != intersecting.end()) {
            // This signals a faulty assumption in the test logic; and the test
            // needs to be rewritten.
            throw std::runtime_error("Ids aren't unique.");
        }

        intersecting[id] = false;
    }

    for(const auto& element : found) {
        auto id = get_id(element);
        intersecting[id] = true;
    }

    for(const auto& [id, actual] : intersecting) {
        const auto &element = elements.at(id);
        auto expected = geometry_intersects(query_shape, element, GeometryMode{});

        if(actual != expected) {
            std::cout << element << " expected = " << expected << "\n";
        }

        BOOST_CHECK(actual == expected);
    }
}


BOOST_AUTO_TEST_CASE(MorphIndexQueries) {
    if(mpi::rank(MPI_COMM_WORLD) != 0) {
        return;
    }

    auto n_elements = identifier_t(1000);
    auto domain = std::array<CoordType, 2>{-10.0, 10.0};

    auto elements = random_elements<Segment>(n_elements, domain, 0);

    auto index = IndexTree<Segment>();
    index.insert(elements.begin(), elements.end());

    // Large-ish sphere, its bounding box is:
    //   [-7.0, -7.0, -7.0] x [1.0, 1.0, 1.0]
    auto query_shape = Sphere{{-3.0, -3.0, -3.0}, 4.0};

    check_queries_against_geometric_primitives(elements, index, query_shape);
}


BOOST_AUTO_TEST_CASE(MorphMultiIndexQueries) {
    auto output_dir = "tmp-ndwiu";

    int n_required_ranks = 2;
    auto comm = mpi::comm_shrink(MPI_COMM_WORLD, n_required_ranks);

    if(*comm == MPI_COMM_NULL) {
        return;
    }

    auto n_elements = identifier_t(1000);
    auto domain = std::array<CoordType, 2>{-10.0, 10.0};

    auto mpi_rank = mpi::rank(*comm);
    auto elements = random_elements<Segment>(n_elements, domain, mpi_rank * n_elements);
    auto all_elements = gather_elements(elements, *comm);

    auto builder = MultiIndexBulkBuilder<Segment>(output_dir);
    builder.insert(elements.begin(), elements.end());
    builder.finalize(*comm);

    if(mpi_rank == 0) {
        auto index = MultiIndexTree<Segment>(output_dir, /* mem = */ size_t(1e6));

        // Large-ish sphere.
        auto query_shape = Sphere{{-3.0, -3.0, -3.0}, 4.0};

        check_queries_against_geometric_primitives(all_elements, index, query_shape);
    }
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
