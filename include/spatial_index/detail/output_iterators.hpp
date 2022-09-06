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


template <typename S, typename IdType>
inline identifier_t get_id_from(IndexedShape<S, IdType> const& obj) {
    return obj.id;
}

template <typename S>
inline identifier_t get_id_from(IndexedShape<S, MorphPartId> const& obj) {
    return obj.gid();
}

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

// Three overloaded functions to export endpoints.
// These are added mainly to allow export as numpy.
// Depending on the object they can return a quiet_NaN
// as a second endpoint if the object doesn't have an endpoint.
// In that case the centroid will be returned as the first endpoint.

inline Point3D get_endpoint(const Soma& sphere, bool first) {
    constexpr auto nan = std::numeric_limits<CoordType>::quiet_NaN();
    return first ? sphere.centroid : Point3D{nan, nan, nan};
}

inline Point3D get_endpoint(const Segment& seg, bool first) {
    return first ? seg.p1 : seg.p2;
}


// Structures that contains the results of a query.
// Necessary to export data as numpy arrays.

enum class entry_kind {SOMA, SEGMENT, SYNAPSE};

inline entry_kind get_entry_kind(const Sphere&) {
    return entry_kind::SYNAPSE;
}

inline entry_kind get_entry_kind(const Segment&) {
    return entry_kind::SEGMENT;
}

inline entry_kind get_entry_kind(const Soma&) {
    return entry_kind::SOMA;
}

template<typename Element>
struct query_result;

template<>
struct query_result<MorphoEntry> {
    std::vector<identifier_t> gid;
    std::vector<unsigned> section_id;
    std::vector<unsigned> segment_id;
    std::vector<gid_segm_t> ids;
    std::vector<Point3D> centroid;
    std::vector<CoordType> radius;
    std::vector<Point3D> endpoint1;
    std::vector<Point3D> endpoint2;
    std::vector<entry_kind> kind;
};

template<>
struct query_result<Synapse> {
    std::vector<identifier_t> id;
    std::vector<identifier_t> pre_gid;
    std::vector<identifier_t> post_gid;
    std::vector<Point3D> position;
    std::vector<entry_kind> kind;
};

template<>
struct query_result<IndexedSphere> {
    std::vector<identifier_t> id;
    std::vector<Point3D> centroid;
    std::vector<CoordType> radius;
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
        output_.emplace_back(result_entry.gid(), result_entry.section_id(), result_entry.segment_id());
        return *this;
    }

    template <typename... ManyT>
    inline iter_gid_segm_getter& operator=(const boost::variant<ManyT...>& v) {
        output_.emplace_back(boost::apply_visitor(
            [](const auto& t) {
                return gid_segm_t{t.gid(), t.section_id(), t.segment_id()};
            },
            v));
        return *this;
    }

  private:
    std::vector<gid_segm_t>& output_;
};

// Iterators to fetch all data from the payload of segments, soma and synapses.
// Exports all the fields of the payload as query result object i.e. a struct of arrays.
// Used to fetch data to export as numpy arrays.

template<typename Entry>
struct iter_entry_getter;

template<>
struct iter_entry_getter<MorphoEntry> : public detail::iter_append_only<iter_entry_getter<MorphoEntry>> {
    using element_t = MorphoEntry;
    using result_t = detail::query_result<MorphoEntry>;

    iter_entry_getter(result_t& output)
        : output_(output) {}

    inline iter_entry_getter& operator=(const element_t& element) { 
        boost::apply_visitor(
            [this](const auto& t) {
                output_.gid.push_back(t.gid());
                output_.section_id.push_back(t.section_id());
                output_.segment_id.push_back(t.segment_id());
                output_.ids.push_back(gid_segm_t{t.gid(), t.section_id(), t.segment_id()});
                output_.centroid.push_back(t.get_centroid());
                output_.radius.push_back(t.radius);
                output_.endpoint1.push_back(detail::get_endpoint(t, 1));
                output_.endpoint2.push_back(detail::get_endpoint(t, 0));
                output_.kind.push_back(detail::get_entry_kind(t));
            },
            element);
        return *this;
    }

  private:
    result_t& output_;
};

template<>
struct iter_entry_getter<Synapse> : public detail::iter_append_only<iter_entry_getter<Synapse>> {
    using element_t = Synapse;
    using result_t = detail::query_result<element_t>;

    iter_entry_getter(result_t& output)
        : output_(output) {}

    inline iter_entry_getter& operator=(const element_t& element) {
        output_.id.push_back(element.id);
        output_.pre_gid.push_back(element.pre_gid_);
        output_.post_gid.push_back(element.post_gid_);
        output_.position.push_back(element.get_centroid());
        output_.kind.push_back(detail::get_entry_kind(element));
        return *this;
    }

  private:
    result_t& output_;
};


template<>
struct iter_entry_getter<IndexedSphere> : public detail::iter_append_only<iter_entry_getter<IndexedSphere>> {
    using element_t = IndexedSphere;
    using result_t = detail::query_result<element_t>;

    iter_entry_getter(result_t& output)
        : output_(output) {}

    inline iter_entry_getter& operator=(const element_t& element) {
        output_.id.push_back(element.id);
        output_.centroid.push_back(element.centroid);
        output_.radius.push_back(element.radius);
        return *this;
    }

  private:
    result_t& output_;
};

}  // namespace spatial_index
