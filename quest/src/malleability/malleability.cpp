#include "quest/src/malleability/malleability.hpp"
//#include "quest/include/malleability.h"

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


/* Static storage - set once via store_argc_argv(), read by init_malleability() */
static int    s_argc = 0;
static char **s_argv = NULL;

extern "C" void store_argc_argv(int argc, char **argv) {
    s_argc = argc;
    s_argv = argv;
}

void init_malleability(void) {
#ifdef ENABLE_MALLEABILITY
    dmr_init(s_argc, s_argv);
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
