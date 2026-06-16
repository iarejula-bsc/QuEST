/** @file
 * A minimum C/C++-agnostic example of running
 * QuEST, reporting the execution environment
 * and running QFT circuits that exercise the fixed
 * malleability resize policy.
 *
 * @author Tyson Jones
 */
#include "quest.h"
#include "dmr.h"
#include <stdio.h>
#include <math.h>

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


void circuit_24_qubits() { run_qft_circuit(24); }

void circuit_28_qubits() { run_qft_circuit(28); }

void circuit_30_qubits() { run_qft_circuit(30); }

void circuit_32_qubits() { run_qft_circuit(32); }

void circuit_34_qubits() { run_qft_circuit(34); }

void circuit_36_qubits() { run_qft_circuit(36); }

int main(int argc, char *argv[]) {
  initQuESTEnv();
  reportQuESTEnv();
  circuit_executor_init(argc, argv);
  circuit_executor_add(circuit_28_qubits);
  circuit_executor_add(circuit_30_qubits);
  circuit_executor_add(circuit_24_qubits);
  circuit_executor_add(circuit_36_qubits);
  circuit_executor_add(circuit_32_qubits);
  circuit_executor_add(circuit_34_qubits);
  circuit_executor_run();
  circuit_executor_destroy();
  finalizeQuESTEnv();
  return 0;
}
