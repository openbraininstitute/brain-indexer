#pragma once

#include <spatial_index/distributed_analysis.hpp>

namespace spatial_index {
namespace py_bindings {

inline void create_call_some_mpi_from_cxx_bindings(py::module& m) {
    m.def("call_some_mpi_from_cxx",
          []() {
              auto rank = mpi::rank(MPI_COMM_WORLD);
              std::cout << "MPI: rank = " << rank << "\n";
          },
          R"(
          Calls some MPI from C++ to ensure there's no version mismatch
          )"
    );
}

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
}

}
}
