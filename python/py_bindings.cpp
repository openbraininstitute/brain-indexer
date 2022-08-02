
#include "rtree_index.hpp"
#include "index_grid.hpp"

#if SI_MPI == 1
#include "distributed_analysis.hpp"
#endif

#ifndef SI_GRID_VOXEL_LENGTH
#define SI_GRID_VOXEL_LENGTH 10
#endif

namespace si_python = si::py_bindings;

namespace spatial_index { namespace py_bindings {

void check_signals() {
    if (PyErr_CheckSignals() != 0) {
        throw py::error_already_set();
    }
}

using SphereGridT = si::SpatialGrid<si::IndexedSphere, SI_GRID_VOXEL_LENGTH>;
using MorphGridT = si::MorphSpatialGrid<SI_GRID_VOXEL_LENGTH>;

}}  // namespace spatial_index::py_bindings

PYBIND11_DECLARE_HOLDER_TYPE(T, si::MemDiskPtr<T>);

PYBIND11_MODULE(_spatial_index, m) {

    py::enum_<si::detail::entry_kind>(m, "EntryKind")
    .value("SOMA", si::detail::entry_kind::SOMA)
    .value("SEGMENT", si::detail::entry_kind::SEGMENT)
    .value("SYNAPSE", si::detail::entry_kind::SYNAPSE);

    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, section_id, segment_id);  // struct as numpy dtype

    si_python::create_Sphere_bindings(m);
    si_python::create_Synapse_bindings(m);
    si_python::create_MorphoEntry_bindings(m);

    si_python::create_IndexTree_bindings<si::IndexedSphere>(m, "SphereIndex");
    si_python::create_SynapseIndex_bindings(m, "SynapseIndex");
    si_python::create_MorphIndex_bindings(m, "MorphIndex");

    // Experimental memory from mem-mapped file
    using MorphIndexTreeMemDisk = si::MemDiskRtree<si::MorphoEntry>;
    si_python::create_MorphIndex_bindings<MorphIndexTreeMemDisk>(m, "MorphIndexMemDisk");
    using SynIndexMemDisk = si::MemDiskRtree<si::Synapse>;
    si_python::create_SynapseIndex_bindings<SynIndexMemDisk>(m, "SynapseIndexMemDisk");

    si_python::create_IndexedShapeGrid_bindings<si_python::SphereGridT>(m, "SphereGrid");
    si_python::create_SpatialGrid_bindings<si_python::MorphGridT>(m, "MorphGrid");

    // Experimental distributed/lazy R-trees.
    si_python::create_MorphMultiIndex_bindings(m, "MorphMultiIndex");

#if SI_MPI == 1
    si_python::create_MorphMultiIndexBulkBuilder_bindings(m, "MorphMultiIndexBulkBuilder");

    si_python::create_call_some_mpi_from_cxx_bindings(m);
    si_python::create_analysis_bindings(m);
#endif

}

