#pragma once

#include <memory>
#include <vector>
#include <sstream>

#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

namespace pybind_utils {


/**
 * @brief "Casts" a Cpp sequence to a python array (no memory copies)
 *  Python capsule handles void pointers to objects and makes sure
 *	that they will remain alive.
 *
 *	https://github.com/pybind/pybind11/issues/1042#issuecomment-325941022
 */
template <typename Sequence>
inline auto as_pyarray(Sequence&& seq) {
    // Move entire object to heap. Memory handled via Python capsule
    Sequence* seq_ptr = new Sequence(std::move(seq));
    // Capsule shall delete sequence object when done
    auto capsule = py::capsule(seq_ptr,
                               [](void* p) { delete reinterpret_cast<Sequence*>(p); });

    return py::array(seq_ptr->size(),  // shape of array
                     seq_ptr->data(),  // c-style contiguous strides for Sequence
                     capsule           // numpy array references this parent
    );
}


/**
 * \brief Converts and STL Sequence to numpy array by copying i
 */
template <typename Sequence>
inline auto to_pyarray(const Sequence& sequence) {
    return py::array(sequence.size(), sequence.data());
}


/**
 * \brief A class to obtain the underlying buffer of a stringbuffer
 * It matches the containers API for compat with as_pyarray()
 */
class StringBuffer : public std::stringbuf {
  public:
    inline std::stringbuf::char_type* data() const noexcept {
        return pbase();
    }

    inline std::size_t size() const noexcept {
        return std::size_t(pptr() - pbase());
    }
};


}  // namespace pybind_utils
