#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>

#include <brain_indexer/util.hpp>


using namespace brain_indexer;


BOOST_AUTO_TEST_CASE(IdentityVector) {
    util::identity<> ids(100);

    BOOST_TEST( ids.size() == 100 );

    for (size_t i : {0, 1, 2, 3, 50, 98, 99, 110}) {
        BOOST_TEST(ids[i] == i);
    }
}


BOOST_AUTO_TEST_CASE(ConstantVector) {
    constexpr size_t K = 27;
    util::constant<> ids(K, 100);

    BOOST_TEST( ids.size() == 100 );

    for (size_t i : {0, 1, 2, 3, 50, 98, 99, 110}) {
        BOOST_TEST(ids[i] == K);
    }
}



BOOST_AUTO_TEST_CASE(SOA_Reader) {
    struct S { int a, b; };
    std::vector<int> v1{1, 2, 3, 4}, v2{5, 6, 7, 8};
    auto soa = util::make_soa_reader<S>(v1, v2);

    for (size_t i=0; i < v1.size(); i++) {
        auto item = soa[i];
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
}


BOOST_AUTO_TEST_CASE(SOA_Iterator) {
    struct S { int a, b; };
    std::vector<int> v1{1, 2, 3, 4}, v2{5, 6, 7, 8};
    auto soa = util::make_soa_reader<S>(v1, v2);

    int i=0;
    for(auto iter=soa.begin(); iter < soa.end(); ++iter, i++) {
        const auto item = *iter;
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
    BOOST_TEST( i == 4 );

    // Awesome range loops
    i=0;
    for(auto item : soa) {
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
        i++;
    }
}


struct S {
    int a, b;
    inline S(int a_, int b_) noexcept : a(a_), b(b_) { printf("CTOR!\n"); }
    inline S(const S& rhs) noexcept : a(rhs.a), b(rhs.b) { printf("COPY Ctor!\n"); }
    inline S(S&& rhs) noexcept : a(rhs.a), b(rhs.b) { printf("MOVE Ctor!\n"); }
    inline S& operator=(const S& rhs) noexcept { a = rhs.a; b = rhs.b; printf("COPY (=)!\n"); return *this; }
    inline S& operator=(S&& rhs) noexcept{ a = rhs.a; b = rhs.b; printf("MOVE (=)\n"); return *this; }
    inline ~S() noexcept { printf("Destroy!\n"); };

    template <typename... U>
    inline S(std::tuple<U...> tup)
        : S(std::get<0>(tup), std::get<1>(tup)) { printf("Using Tuple directly!\n"); }
};


BOOST_AUTO_TEST_CASE(SOA_Performance) {
    std::vector<int> v1{1, 2}, v2{5, 6};
    auto soa = util::make_soa_reader<S>(v1, v2);
    std::vector<S> vec;
    vec.reserve(v1.size());

    {
        printf("Test 0\n");
        for(auto iter=soa.begin(); iter < soa.end(); ++iter) {
            vec.emplace_back(*iter);
        }
        vec.clear();
    }

    {
        printf("Test 1\n");
        for(auto iter=soa.begin(); iter < soa.end(); ++iter) {
            vec.emplace_back(iter.get_tuple());  // Will be able to construct inplace?
        }
        vec.clear();
    }

    {
        printf("Test 2 \n");
        for(auto item : soa) {
            vec.emplace_back(std::move(item));
        }
    }
}
