/** @file
 * Malleability integration with DMRlib.
 *
 * All implementations live here: mal_*, checkpoints, CircuitExecutor methods,
 * and public API wrappers.
 *
 * @author Íñigo Aréjula Aísa
 */

#include "quest/src/malleability/malleability.hpp"

#ifdef ENABLE_MALLEABILITY
#define MPICH_SKIP_MPICXX
#define OMPI_SKIP_MPICXX
#include <dmr.h>
#include <mpi.h>
#endif

#include "quest/include/environment.h"

#include <cstdio>
#include <cstdlib>

static const char *MAL_CHECKPOINT_FILE = "quest_circuit_checkpoint.bin";

// PRIVATE STATE

static bool global_mal_initialized = false;

int mal_target_nodes_for_qubits(int numQubits) {
#ifdef ENABLE_MALLEABILITY
  if (numQubits < 28)
    return 1;

  int nodeExp = numQubits - 26;
  if (nodeExp > 6)
    nodeExp = 6;

  return 1 << nodeExp;
#else
  (void)numQubits;
  return 1;
#endif
}

int checkpoint_load(void) {
  FILE *f = fopen(MAL_CHECKPOINT_FILE, "rb");
  if (!f)
    return 0;
  int index = 0;
  fread(&index, sizeof(int), 1, f);
  fclose(f);
  return index;
}

void checkpoint_save(int index) {
  FILE *f = fopen(MAL_CHECKPOINT_FILE, "wb");
  if (!f)
    return;
  fwrite(&index, sizeof(int), 1, f);
  fclose(f);
}

void checkpoint_clear(void) { remove(MAL_CHECKPOINT_FILE); }

void mal_init(int argc, char **argv) {
#ifdef ENABLE_MALLEABILITY
  int mpi_initialized;
  MPI_Initialized(&mpi_initialized);
  if (!mpi_initialized) {
    fprintf(stderr, "[malleability] ERROR: MPI must be initialized before "
                    "circuit_executor_init(). Call initQuESTEnv() first.\n");
    exit(1);
  }
  if (global_mal_initialized) {
    fprintf(stderr, "[malleability] WARNING: circuit_executor_init() called "
                    "twice, ignoring.\n");
    return;
  }
  DMR_AUTO(dmr_init(argc, argv), (void)NULL, (void)NULL,
             (void)NULL);

#else
  (void)argc;
  (void)argv;
#endif
  global_mal_initialized = true;
}

void mal_finalize(void) {
#ifdef ENABLE_MALLEABILITY
  if (!global_mal_initialized)
    return;
  dmr_finalize();
#endif
  global_mal_initialized = false;
}

void mal_resize_to(int target_nodes) {
#ifdef ENABLE_MALLEABILITY
  int current = dmr_get_current_node_count();
  if (target_nodes == current)
    return;

  if (target_nodes < current) {
    int to_remove = current - target_nodes;
    dmr_set_nodes_next_shrink(to_remove);
    DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
  } else {
    int to_add = target_nodes - current;
    dmr_set_nodes_next_expand(to_add);
    DMR_AUTO(dmr_check(SHOULD_EXPAND), (void)NULL, (void)NULL, (void)NULL);
  }
#else
  (void)target_nodes;
#endif
}

// CircuitExecutor METHOD IMPLEMENTATIONS

CircuitExecutor::CircuitExecutor() : initialized_(false) {}

CircuitExecutor &CircuitExecutor::instance() {
  static CircuitExecutor instance_;
  return instance_;
}

void CircuitExecutor::init(int argc, char **argv) {
  mal_init(argc, argv);
  initialized_ = true;
}

void CircuitExecutor::add(Circuit circuit) {
  if (!initialized_) {
    fprintf(stderr, "[malleability] ERROR: call circuit_executor_init() "
                    "before circuit_executor_add().\n");
    return;
  }
  circuits_.push_back(circuit);
}


void CircuitExecutor::run() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Solo rank 0 valida inicialización
  if (rank == 0) {
    if (!initialized_) {
      fprintf(stderr, "[malleability] ERROR: call circuit_executor_init() "
                      "before circuit_executor_run().\n");
    }
  }

  // Broadcast del estado de initialized_
  MPI_Bcast(&initialized_, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

  if (!initialized_) return;

  int start = 0;

  // Solo rank 0 carga checkpoint
  if (rank == 0) {
    start = checkpoint_load();
  }

  // Rank 0 hace broadcast del start
  MPI_Bcast(&start, 1, MPI_INT, 0, MPI_COMM_WORLD);

  for (int i = start; i < (int)circuits_.size(); i++) {

    // Solo rank 0 guarda checkpoint
    if (rank == 0) {
      checkpoint_save(i);
    }

    // Todos los ranks ejecutan el circuito
    circuits_[i]();
  }

  // Solo rank 0 limpia checkpoint
  if (rank == 0) {
    checkpoint_clear();
  }
}

void CircuitExecutor::destroy() {
  circuits_.clear();
  initialized_ = false;
}

/*
 * PUBLIC API — thin wrappers over the singleton
 */

void circuit_executor_init(int argc, char **argv) {
  CircuitExecutor::instance().init(argc, argv);
}

void circuit_executor_add(Circuit circuit) {
  CircuitExecutor::instance().add(circuit);
}

void circuit_executor_run(void) { CircuitExecutor::instance().run(); }

void circuit_executor_destroy(void) { CircuitExecutor::instance().destroy(); }
