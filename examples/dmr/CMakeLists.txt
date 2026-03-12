find_package(MPI REQUIRED)

# without DMR - only needs QuEST
add_executable(example_without_c example_without.c)
target_link_libraries(example_without_c PUBLIC QuEST)

# with DMR - needs QuEST + MPI + dmr
add_executable(example_with_dmr_c example_with_dmr.c)
target_link_libraries(example_with_dmr_c PUBLIC QuEST MPI::MPI_C dmr)
