/** @file
 * Batch of QFT circuits run through the malleable circuit executor, one
 * circuit per qubit count given on the command line (default: 28 30 24 34,
 * the batch of the paper's table). Before each circuit the resize policy
 * picks the node count and DMR reconfigures.
 *
 * @author Íñigo Aréjula Aísa
 */
#include "quest.h"
#include "dmr.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void emit_analytics_event(const char *event_name) {
  DMRAnalytics *analytics;
  dmr_create_custom_analytics_event((char *) event_name, &analytics);
  dmr_print_analytics_from(analytics);
  dmr_destroy_custom_analytics_event(analytics);
}

static double calcPhaseShift(int M) {
  return M_PI / pow(2, (M - 1));
}

static void qftQubit(Qureg qureg, int numQubits, int qubit_id) {
  applyHadamard(qureg, qubit_id);
  int m = 2;
  for (int control = qubit_id + 1; control < numQubits; ++control) {
    double angle = calcPhaseShift(m++);
    applyTwoQubitPhaseShift(qureg, control, qubit_id, angle);
  }
}

static void qft(Qureg qureg, int numQubits) {
  for (int qid = 0; qid < numQubits; ++qid)
    qftQubit(qureg, numQubits, qid);
}

static void run_qft_circuit(int numQubits) {
  printf("\n[policy] %d qubits -> %d nodes\n", numQubits,
         mal_target_nodes_for_qubits(numQubits));

  Qureg qureg = createQureg(numQubits);

  reportQuregParams(qureg);
  emit_analytics_event("circuit_start");

  initZeroState(qureg);
  qft(qureg, numQubits);

  char probLabel[64];
  snprintf(probLabel, sizeof(probLabel), "prob of |0..0> (%d qubits)",
           numQubits);
  reportScalar(probLabel, calcProbOfBasisState(qureg, 0));

  emit_analytics_event("circuit_end");

  destroyQureg(qureg);
}


#define MAX_CIRCUITS 8

static int batchQubits[MAX_CIRCUITS];

static void circuit_0(void) { run_qft_circuit(batchQubits[0]); }
static void circuit_1(void) { run_qft_circuit(batchQubits[1]); }
static void circuit_2(void) { run_qft_circuit(batchQubits[2]); }
static void circuit_3(void) { run_qft_circuit(batchQubits[3]); }
static void circuit_4(void) { run_qft_circuit(batchQubits[4]); }
static void circuit_5(void) { run_qft_circuit(batchQubits[5]); }
static void circuit_6(void) { run_qft_circuit(batchQubits[6]); }
static void circuit_7(void) { run_qft_circuit(batchQubits[7]); }

static const Circuit circuits[MAX_CIRCUITS] = {
  circuit_0, circuit_1, circuit_2, circuit_3,
  circuit_4, circuit_5, circuit_6, circuit_7
};

int main(int argc, char *argv[]) {
  static const int defaultQubits[] = {28, 30, 24, 34};
  int numCircuits = argc - 1;

  if (numCircuits == 0) {
    numCircuits = sizeof(defaultQubits) / sizeof(defaultQubits[0]);
    for (int i = 0; i < numCircuits; i++)
      batchQubits[i] = defaultQubits[i];
  } else {
    if (numCircuits > MAX_CIRCUITS) {
      fprintf(stderr, "qft_batch: at most %d circuits\n", MAX_CIRCUITS);
      return 1;
    }
    for (int i = 0; i < numCircuits; i++)
      batchQubits[i] = atoi(argv[i + 1]);
  }

  initQuESTEnv();
  reportQuESTEnv();
  circuit_executor_init(argc, argv);
  for (int i = 0; i < numCircuits; i++)
    circuit_executor_add(circuits[i]);
  circuit_executor_run();
  circuit_executor_destroy();
  finalizeQuESTEnv();
  return 0;
}
