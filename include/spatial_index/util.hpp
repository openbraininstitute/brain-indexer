#pragma once

#include <boost/filesystem.hpp>
#include <boost/numeric/conversion/converter.hpp>

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


}  // namespace util
}  // namespace spatial_index


#include "detail/input_iterators.hpp"
