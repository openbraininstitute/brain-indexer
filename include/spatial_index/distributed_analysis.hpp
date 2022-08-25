#pragma once

#include <vector>

#include <spatial_index/mpi_wrapper.hpp>
#include <spatial_index/logging.hpp>

namespace spatial_index {


inline std::vector<size_t> histogram(const std::vector<CoordType>& bins,
                                     std::vector<CoordType> data,
                                     MPI_Comm comm) {

    auto mpi_rank = mpi::rank(comm);

    std::sort(data.begin(), data.end());

    auto counts = std::vector<size_t>(bins.size(), 0ul);
    size_t current_bin_idx = 0;
    for(auto x : data) {
        while(x >= bins[current_bin_idx]) {
            ++current_bin_idx;
        }

        ++counts[current_bin_idx];
    }

    MPI_Reduce(
        (mpi_rank == 0) ? MPI_IN_PLACE : counts.data(),
        counts.data(),
        util::safe_integer_cast<int>(bins.size()),
        mpi::datatype<size_t>(),
        MPI_SUM,
        0,
        comm
    );

    return (mpi_rank == 0) ? counts : std::vector<size_t>{};
}


std::vector<CoordType> log10_bins(CoordType log10_low,
                                  CoordType log10_high,
                                  size_t bins_per_decade) {

    auto n_decades = log10_high - log10_low;
    auto n_bins = 2 + bins_per_decade * n_decades;
    auto bin_width = CoordType(1.0) / bins_per_decade;
    auto bins = std::vector<CoordType>(n_bins);

    for(size_t i = 0; i < n_bins-1; ++i) {
        bins[i] = std::pow(
            CoordType(10.0),
            CoordType(log10_low + i * bin_width)
        );
    }
    bins[n_bins-1] = std::numeric_limits<CoordType>::max();

    return bins;
}


void segment_length_histogram(const std::string &output_dir, MPI_Comm comm = MPI_COMM_WORLD) {
    auto storage = NativeStorageT<MorphoEntry>(output_dir);
    auto top_tree = storage.load_top_tree();
    auto n_subtrees = top_tree.size();

    auto comm_size = mpi::size(comm);
    auto comm_rank = mpi::rank(comm);

    auto chunk = util::balanced_chunks(n_subtrees, comm_size, comm_rank);
    auto data = std::vector<CoordType>{};
    for(size_t i = chunk.low; i < chunk.high; ++i) {
        log_info(boost::format("loading: %d") % i);
        auto subtree = storage.load_subtree(i); 

        for(const auto &value : subtree) {
            data.push_back(characteristic_length(value));
        }
    }

    auto bins = log10_bins(CoordType(-8), CoordType(6), 4ul);
    auto counts = histogram(bins, std::move(data), comm);

    MPI_Barrier(comm);

    if(mpi::rank(comm) == 0) {
        std::cout << std::setprecision(4) << std::scientific;
        std::cout << "[      -inf, " << bins[0] << "): " << counts[0] << "\n";
        for(size_t i = 1; i < bins.size()-1; ++i) {
            std::cout << "[" << bins[i-1] << ", " << bins[i] << "): "  << counts[i] << "\n";
        }
        std::cout << "[" << bins[bins.size()-2] << ",        inf): " << counts.back() << "\n";
    }
}

}
