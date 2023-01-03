#define BOOST_TEST_MODULE SpatialIndex_UnitTests
#include <boost/test/unit_test.hpp>
namespace bt = boost::unit_test;

#include <memory>
#include <random>

#include <spatial_index/util.hpp>

using namespace spatial_index;


BOOST_AUTO_TEST_CASE(CheckThrowNoSuchFile) {
    std::string invalid_filename = "foo/bar.txt";
    BOOST_CHECK_THROW({
        try {
            util::open_ifstream(invalid_filename);
        }
        catch( const std::runtime_error& e) {
            std::string msg = e.what();
            BOOST_CHECK(msg.find(invalid_filename) != std::string::npos);

            throw;
        }
    }, std::runtime_error);
}
