find_package(MPI REQUIRED)

# without DMR - only needs QuEST
add_executable(example_multi_c example_multi.c)
target_link_libraries(example_multi_c PUBLIC QuEST)

# benchmark: all qubit-count transitions up to 5 qubits
add_executable(benchmark_reconfig_c benchmark_reconfig.c)
target_link_libraries(benchmark_reconfig_c PUBLIC QuEST)

# with DMR - needs QuEST + MPI + dmr
add_executable(example_with_dmr_c example_with_dmr.c)
target_link_libraries(example_with_dmr_c PUBLIC QuEST MPI::MPI_C dmr)
