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

using EveryEntry = boost::variant<IndexedSubtreeBox, Soma, Segment>;

template<class Element>
identifier_t get_id(const Element &element) {
    return element.gid();
}


template<class... VariantArgs>
identifier_t get_id(const boost::variant<VariantArgs...>& variant) {
    return boost::apply_visitor([](const auto &v){
        return get_id(v);
    },
    variant);
}


template<>
identifier_t get_id<IndexedSubtreeBox>(const IndexedSubtreeBox& element) {
    return element.id;
}


template<class Element>
static std::vector<Element>
random_shapes(size_t n_elements,
              const std::array<CoordType, 2> &domain,
              const std::array<CoordType, 2> &log_range,
              std::default_random_engine& gen);

template<>
std::vector<Box3D>
random_shapes<Box3D>(size_t n_shapes,
                     const std::array<CoordType, 2> &domain,
                     const std::array<CoordType, 2> &log_range,
                     std::default_random_engine& gen) {

    auto length = domain[1] - domain[0];

    auto pos_dist = std::uniform_real_distribution<CoordType>(domain[0], domain[1]);
    auto offset_dist = std::uniform_real_distribution<CoordType>(0.001*length, 1.0*length);
    auto log_length_dist = std::uniform_real_distribution<CoordType>(log_range[0], log_range[1]);

    auto shapes = std::vector<Box3D>{};
    shapes.reserve(n_shapes);

    for(size_t i = 0; i < n_shapes; ++i) {
        auto x = Point3Dx{pos_dist(gen), pos_dist(gen), pos_dist(gen)};
        auto dx = Point3Dx{offset_dist(gen), offset_dist(gen), offset_dist(gen)};
        auto l = std::pow(CoordType(10.0), log_length_dist(gen)) * length;

        shapes.emplace_back(x, x + l * dx/dx.norm());
    }

    return shapes;
}

template<>
std::vector<Sphere>
random_shapes<Sphere>(size_t n_shapes,
                      const std::array<CoordType, 2> &domain,
                      const std::array<CoordType, 2> &log_range,
                      std::default_random_engine& gen) {

    auto length = domain[1] - domain[0];

    auto pos_dist = std::uniform_real_distribution<CoordType>(domain[0], domain[1]);
    auto log_radius_dist = std::uniform_real_distribution<CoordType>(log_range[0], log_range[1]);

    auto shapes = std::vector<Sphere>{};
    shapes.reserve(n_shapes);

    for(size_t i = 0; i < n_shapes; ++i) {
        auto x = Point3Dx{pos_dist(gen), pos_dist(gen), pos_dist(gen)};
        auto r = std::pow(CoordType(10.0), log_radius_dist(gen)) * length;

        shapes.emplace_back(x, r);
    }

    return shapes;
}

template<>
std::vector<Cylinder>
random_shapes<Cylinder>(size_t n_shapes,
                        const std::array<CoordType, 2> &domain,
                        const std::array<CoordType, 2> &log_range,
                        std::default_random_engine& gen) {

    auto length = domain[1] - domain[0];

    auto pos_dist = std::uniform_real_distribution<CoordType>(domain[0], domain[1]);
    auto offset_dist = std::uniform_real_distribution<CoordType>(-1.0, 1.0);
    auto log_length_dist = std::uniform_real_distribution<CoordType>(log_range[0], log_range[1]);

    auto shapes = std::vector<Cylinder>{};
    shapes.reserve(n_shapes);

    for(size_t i = 0; i < n_shapes; ++i) {
        auto l = std::pow(CoordType(10.0), log_length_dist(gen)) * length;
        auto x = Point3Dx{pos_dist(gen), pos_dist(gen), pos_dist(gen)};
        auto dx = Point3Dx{offset_dist(gen), offset_dist(gen), offset_dist(gen)};
        auto r = std::pow(CoordType(10.0), log_length_dist(gen)) * length;

        shapes.emplace_back(x, x + l * dx, r);
    }

    return shapes;
}



template<class Element>
static std::vector<Element>
random_elements(size_t n_elements,
                const std::array<CoordType, 2> &domain,
                size_t id_offset,
                std::default_random_engine& gen);


template<>
std::vector<IndexedSubtreeBox>
random_elements<IndexedSubtreeBox>(size_t n_elements,
                                   const std::array<CoordType, 2> &domain,
                                   size_t id_offset,
                                   std::default_random_engine& gen) {

    auto boxes = random_shapes<Box3D>(n_elements, domain, {-6.0, -2.0}, gen);

    auto elements = std::vector<IndexedSubtreeBox>{};
    elements.reserve(n_elements);

    for(size_t i = 0; i < n_elements; ++i) {
        elements.emplace_back(SubtreeId{id_offset + i, 0ul}, boxes[i]);
    }

    return elements;
}

template<>
std::vector<Segment>
random_elements<Segment>(size_t n_elements,
                         const std::array<CoordType, 2> &domain,
                         size_t id_offset,
                         std::default_random_engine& gen) {

    auto cylinders = random_shapes<Cylinder>(n_elements, domain, {-6.0, -2.0}, gen);

    auto elements = std::vector<Segment>{};
    elements.reserve(n_elements);

    for(size_t i = 0; i < n_elements; ++i) {
        elements.emplace_back(MorphPartId{id_offset + i, 0u, 0u}, cylinders[i]);
    }

    return elements;
}


template<>
std::vector<Soma>
random_elements<Soma>(size_t n_elements,
                      const std::array<CoordType, 2> &domain,
                      size_t id_offset,
                      std::default_random_engine& gen) {

    auto spheres = random_shapes<Sphere>(n_elements, domain, {-6.0, -2.0}, gen);

    auto elements = std::vector<Soma>{};
    elements.reserve(n_elements);

    for(size_t i = 0; i < n_elements; ++i) {
        elements.emplace_back(MorphPartId{id_offset + i, 0u, 0u}, spheres[i]);
    }

    return elements;
}


