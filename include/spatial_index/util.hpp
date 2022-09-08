#pragma once

#include <boost/filesystem.hpp>
#include <boost/numeric/conversion/converter.hpp>

#include "logging.hpp"

namespace spatial_index {

namespace util {

/// Create an array of type T and fill with elements initialized from given arrays
template <typename T, std::enable_if_t<!std::is_trivial<T>::value, int> = 0, typename... Args>
std::vector<T> make_vec(int count, const Args&... args) {
    std::vector<T> v;
    v.reserve(count);
    for (int i = 0; i < count; i++) {
        v.emplace_back(args[i]...);
    }
    return v;
}

/// Create an array of type T. Overload for PODs.
/// emplace_back calls a 'regular' constructor. PODs dont have those
template <typename T, std::enable_if_t<std::is_trivial<T>::value, int> = 0, typename... Args>
std::vector<T> make_vec(int count, const Args&... args) {
    std::vector<T> v;
    v.reserve(count);
    for (int i = 0; i < count; i++) {
        v.push_back({args[i]...});
    }
    return v;
}


/// \brief Virtual array where each position returns the index
template <typename T=size_t>
struct identity {
    inline identity(size_t size = 0) noexcept
        : size_(size) {}

    inline constexpr T operator[](size_t x) const noexcept {
        return static_cast<T>(x);
    }

    inline size_t size() const noexcept {
        return size_;
    }

  protected:
    const size_t size_;
};


/// \brief Virtual array where each position always returns the same number
template <typename T=size_t>
struct constant: public identity<T> {
    inline constant(T x, size_t size = 0) noexcept
        : identity<T>{size}
        , x_(x) {}

    inline constexpr T operator[](size_t) const noexcept {
        return x_;
    }

  private:
    const T x_;
};


/// \Brief Iterator reading SOA and offering AOS interface
template <typename T, typename... Fields>
class SoA;


/**
 * /brief Helper for creating SOA_Iterators
 * /param fields: The various data structures (implementing []) to fill each field.
 *  Note: params must be lvalue references (const refs stored internally).
 */
template <typename T, typename... Fields>
inline auto make_soa_reader(Fields&... fields) {
    return SoA<T, Fields...>(fields...);
}


/** \brief Ensure that the output directory is valid.
 * 
 *  Either the directory already exists and is empty, or it's created now.
 */
inline void ensure_valid_output_directory(const std::string &output_dir) {
    if(boost::filesystem::is_directory(output_dir)) {
        if(!boost::filesystem::is_empty(output_dir)) {
            throw std::runtime_error("Not an empty directory: " + output_dir);
        }
    }
    else {
        boost::filesystem::create_directories(output_dir);
    }
}


/** \brief Safe conversion of integer types.
 * 
 *  Safe means this function will throw an exception if the conversion isn't
 *  exact.
 */
template<class T, class S>
T safe_integer_cast(S s) {
    static_assert(std::is_integral<T>::value, "The target type must be integral.");
    static_assert(std::is_integral<S>::value, "The source type must be integral.");

    return boost::numeric::converter<T, S>::convert(s);
}


/** \brief Cheap, unsafe conversion of integer types.
 * 
 *  In production this reverts to essentially a `static_cast`. However,
 *  when assertions are turned on as defined by `NDEBUG`, then this will
 *  check that the integer conversion is safe.
 * 
 *  Note, use this in performance critical parts of the code where the
 *  overhead of checking that the integer conversion is safe is
 *  unacceptable. Otherwise, use `safe_integer_cast`.
 */
template<class T, class S>
T integer_cast(S s) {
    static_assert(std::is_integral<T>::value, "The target type must be integral.");
    static_assert(std::is_integral<S>::value, "The source type must be integral.");

#ifndef NDEBUG
    return safe_integer_cast<T>(s);
#else
    return static_cast<T>(s);
#endif
}


/// Represents the range [low, high).
struct Range {
    size_t low;
    size_t high; ///< one-past the end
};


/** \brief Computes the size of perfectly balanced chunks.
 *
 * The fair chunk size of chunk `i` is element `i` in the
 * return value.
 *
 * \param global_count  The total number of elements.
 * \param n_chunks      Number of chunks.
 *
 */
std::vector<size_t> balanced_chunk_sizes(size_t global_count, size_t n_chunks);


/** \brief Split interval in almost equally sized chunks.
 *
 * The interval `[range.low, range.high)` is split into `n_chunks` parts. This
 * function returns the range of chunk `k_chunk`.
 */
inline Range balanced_chunks(const Range &range, size_t n_chunks, size_t k_chunk);


/// Split the interval `[0, n_total)` in almost equally sized chunks.
inline Range balanced_chunks(size_t n_total, size_t n_chunks, size_t k_chunk);


/// Now formatted as 'YYYY-MM-DDTHH:MM:SS'.
std::string iso_datetime_now() {
    // Credit: https://stackoverflow.com/a/9528166

    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09"];
    strftime(buf, sizeof buf, "%FT%T", gmtime(&now));

    return std::string(buf);
}

/// @brief Read the environment variable and convert it to `bool`.
bool read_boolean_environment_variable(const std::string& name) {
    char const * const var_c_str = std::getenv(name.c_str());

    if(var_c_str == nullptr) {
        return false;
    }

    auto var = std::string(var_c_str);
    if(var == "") {
        return false;
    }

    if(var == "0") {
        return false;
    }

    if(var == "Off" || var == "off" || var == "OFF") {
        return false;
    }

    if(var == "1") {
        return true;
    }

    if(var == "On" || var == "on" || var == "ON") {
        return true;
    }

    log_warn("Ambiguous value for environment variable: " + name + ". Please"
             " use `0`, `Off`; or `1`, `On`. Defaulting to: true.");

    return true;
}


}  // namespace util
}  // namespace spatial_index


namespace spatial_index {
namespace py_bindings {

#if SI_FOR_PYBIND == 1
// Defined in `py_bindings.cpp`.
void check_signals();
#endif

} // spatial_index::py_bindings

namespace util {

void check_signals() {
#if SI_FOR_PYBIND == 1
    spatial_index::py_bindings::check_signals();
#endif
}

} // spatial_index::util
} // spatial_index




#include "detail/util.hpp"
#include "detail/input_iterators.hpp"
