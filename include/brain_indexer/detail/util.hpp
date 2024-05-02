#pragma once

#include <vector>


namespace brain_indexer { namespace util {

inline std::vector<size_t> balanced_chunk_sizes(size_t global_count, size_t n_chunks) {
    size_t base_count = global_count / n_chunks;

    std::vector<size_t> balanced_count_per_rank(n_chunks, base_count);
    for(size_t i = 0; i < global_count % n_chunks; ++i) {
        balanced_count_per_rank[i]++;
    }

    return balanced_count_per_rank;
}


inline Range balanced_chunks(const Range& range, size_t n_chunks, size_t k_chunk) {
    size_t length = range.high - range.low;
    size_t chunk_size = length / n_chunks;
    size_t n_large_chunks = length % n_chunks;

    size_t low =  range.low + k_chunk     * chunk_size + std::min(k_chunk,   n_large_chunks);
    size_t high = range.low + (k_chunk+1) * chunk_size + std::min(k_chunk+1, n_large_chunks);

    return Range{std::min(low, range.high), std::min(high, range.high)};
}


inline Range balanced_chunks(size_t n_total, size_t n_chunks, size_t k_chunk) {
    return balanced_chunks(Range{0, n_total}, n_chunks, k_chunk);
}

}
}
