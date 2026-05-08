/** @file
 * A minimum C/C++-agnostic example of running
 * QuEST, reporting the execution environment
 * and preparing random pure states with varying qubit counts.
 *
 * @author Tyson Jones
 */
#include "quest.h"
#include "dmr.h"
#include <stdio.h>

static void emit_analytics_event(const char *event_name) {
  DMRAnalytics *analytics;
  dmr_create_custom_analytics_event((char *) event_name, &analytics);
  dmr_print_analytics_from(analytics);
  dmr_destroy_custom_analytics_event(analytics);
}

static PauliStrSum create_heisenberg_ring_hamiltonian(int numQubits) {
  char *operators[] = {"XX", "YY", "ZZ", "Z"};
  qcomp coefficients[] = {.1, .2, .3, .4};

  int numTerms = 4 * numQubits;
  PauliStr allStrings[256];
  qcomp allCoeffs[256];
  int allInd = 0;

  for (int p = 0; p < 3; p++) {
    for (int i = 0; i < numQubits; i++) {
      int targs[] = {i, (i + 1) % numQubits};
      allStrings[allInd] = getPauliStr(operators[p], targs, 2);
      allCoeffs[allInd] = coefficients[p];
      allInd++;
    }
  }

  for (int i = 0; i < numQubits; i++) {
    allStrings[allInd] = getPauliStr(operators[3], &i, 1);
    allCoeffs[allInd] = coefficients[3];
    allInd++;
  }

  return createPauliStrSum(allStrings, allCoeffs, numTerms);
}

static PauliStrSum create_alternating_observable(int numQubits) {
  PauliStr strings[64];
  qcomp coeffs[64];

  for (int i = 0; i < numQubits; i++) {
    char pauli = "XYZ"[i % 3];
    strings[i] = getPauliStr(&pauli, &i, 1);
    coeffs[i] = getQcomp((i % 4) + 1, 0);
  }

  return createPauliStrSum(strings, coeffs, numQubits);
}

static void run_dynamics_like_circuit(int numQubits) {
  Qureg qureg = createQureg(numQubits);
  PauliStrSum hamil = create_heisenberg_ring_hamiltonian(numQubits);
  PauliStrSum observ = create_alternating_observable(numQubits);

  reportQuregParams(qureg);
  emit_analytics_event("circuit_start");

  initPlusState(qureg);

  qreal dt = 0.1;
  int order = 2;
  int reps = 20;
  int steps = 100;

  for (int s = 0; s < steps; s++) {
    applyTrotterizedUnitaryTimeEvolution(qureg, hamil, dt, order, reps);
  }

  char expecLabel[64];
  snprintf(expecLabel, sizeof(expecLabel), "<O> after evolution (%d qubits)",
           numQubits);
  reportScalar(expecLabel, calcExpecPauliStrSum(qureg, observ));

  char probLabel[64];
  snprintf(probLabel, sizeof(probLabel), "Total probability (%d qubits)",
           numQubits);
  reportScalar(probLabel, calcTotalProb(qureg));

  emit_analytics_event("circuit_end");

  destroyQureg(qureg);
  destroyPauliStrSum(hamil);
  destroyPauliStrSum(observ);
}

void circuit_2_qubits() { run_dynamics_like_circuit(2); }

void circuit_3_qubits() { run_dynamics_like_circuit(3); }

void circuit_4_qubits() { run_dynamics_like_circuit(4); }

void circuit_5_qubits() { run_dynamics_like_circuit(5); }

void circuit_6_qubits() { run_dynamics_like_circuit(6); }

int main(int argc, char *argv[]) {
  initQuESTEnv();
  reportQuESTEnv();
  circuit_executor_init(argc, argv);
  circuit_executor_add(circuit_2_qubits);
  circuit_executor_add(circuit_6_qubits);
  circuit_executor_add(circuit_4_qubits);
  circuit_executor_add(circuit_3_qubits);
  circuit_executor_add(circuit_5_qubits);
  circuit_executor_run();
  circuit_executor_destroy();
  finalizeQuESTEnv();
  return 0;
}
