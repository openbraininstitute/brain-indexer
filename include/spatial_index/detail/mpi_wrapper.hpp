#pragma once

#if SI_MPI == 1

#include <iostream>

namespace spatial_index {
namespace mpi {

template <class Derived, class Handle>
Resource<Derived, Handle>::Resource(Handle handle)
    : handle_(handle) {}


template <class Derived, class Handle>
Resource<Derived, Handle>::Resource(Resource&& other) noexcept
    : handle_(other.drop_ownership()) {}


template <class Derived, class Handle>
Resource<Derived, Handle>::~Resource() {
    if(handle_ != Derived::invalid_handle()) {
        Derived::free(handle_);
    }
}


template <class Derived, class Handle>
Resource<Derived, Handle>&
Resource<Derived, Handle>::operator=(Resource&& other) noexcept {
    handle_ = other.drop_ownership();
    return (*this);
}


template <class Derived, class Handle>
Handle Resource<Derived, Handle>::operator*() const {
    return handle_;
}


template <class Derived, class Handle>
Handle Resource<Derived, Handle>::drop_ownership() {
    auto tmp = handle_;
    handle_ = Derived::invalid_handle();

    return tmp;
}


template <class T>
Datatype create_contiguous_datatype() {
    MPI_Datatype datatype_;
    MPI_Type_contiguous(sizeof(T), MPI_BYTE, &datatype_);
    MPI_Type_commit(&datatype_);

    return Datatype{datatype_};
}



}
}

#endif
