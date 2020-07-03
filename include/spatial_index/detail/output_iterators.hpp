#pragma once
// Set of query output iterators.
// These are specialized iterators which can / should be used
// in place of back_inserter() and will either
//   (1) execute a callback (callback_iterator)
// 	 (2) Retieve the index / index &segment from the Tree entry

#include "../index.hpp"

namespace spatial_index {

namespace detail {

template <typename RT>
struct iter_append_only {
    typedef std::output_iterator_tag iterator_category;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;
    inline RT& operator*()     { return *static_cast<RT*>(this); }
    inline RT& operator++()    { return *static_cast<RT*>(this); }
    inline RT& operator--(int) { return *static_cast<RT*>(this); }
};


template <typename S>
inline identifier_t get_id_from(IndexedShape<S, ShapeId> const& obj) { return obj.id; }

template <typename S>
inline identifier_t get_id_from(IndexedShape<S, MorphPartId> const& obj) { return obj.gid(); }

template <typename... S>
inline identifier_t get_id_from(boost::variant<S...> const& obj) {
    return boost::apply_visitor([](const auto& t) { return t.gid(); }, obj);
}


// To automatically extract id / {id, segm_id} we map types to the id getter class
// Since we don't want to inherit from b::variant to just set id_getter_t
// we define a helper class which defines `type` if it's possible to get an id_getter

template <typename S>
struct id_getter_for {
    using type = typename S::id_getter_t;
};

template <typename S1, typename... S>
struct id_getter_for<boost::variant<S1, S...>> {
    using type = typename id_getter_for<S1>::type;
};



}  // namespace detail



template <typename ArgT>
struct iter_callback: public detail::iter_append_only<iter_callback<ArgT>> {
    iter_callback(const std::function<void(const ArgT&)>& func)
        : f_(func) {}

    inline iter_callback<ArgT>& operator=(const ArgT& v) {
        f_(v);
        return *this;
    }

  private:
    // Keep the const ref to the function. Func must get items by const-ref
    const std::function<void(const ArgT&)>& f_;
};


struct iter_ids_getter: public detail::iter_append_only<iter_ids_getter> {
    using value_type = identifier_t;

    iter_ids_getter(std::vector<identifier_t>& output)
        : output_(output) {}

    template <typename T>
    inline iter_ids_getter& operator=(const T& result_entry) {
        output_.push_back(detail::get_id_from(result_entry));
        return *this;
    }

  private:
    // Keep a ref to modify the original vec
    std::vector<identifier_t>& output_;
};


struct iter_gid_segm_getter: public detail::iter_append_only<iter_gid_segm_getter> {
    using value_type = gid_segm_t;

    iter_gid_segm_getter(std::vector<gid_segm_t>& output)
        : output_(output) {}

    template <typename S>
    inline iter_gid_segm_getter& operator=(const IndexedShape<S, MorphPartId>& result_entry) {
        output_.emplace_back(result_entry.gid(), result_entry.segment_i());
        return *this;
    }

    template <typename... ManyT>
    inline iter_gid_segm_getter& operator=(const boost::variant<ManyT...>& v) {
        output_.emplace_back(boost::apply_visitor(
            [](const auto& t) {
                return gid_segm_t{t.gid(), t.segment_i()};
            },
            v));
        return *this;
    }

  private:
    std::vector<gid_segm_t>& output_;
};

}  // namespace spatial_index
