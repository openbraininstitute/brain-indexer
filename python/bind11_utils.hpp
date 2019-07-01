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

/**
 * @brief Python capsule handles void pointers to objects and makes sure
 *	that they will remain alive.
 *
 *	https://github.com/pybind/pybind11/issues/1042#issuecomment-325941022
 *
 * @param ptr [description]
 * @param p [description]
 * @return [description]
 */
template <typename Sequence>
inline py::array_t<typename Sequence::value_type> to_ndarray_copyless(Sequence* seq_ptr) {
    // Create a Python object that will free the allocated memory when destroyed:
    auto capsule = py::capsule(seq_ptr, [](void* p) { delete reinterpret_cast<Sequence*>(p); });

    return py::array(seq_ptr->size(),  // shape of array
                     seq_ptr->data(),  // c-style contiguous strides for Sequence
                     capsule           // numpy array references this parent
    );
}

template <typename T>
struct ArrayWrapper {
    using vector_t = std::vector<T>;

    vector_t* vector_ptr_;

    ArrayWrapper() {
        vector_ptr_ = new vector_t();
    }

    /* get the reference to the vector vector_ptr_ points to */
    inline vector_t& as_vector() const noexcept {
        return *vector_ptr_;
    }

    /**
     * @brief Return as array and give numpy the responsibility to deallocate the data
     * @details [long description]
     *
     * @param to_ndarray_copyless [description]
     * @return [description]
     */
    inline py::array_t<T> as_pyarray() const {
        return to_ndarray_copyless(vector_ptr_);
    }

    inline py::array_t<T> as_pyarray(size_t n_cols) const {
        size_t n_rows(size() / n_cols);

        auto out_array = as_pyarray();
        out_array.resize({n_rows, n_cols});

        return out_array;
    }

    inline size_t size() const noexcept {
        return vector_ptr_->size();
    }
};


/**
 * \brief Converts and STL Sequence to numpy array by copying i
 **/
template <typename Sequence>
inline py::array_t<typename Sequence::value_type> to_ndarray_copy(Sequence& sequence) {
    return py::array(sequence.size(), sequence.data());
}
