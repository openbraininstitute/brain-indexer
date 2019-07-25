#define BOOST_TEST_MODULE SpatialIndex_Benchmarks
#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>

#include <numeric>
#include <spatial_index/index.hpp>
#include <spatial_index/util.hpp>


using namespace spatial_index;
using namespace boost::timer;


BOOST_AUTO_TEST_CASE(VariantUsingSOA) {

     //src data
    std::vector<unsigned long> ids(1e7);
    std::iota(ids.begin(), ids.end(), 0);
    std::vector<float> points(1e7 * 3);
    std::vector<float> radix(1e7);

    std::vector<Soma> somas;
    auto points_ptr = reinterpret_cast<const Point3D*>(points.data());

    {
        cpu_timer timer;
        somas.reserve(ids.size());

        for(size_t i = 0; i < ids.size(); i++)  {
            somas.emplace_back(ids[i], points_ptr[i], radix[i]);
        }

        IndexTree<MorphoEntry> rtree(somas);
        std::cout << "With intermediary array:" << timer.format() << std::endl;
    }

    {
        cpu_timer timer;
        auto soa=util::make_soa_reader<Soma>(ids, points_ptr, radix);

        IndexTree<MorphoEntry> rtree(soa.begin(), soa.end());
        std::cout << "Without Copy (SoA reader):" << timer.format() << std::endl;

    }
}