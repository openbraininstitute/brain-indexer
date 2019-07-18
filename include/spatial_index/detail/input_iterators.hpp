#pragma once

#include <utility>


namespace spatial_index {

namespace detail {


template <typename CRT, typename ValueT>
struct indexed_iterator_base {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = size_t;
    using value_type        = ValueT;
    using reference         = ValueT&;
    using pointer           = ValueT*;

    size_t i_;
    inline CRT& operator+=(size_t i) noexcept { i_ += i; return *static_cast<CRT*>(this); }
    inline CRT& operator-=(size_t i) noexcept { i_ += i; return *static_cast<CRT*>(this); }
    inline CRT& operator++() noexcept { return (*this)+=1; }
    inline CRT& operator--() noexcept { return (*this)-=1; }
    inline difference_type operator-(const CRT& rhs) const noexcept { return i_ - rhs.i_; }
    inline bool operator==(const CRT& rhs) const noexcept { return i_ == rhs.i_; }
    inline bool operator!=(const CRT& rhs) const noexcept  { return i_ != rhs.i_; }
    inline bool operator<(const CRT& rhs) const noexcept  { return i_ < rhs.i_; }

    // inline ValueT get(size_t i);  // To be implemented in subclass
    inline ValueT operator*() { return static_cast<CRT*>(this)->get(i_); }
    inline ValueT operator[](size_t i) { return static_cast<CRT*>(this)->get(i_ + i); }

};

}  // namespace detail



namespace util {

template <typename T, typename... Fields>
class SoA {
  public:
    struct iterator : public detail::indexed_iterator_base<iterator, const T> {
        using super = detail::indexed_iterator_base<iterator, const T>;

        inline iterator(const SoA& soa, size_t i)
            : super{i}
            , soa_(soa) {}

        inline T get(size_t i) {
            return soa_.get().get(i);
        }

    private:
        std::reference_wrapper<const SoA> soa_;  // Make the iterator copyable, swapable...
    };


    inline SoA(Fields&&... args) = delete;
    inline SoA(Fields&... args)
        : data_(args...) {}

    inline const iterator begin() const {
        return iterator(*this, 0);
    }
    inline const iterator end() const {
        return iterator(*this, std::get<0>(data_).size());
    }

    inline T get(size_t i) const {
        // Inspired by https://github.com/crosetto/SoAvsAoS/blob/master/main.cpp#L64
        return get_(i, std::make_integer_sequence<unsigned, sizeof...(Fields)>());
    }

  private:
    template <unsigned... Ids>
    inline T get_(size_t i, std::integer_sequence<unsigned, Ids...>) const {
        return T{std::get<Ids>(data_)[i]...};
    }

    std::tuple<const Fields&...> data_;
};


}  // namespace util

}  // namespace spatial_index
