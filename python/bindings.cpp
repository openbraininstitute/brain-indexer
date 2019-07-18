
#include <pybind11/pybind11.h>


extern void bind_rtree_soma(pybind11::module &m);
extern void bind_rtree(pybind11::module &m);


PYBIND11_MODULE(_spatial_index, m) {
    bind_rtree_soma(m);
    bind_rtree(m);
}
