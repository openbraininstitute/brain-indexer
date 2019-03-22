#pragma once

// Define a function object converting a value_type of indexed Range into std::pair<>.
// This is a generic implementation but of course it'd be possible to use some
// specific types. One could also take Value as template parameter and access
// first_type and second_type members, etc.
template <typename First, typename Second>
struct pair_maker
{
    typedef std::pair<First, Second> result_type;
    template<typename T>
    inline result_type operator()(T const& v) const
    {
        return result_type(v.value(), v.index());
    }
};
