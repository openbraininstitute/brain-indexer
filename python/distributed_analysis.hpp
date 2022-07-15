#pragma once

#include <spatial_index/distributed_analysis.hpp>

namespace spatial_index {
namespace py_bindings {

inline void create_analysis_bindings(py::module& m) {
    m.def("segment_length_histogram",
         [](std::string output_dir) {
            segment_length_histogram(output_dir, MPI_COMM_WORLD);
         },
         R"(
         Computes a histogram of the segment lengths.

         This is an MPI collective operation.
         )"
         );

    m.def("inspect_bad_cases",
         []() {
            inspect_bad_cases();
         },
         R"(
         For debugging purposes only.
         )"
         );
}

}
}
