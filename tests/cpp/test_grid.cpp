#define BOOST_TEST_MODULE SpatialIndex_Benchmarks
#include <boost/test/unit_test.hpp>
#include <spatial_index/index_grid.hpp>

namespace spatial_index {

template<>
inline std::array<int, 3> floor(const int& value, int divisor) {
    return {int(std::floor(double(value) / divisor)), 0, 0};
}

}  // namespace


using namespace spatial_index;


BOOST_AUTO_TEST_CASE(BasicTest) {
    SpatialGrid<int, 5> grid;

    grid.insert(1);
    grid.insert(3);
    grid.insert(6);
    grid.insert(-1);

    grid.print();
}


BOOST_AUTO_TEST_CASE(MorphoEntryTest) {
    SpatialGrid<MorphoEntry, 5> grid;

    grid.insert(Soma(0ul, Point3D{1, 2, 3}, 2.f));

    grid.print();
}