template<>
std::vector<EveryEntry>
random_elements<EveryEntry>(size_t n_elements,
                            const std::array<CoordType, 2> &domain,
                            size_t id_offset,
                            std::default_random_engine& gen) {

    auto elements = std::vector<EveryEntry>{};
    elements.reserve(3*n_elements);

    auto subtrees = random_elements<IndexedSubtreeBox>(n_elements, domain, 3*id_offset, gen);
    auto somas = random_elements<Soma>(n_elements, domain, 3*id_offset + n_elements, gen);
    auto segments = random_elements<Segment>(n_elements, domain, 3*id_offset + 2*n_elements, gen);

    for(const auto& v : subtrees) {
        elements.push_back(v);
    }

    for(const auto& v : somas) {
        elements.push_back(v);
    }

    for(const auto& v : segments) {
        elements.push_back(v);
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


template<class GeometryMode, class Element, class Index, class QueryShape>
static void check_queries_against_geometric_primitives(
    const std::vector<Element> &elements,
    const Index &index,
    const QueryShape &query_shape) {

    std::vector<Element> found;
    index.template find_intersecting<GeometryMode>(query_shape, std::back_inserter(found));

    {
        auto actual = index.template count_intersecting<GeometryMode>(query_shape);
        auto expected = found.size();
        BOOST_CHECK_MESSAGE(
            actual == expected,
            "count_intersecting: query_shape = " << query_shape
                                                 << ", n_found = "
                                                 << actual << "/" << expected
        );
    }
    {
        auto actual = index.template is_intersecting<GeometryMode>(query_shape);
        auto expected = !found.empty();
        BOOST_CHECK_MESSAGE(
            actual == expected,
            "is_intersecting: query_shape = " << query_shape << ", actual = " << actual
        );
    }

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
        auto expected = bg::intersects(
                bgi::indexable<QueryShape>{}(query_shape),
                bgi::indexable<Element>{}(element))
            && geometry_intersects(query_shape, element, GeometryMode{});

        BOOST_CHECK_MESSAGE(actual == expected,
            "find_intersecting: query_shape = " << query_shape
                                                << ", element =" << element
                                                << ", expected = " << expected
        );
    }
}


template<class Element, class Index>
void check_with_all_query_shapes(
        const std::vector<Element>& all_elements,
        const Index& index,
        const std::array<CoordType, 2>& domain,
        std::default_random_engine& gen) {

    {
        auto query_shapes = random_shapes<Sphere>(20, domain, {-2.0, 1.0}, gen);
        for(const auto& query_shape : query_shapes) {
            check_queries_against_geometric_primitives<BoundingBoxGeometry>(all_elements, index, query_shape);
            check_queries_against_geometric_primitives<ExactGeometry>(all_elements, index, query_shape);
        }
    }

    {
        auto query_shapes = random_shapes<Box3D>(20, domain, {-2.0, 1.0}, gen);
        for(const auto& query_shape : query_shapes) {
            check_queries_against_geometric_primitives<BoundingBoxGeometry>(all_elements, index, query_shape);
            check_queries_against_geometric_primitives<ExactGeometry>(all_elements, index, query_shape);
        }
    }

    {
        auto query_shapes = random_shapes<Cylinder>(20, domain, {-2.0, 1.0}, gen);
        for(const auto& query_shape : query_shapes) {
            check_queries_against_geometric_primitives<BoundingBoxGeometry>(all_elements, index, query_shape);
            check_queries_against_geometric_primitives<ExactGeometry>(all_elements, index, query_shape);
        }
    }

    // In order to check for non-intersection we need a few small shapes as well.
}


BOOST_AUTO_TEST_CASE(MorphIndexQueries) {
    if(mpi::rank(MPI_COMM_WORLD) != 0) {
        return;
    }

    auto gen = std::default_random_engine{};
    auto n_elements = identifier_t(1000);
    auto domain = std::array<CoordType, 2>{-10.0, 10.0};

    auto elements = random_elements<EveryEntry>(n_elements, domain, 0, gen);
    auto index = IndexTree<EveryEntry>(elements);

    check_with_all_query_shapes(elements, index, domain, gen);
}


BOOST_AUTO_TEST_CASE(MultiIndexQueries) {
    auto output_dir = "tmp-ndwiu";

    int n_required_ranks = 2;
    auto comm = mpi::comm_shrink(MPI_COMM_WORLD, n_required_ranks);

    if(*comm == MPI_COMM_NULL) {
        return;
    }

    auto n_elements = identifier_t(1000);
    auto domain = std::array<CoordType, 2>{-10.0, 10.0};

    auto mpi_rank = mpi::rank(*comm);

    auto gen = std::default_random_engine{util::integer_cast<long unsigned int>(mpi_rank)};
    auto elements = random_elements<EveryEntry>(n_elements, domain, mpi_rank * n_elements, gen);
    auto all_elements = gather_elements(elements, *comm);

    auto builder = MultiIndexBulkBuilder<EveryEntry>(output_dir);
    builder.insert(elements.begin(), elements.end());
    builder.finalize(*comm);

    if(mpi_rank == 0) {
        auto index = MultiIndexTree<EveryEntry>(output_dir, /* mem = */ size_t(1e6));
        check_with_all_query_shapes(all_elements, index, domain, gen);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    auto return_code = bt::unit_test_main([](){ return true; }, argc, argv );

    MPI_Finalize();
    return return_code;
}
