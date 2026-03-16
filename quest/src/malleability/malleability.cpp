#include "quest/src/malleability/malleability.hpp"

#ifdef ENABLE_MALLEABILITY

// Prevent mpicxx.h C++ bindings from conflicting with dmr.h's C MPI include
#define MPICH_SKIP_MPICXX
#define OMPI_SKIP_MPICXX

#include <dmr.h>
#include <mpi.h>
#endif

#include <stdio.h>

void restart() {
    //TODO: some debug message
}

void finalize() {
    //TODO: some debug message
}

void init_malleability() {
#ifdef ENABLE_MALLEABILITY
    DMR_AUTO(dmr_init(0, nullptr), (void)NULL, restart(),
             (void)NULL);
#endif
}

void shrink_to(int target_nodes) {
#ifdef ENABLE_MALLEABILITY
    int current_nodes = dmr_get_current_node_count();

    // cannot expand, only shrink
    if (target_nodes >= current_nodes)
        return;

    int nodes_to_remove = current_nodes - target_nodes;
    dmr_set_nodes_next_shrink(nodes_to_remove);
    DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL,
             finalize());
#endif
}
