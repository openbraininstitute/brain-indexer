
#include "rtree_index.hpp"
#include <brain_indexer/logging.hpp>

#if SI_MPI == 1
#include "distributed_analysis.hpp"
#endif

namespace si_python = si::py_bindings;

namespace brain_indexer { namespace py_bindings {

void check_signals() {
    if (PyErr_CheckSignals() != 0) {
        throw py::error_already_set();
    }
}

}}  // namespace brain_indexer::py_bindings


PYBIND11_MODULE(_brain_indexer, m) {

    m.def("_minimum_log_severity",
        []() -> si::LogSeverity {
            return si::get_global_minimum_log_severity();
        }
    );

    m.def("_register_python_logger",
        [](py::object python_logger) {
            si::register_logging_callback(
                [python_logger](si::LogSeverity log_severity, const std::string& message) {
                    if(log_severity == si::LogSeverity::DEBUG) {
                        python_logger.attr("debug")(py::str(message));
                    }
                    else if (log_severity == si::LogSeverity::INFO) {
                        python_logger.attr("info")(py::str(message));
                    }
                    else if (log_severity == si::LogSeverity::WARN) {
                        python_logger.attr("warning")(py::str(message));
                    }
                    else if (log_severity == si::LogSeverity::ERROR) {
                        python_logger.attr("error")(py::str(message));
                    }
                    else {
                        python_logger.attr("error")(py::str("Invalid log severity detected for message:"));
                        python_logger.attr("error")(py::str(message));
                        throw std::runtime_error("Invalid log severity.");
                    }
                }
            );
        }
    );

    py::enum_<si::LogSeverity>(m, "_LogSeverity")
        .value("DEBUG", si::LogSeverity::DEBUG)
        .value("INFO", si::LogSeverity::INFO)
        .value("WARN", si::LogSeverity::WARN)
        .value("ERROR", si::LogSeverity::ERROR);


    py::module m_test = m.def_submodule("tests");
    m_test.def("write_logs",
        [](const std::string& debug_message,
           const std::string& cond_debug_message,
           const std::string& info_message,
           const std::string& warn_message,
           const std::string& error_message
        ) {
            SI_LOG_DEBUG(debug_message);
            SI_LOG_DEBUG_IF(cond_debug_message.size() > 0, cond_debug_message);
            si::log_info(info_message);
            si::log_warn(warn_message);
            si::log_error(error_message);
        }
    );

    si_python::create_MetaDataConstants_bindings(m);

    using namespace pybind11::literals;
    m.attr("SectionType") = py::module::import("enum").attr("IntEnum")(
        "SectionType",
        py::dict(
            "undefined"_a=static_cast<unsigned char>(si::SectionType::undefined),
            "soma"_a=static_cast<unsigned char>(si::SectionType::soma),
            "axon"_a=static_cast<unsigned char>(si::SectionType::axon),
            "basal_dendrite"_a=static_cast<unsigned char>(si::SectionType::basal_dendrite),
            "apical_dendrite"_a=static_cast<unsigned char>(si::SectionType::apical_dendrite)
        )
    );

    PYBIND11_NUMPY_DTYPE(si::gid_segm_t, gid, section_id, segment_id);  // struct as numpy dtype

    si_python::create_IndexedPoint_bindings(m);
    si_python::create_Sphere_bindings(m);
    si_python::create_Synapse_bindings(m);
    si_python::create_MorphoEntry_bindings(m);

    si_python::create_PointIndex_bindings(m, "PointIndex");
    si_python::create_SphereIndex_bindings(m, "SphereIndex");
    si_python::create_SynapseIndex_bindings(m, "SynapseIndex");
    si_python::create_MorphIndex_bindings(m, "MorphIndex");

    si_python::create_SynapseIndexBulkBuilder_bindings(m, "SynapseIndexBulkBuilder");
    si_python::create_MorphIndexBulkBuilder_bindings(m, "MorphIndexBulkBuilder");

    // Distributed/lazy R-trees, multi-indexes.
    si_python::create_MorphMultiIndex_bindings(m, "MorphMultiIndex");
    si_python::create_SynapseMultiIndex_bindings(m, "SynapseMultiIndex");

#if SI_MPI == 1
    si_python::create_MorphMultiIndexBulkBuilder_bindings(m, "MorphMultiIndexBulkBuilder");
    si_python::create_SynapseMultiIndexBulkBuilder_bindings(m, "SynapseMultiIndexBulkBuilder");

    si_python::create_call_some_mpi_from_cxx_bindings(m);
    si_python::create_analysis_bindings(m);
    si_python::create_is_valid_comm_size_bindings(m);
#endif

    // Experimental space filling order
    si_python::create_experimental_space_filling_order(m);
}

