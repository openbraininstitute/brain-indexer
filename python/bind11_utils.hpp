#pragma once
#include <memory>
#include <vector>

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
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence&& seq) {
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
inline py::array_t<typename Sequence::value_type> to_pyarray(const Sequence& sequence) {
    return py::array(sequence.size(), sequence.data());
}

}  // namespace pybind_utils
