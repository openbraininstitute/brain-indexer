#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>

#include <random>
#include <vector>
#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>

using namespace spatial_index;

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

BOOST_AUTO_TEST_CASE(MorphIndexQueries) {
    auto n_elements = identifier_t(1000);
    auto domain = std::array<CoordType, 2>{-10.0, 10.0};

    auto elements = random_elements<Segment>(n_elements, domain, 0);

    auto index = IndexTree<Segment>();
    index.insert(elements.begin(), elements.end());

    // Large-ish sphere, its bounding box is:
    //   [-7.0, -7.0, -7.0] x [1.0, 1.0, 1.0]
    auto query_shape = Sphere{{-3.0, -3.0, -3.0}, 4.0};

    std::vector<Segment> found;
    index.find_intersecting(query_shape, std::back_inserter(found));

    auto intersecting = std::unordered_map<identifier_t, bool>{};

    for(const auto &element : elements) {
        if(intersecting.find(element.gid()) != intersecting.end()) {
            // This signals a faulty assuption in the test logic; and the test
            // needs to be rewritten.
            throw std::runtime_error("Ids aren't unique.");
        }

        intersecting[element.gid()] = false;
    }

    for(const auto &element : found) {
        intersecting[element.gid()] = true;
    }

    for(const auto &kv : intersecting) {
        auto id = kv.first;
        auto actual = kv.second;
        const auto &element = elements.at(id);

        auto expected = query_shape.intersects(element);

        if(actual != expected) {
            std::cout << element << "\n";
        }

        BOOST_CHECK(actual == expected);
    }
}
