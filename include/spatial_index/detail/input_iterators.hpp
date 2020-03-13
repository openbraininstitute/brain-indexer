#pragma once

#include <cstddef>
#include <functional>

namespace spatial_index {

namespace detail {


template <typename CRT, typename ValueT>
struct indexed_iterator_base {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = size_t;
    using value_type        = ValueT;
    using reference         = const ValueT&;  // Support Temporaries
    using pointer           = ValueT*;

    size_t i_;
    inline CRT& operator+=(size_t i) noexcept { i_ += i; return *static_cast<CRT*>(this); }
    inline CRT& operator-=(size_t i) noexcept { i_ += i; return *static_cast<CRT*>(this); }
    inline CRT& operator+(size_t i) && noexcept { return (*this)+=i; }
    inline CRT& operator-(size_t i) && noexcept { return (*this)+=i; }
    inline CRT& operator++() noexcept { return (*this)+=1; }
    inline CRT& operator--() noexcept { return (*this)-=1; }
    inline difference_type operator-(const CRT& rhs) const noexcept { return i_ - rhs.i_; }
    inline bool operator==(const CRT& rhs) const noexcept { return i_ == rhs.i_; }
    inline bool operator!=(const CRT& rhs) const noexcept  { return i_ != rhs.i_; }
    inline bool operator<(const CRT& rhs) const noexcept  { return i_ < rhs.i_; }

    // get(size_t i);  // To be implemented in subclass
    // Note: we use decltype because get() may return by value, by [const]ref...
    inline decltype(auto) operator*() const noexcept {
        return static_cast<const CRT*>(this)->get(i_);
    }
    inline decltype(auto) operator[](size_t i) const noexcept {
        return static_cast<const CRT*>(this)->get(i_ + i);
    }
};


template <typename SoA_T>
struct SoA_Iterator : public detail::indexed_iterator_base<SoA_Iterator<SoA_T>,
                                                           typename SoA_T::value_type> {
    inline SoA_Iterator(const SoA_T& soa, size_t i) noexcept
        : super{i}
        , soa_(soa) {}

    inline decltype(auto) get(size_t i) const {
        return soa_.get()[i];
    }

    inline decltype(auto) get_tuple() const {
        return soa_.get().get_tuple(this->i_);
    }

  private:
    using super = detail::indexed_iterator_base<SoA_Iterator<SoA_T>,
                                                typename SoA_T::value_type>;
    std::reference_wrapper<const SoA_T> soa_;  // Make the iterator copyable, swapable...
};

}  // namespace detail


namespace util {


template <typename T, typename... Fields>
class SoA {
  public:
    using value_type = T;
    using iterator = detail::SoA_Iterator<SoA<T, Fields...>>;

    inline SoA(Fields&&... args) = delete;
    inline SoA(Fields&... args)
        : data_{args...}
        , last_{*this, size()} {}

    inline iterator begin() const noexcept {
        return iterator(*this, 0);
    }

    inline const iterator& end() const noexcept {
        return last_;
    }

    inline size_t size() const noexcept {
        return std::get<0>(data_).size();
    }

    inline T operator[](size_t i) const {
        return get_(i, std::index_sequence_for<Fields...>{});
    }

    inline auto get_tuple(size_t i) const {
        return get_tuple_(i, std::index_sequence_for<Fields...>{});
    }

  private:
    template <size_t... Ids>
    inline T get_(size_t i, std::index_sequence<Ids...>) const {
        return {std::get<Ids>(data_)[i]...};
    }

    template <size_t... Ids>
    inline auto get_tuple_(size_t i, std::index_sequence<Ids...>) const {
        return std::forward_as_tuple(std::get<Ids>(data_)[i]...);
    }

    std::tuple<const Fields&...> data_;
    iterator last_;  // Very often accessed, especially in range-loops
};


}  // namespace util

}  // namespace spatial_index
