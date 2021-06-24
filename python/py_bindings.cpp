
#include "rtree_index.hpp"
#include "index_grid.hpp"

#ifndef SI_GRID_VOXEL_LENGTH
#define SI_GRID_VOXEL_LENGTH 10
#endif

namespace si_python = si::py_bindings;

namespace spatial_index { namespace py_bindings {

using SphereGridT = si::SpatialGrid<si::IndexedSphere, SI_GRID_VOXEL_LENGTH>;
using MorphGridT = si::MorphSpatialGrid<SI_GRID_VOXEL_LENGTH>;

}}  // namespace spatial_index::py_bindings


PYBIND11_MODULE(_spatial_index, m) {
    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, section_id, segment_id);  // struct as numpy dtype

    si_python::create_IndexTree_bindings<si::IndexedSphere>(m, "SphereIndex");
    si_python::create_MorphIndex_bindings(m, "MorphIndex");

    si_python::create_IndexedShapeGrid_bindings<si_python::SphereGridT>(m, "SphereGrid");
    si_python::create_SpatialGrid_bindings<si_python::MorphGridT>(m, "MorphGrid");
}

