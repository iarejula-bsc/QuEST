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
#include <unistd.h>

static const char *MAL_DEFAULT_CHECKPOINT_FILE = "quest_circuit_checkpoint.bin";

// Jobs sharing a working directory need distinct files, so the path can be set
// with QUEST_MAL_CHECKPOINT_FILE
static const char *checkpoint_path(void) {
  const char *path = getenv("QUEST_MAL_CHECKPOINT_FILE");
  return (path && *path) ? path : MAL_DEFAULT_CHECKPOINT_FILE;
}

// PRIVATE STATE

static bool global_mal_initialized = false;

#ifdef ENABLE_MALLEABILITY
// DMR policy that sizes the job for the next Qureg: every rank sets the same
// num_qubits before dmr_check(USE_POLICY), so run() decides identically on all.
typedef struct {
  int num_qubits;
} QubitPolicyState;

static QubitPolicyState qubit_policy_state = {0};
static DMRPolicy qubit_policy;

static DMRPolicySuggestion qubit_policy_run(DMRPolicy *policy,
                                            DMRPolicyContext const *context) {
  const QubitPolicyState *state = (const QubitPolicyState *)policy->state;
  int target = mal_target_nodes_for_qubits(state->num_qubits);

  if (target > context->current_nodes)
    return dmr_policy_expand_to(target, 0, context);
  if (target < context->current_nodes)
    return dmr_policy_shrink_to(target, 0, context);
  return dmr_policy_stay();
}
#endif

// Resize policy parameters. The defaults are the MN5 experiment's; the
// QUEST_MAL_* environment variables scale them down for small clusters.
static int env_int_or(const char *name, int fallback) {
  const char *value = getenv(name);
  return (value && *value) ? atoi(value) : fallback;
}

int mal_target_nodes_for_qubits(int numQubits) {
#ifdef ENABLE_MALLEABILITY
  static const int minQubits = env_int_or("QUEST_MAL_MIN_QUBITS", 28);
  static const int baseQubits = env_int_or("QUEST_MAL_BASE_QUBITS", 26);
  static const int maxNodeExp = env_int_or("QUEST_MAL_MAX_NODE_EXP", 6);

  if (numQubits < minQubits)
    return 1;

  int nodeExp = numQubits - baseQubits;
  if (nodeExp > maxNodeExp)
    nodeExp = maxNodeExp;

  return 1 << nodeExp;
#else
  (void)numQubits;
  return 1;
#endif
}

int checkpoint_load(void) {
  FILE *f = fopen(checkpoint_path(), "rb");
  if (!f)
    return 0;
  int index = 0;
  fread(&index, sizeof(int), 1, f);
  fclose(f);
  return index;
}

void checkpoint_save(int index) {
  FILE *f = fopen(checkpoint_path(), "wb");
  if (!f)
    return;
  fwrite(&index, sizeof(int), 1, f);
  fclose(f);
}

void checkpoint_clear(void) { remove(checkpoint_path()); }

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

  qubit_policy.name = "quest_qubits";
  qubit_policy.state_size = sizeof(QubitPolicyState);
  qubit_policy.state = &qubit_policy_state;
  qubit_policy.populate = NULL;
  qubit_policy.run = qubit_policy_run;
  qubit_policy.save = NULL;
  qubit_policy.load = NULL;
  qubit_policy.destroy = NULL;
  if (dmr_set_policy(&qubit_policy) != DMR_SUCCESS) {
    fprintf(stderr, "[malleability] ERROR: dmr_set_policy() failed.\n");
    exit(1);
  }

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

void mal_resize_for_qubits(int numQubits) {
#ifdef ENABLE_MALLEABILITY
  // A shrink hands its nodes back asynchronously. An expand requested before
  // that finishes can be granted the same nodes while the DVM still removes
  // them, and DMR then disables itself. Waiting before every expand lets the
  // release finish; the wait falls outside every measured interval.
  static const int expandDelay = env_int_or("QUEST_MAL_EXPAND_DELAY_S", 10);
  if (mal_target_nodes_for_qubits(numQubits) > dmr_get_current_node_count())
    sleep(expandDelay);

  qubit_policy_state.num_qubits = numQubits;
  DMR_AUTO(dmr_check(USE_POLICY), (void)NULL, (void)NULL, (void)NULL);
#else
  (void)numQubits;
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
  mal_finalize();
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
