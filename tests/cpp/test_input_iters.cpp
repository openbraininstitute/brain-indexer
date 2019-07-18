#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>

#include <numeric>
#include <spatial_index/util.hpp>


using namespace spatial_index;


BOOST_AUTO_TEST_CASE(IdentityVector) {
    util::identity ids(100);

    BOOST_TEST( ids.size() == 100 );

    for (size_t i : {0, 1, 2, 3, 50, 98, 99, 110}) {
        BOOST_TEST(ids[i] == i);
    }
}


BOOST_AUTO_TEST_CASE(ConstantVector) {
    constexpr size_t K = 27;
    util::constant<K> ids(100);

    BOOST_TEST( ids.size() == 100 );

    for (size_t i : {0, 1, 2, 3, 50, 98, 99, 110}) {
        BOOST_TEST(ids[i] == K);
    }
}


#define INIT_SOA_DATA     \
    struct S {            \
        int a, b;         \
    };                    \
                          \
    std::vector<int>      \
        v1{1, 2, 3, 4},   \
        v2{5, 6, 7, 8};


BOOST_AUTO_TEST_CASE(SOA_Reader) {
    INIT_SOA_DATA;
    auto soa = util::make_soa_reader<S>(v1, v2);

    for (int i; i < v1.size(); i++) {
        auto item = soa.get(i);
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
}


BOOST_AUTO_TEST_CASE(SOA_Iterator) {
    INIT_SOA_DATA;
    auto soa = util::make_soa_reader<S>(v1, v2);

    int i=0;
    for(auto iter=soa.begin(); iter < soa.end(); ++iter, i++) {
        const auto item = *iter;
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
    BOOST_TEST( i == 4 );
}

