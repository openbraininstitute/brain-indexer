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


BOOST_AUTO_TEST_CASE(SOA_Iterator_forward) {
    struct S { int a, b; };
    std::vector<int> v1{1, 2, 3, 4}, v2{5, 6, 7, 8};
    auto soa = util::make_soa_reader<S>(v1, v2);

    size_t i = 0;
    for(auto iter=soa.begin(); iter < soa.end(); ++iter, i++) {
        const auto item = *iter;
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
    BOOST_TEST( i == 4 );

    // Awesome range loops
    i = 0;
    for(auto item : soa) {
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
        i++;
    }
}


BOOST_AUTO_TEST_CASE(SOA_Iterator_backward) {
    struct S { int a, b; };
    std::vector<int> v1{1, 2, 3, 4}, v2{5, 6, 7, 8};
    auto soa = util::make_soa_reader<S>(v1, v2);

    size_t i = soa.size() - 1;
    for(auto iter = soa.end() - 1; iter > soa.begin(); --iter, i--) {
        const auto item = *iter;
        BOOST_TEST(item.a == v1[i]);
        BOOST_TEST(item.b == v2[i]);
    }
    BOOST_TEST(i == 0);
}


BOOST_AUTO_TEST_CASE(SOA_Iterator_arithmetic) {
    struct S { int a, b; };
    std::vector<int> v1{1, 2, 3, 4}, v2{5, 6, 7, 8};
    auto soa = util::make_soa_reader<S>(v1, v2);

    // operator+
    auto iter1 = soa.begin();
    auto iter2 = iter1 + 3;
    BOOST_TEST((*iter2).a == 4);
    BOOST_TEST((*iter2).b == 8);

    // operator+=
    auto iter3 = soa.begin();
    iter3 += 2;
    BOOST_TEST((*iter3).a == 3);
    BOOST_TEST((*iter3).b == 7);

    // operator-
    auto iter4 = soa.begin() + 3;
    auto iter5 = iter4 - 2;
    BOOST_TEST((*iter5).a == 2);
    BOOST_TEST((*iter5).b == 6);

    // operator-=
    auto iter6 = soa.begin() + 3;
    iter6 -= 2;
    BOOST_TEST((*iter6).a == 2);
    BOOST_TEST((*iter6).b == 6);

    // operator--
    auto iter7 = soa.begin() + 3;
    --iter7;
    BOOST_TEST((*iter7).a == 3);
    BOOST_TEST((*iter7).b == 7);

    // operator==
    auto iter10 = soa.begin() + 2;
    auto iter11 = soa.end() - 2;
    bool eq_result = (iter10 == iter11);
    BOOST_TEST(eq_result == true);

    // operator!=
    auto iter12 = soa.begin();
    auto iter13 = soa.begin() + 1;
    bool neq_result = (iter12 != iter13);
    BOOST_TEST(neq_result == true);

    // operator<
    bool lt_result = (soa.begin() < soa.end());
    BOOST_TEST(lt_result == true);

    // operator<=
    bool lte_result1 = (soa.begin() <= soa.end());
    BOOST_TEST(lte_result1 == true);
    bool lte_result2 = (soa.begin() <= soa.begin());
    BOOST_TEST(lte_result2 == true);

    // operator>
    bool gt_result = (soa.end() > soa.begin());
    BOOST_TEST(gt_result == true);

    // operator>=
    bool gte_result1 = (soa.end() >= soa.begin());
    BOOST_TEST(gte_result1 == true);
    bool gte_result2 = (soa.begin() >= soa.begin());
    BOOST_TEST(gte_result2 == true);

    // operator[] subscript
    auto iter14 = soa.begin() + 1;
    BOOST_TEST(iter14[0].a == 2);
    BOOST_TEST(iter14[2].a == 4);

    /* Checks for signed difference_type vs unsigned size_t */
    // iterator difference (positive)
    auto diff1 = (soa.begin() + 3) - soa.begin();
    BOOST_TEST(diff1 == 3);

    // iterator difference (negative)
    auto diff2 = soa.begin() - (soa.begin() + 2);
    BOOST_TEST(diff2 == -2);

    // negative offsets with operator+
    auto iter15 = soa.begin() + 3;
    auto iter16 = iter15 + (-2);
    BOOST_TEST((*iter16).a == 2);
    BOOST_TEST((*iter16).b == 6);

    // negative offsets with operator+=
    auto iter17 = soa.begin() + 3;
    iter17 += -1;
    BOOST_TEST((*iter17).a == 3);
    BOOST_TEST((*iter17).b == 7);

    // negative offsets with operator- (should move forward)
    auto iter18 = soa.begin() + 1;
    auto iter19 = iter18 - (-2);
    BOOST_TEST((*iter19).a == 4);
    BOOST_TEST((*iter19).b == 8);

    // negative offsets with operator-= (should move forward)
    auto iter20 = soa.begin() + 1;
    iter20 -= -2;
    BOOST_TEST((*iter20).a == 4);
    BOOST_TEST((*iter20).b == 8);

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
