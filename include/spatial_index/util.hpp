
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
struct identity {
    inline identity(size_t size = 0)
        : size_(size){};

    inline constexpr size_t operator[](size_t x) const noexcept {
        return x;
    }

    inline size_t size() const noexcept {
        return size_;
    }

  private:
    const size_t size_;
};


/// \brief Virtual array where each position always returns the same number
template <size_t X>
struct constant: public identity {
    using identity::identity;
    inline constexpr size_t operator[](int) const noexcept {
        return X;
    }
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


}  // namespace util
}  // namespace spatial_index


#include "detail/input_iterators.hpp"
