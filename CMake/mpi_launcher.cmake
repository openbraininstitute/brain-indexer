if(NOT ${PROJECT}_MPIEXEC)
    find_program(${PROJECT}_SRUN srun)
    mark_as_advanced(${PROJECT}_SRUN)

    if(${PROJECT}_SRUN)
        set(${PROJECT}_MPIEXEC ${${PROJECT}_SRUN} CACHE STRING
            "The SLURM command `srun`."
        )
    else()
        set(${PROJECT}_MPIEXEC ${MPIEXEC_EXECUTABLE} CACHE STRING
            "The MPI startup command."
        )
    endif()
    mark_as_advanced(${PROJECT}_MPIEXEC)
endif()

if(NOT ${PROJECT}_MPIEXEC_NPROC_FLAG)
    set(${PROJECT}_MPIEXEC_NPROC_FLAG "-n" CACHE STRING
        "The flag to set the number of processors. For `mpiexec` the standard requires `-n`."
    )
    mark_as_advanced(${PROJECT}_MPIEXEC_NPROC_FLAG)
endif()
