#define BOOST_TEST_MODULE BrainIndexer_UnitTests
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <vector>
#include <brain_indexer/query_ordering.hpp>

using namespace brain_indexer;

BOOST_AUTO_TEST_CASE(SFCOrderingTest) {
    std::vector<Point3Dx> points{
        Point3Dx{1.0, 2.0, 3.0},
        Point3Dx{2.0, 2.0, 3.0},
        Point3Dx{4.0, 2.0, 1.0},
        Point3Dx{1.1, 2.0, 3.0}
    };

    auto order = experimental::space_filling_order(points);

    std::map<size_t, size_t> counts;
    for(auto i : order) {
        ++counts[i];
    }

    BOOST_CHECK(counts.size() == points.size());
    for(size_t i = 0; i < points.size(); ++i) {
        BOOST_CHECK(counts[i] == 1ul);
    }
}