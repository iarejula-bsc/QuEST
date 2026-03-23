/** @file
 * A minimum C/C++-agnostic example of running
 * QuEST, reporting the execution environment
 * and preparing random pure states with varying qubit counts.
 *
 * @author Tyson Jones
 */
#include "quest.h"

// Circuit with 2 qubits
void circuit_2_qubits() {
  Qureg qureg = createForcedQureg(2);
  reportQuregParams(qureg);
  initRandomPureState(qureg);
  reportQureg(qureg);
  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability (2 qubits)", prob);
  destroyQureg(qureg);
}

// Circuit with 3 qubits
void circuit_3_qubits() {
  Qureg qureg = createForcedQureg(3);
  reportQuregParams(qureg);
  initRandomPureState(qureg);
  reportQureg(qureg);
  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability (3 qubits)", prob);
  destroyQureg(qureg);
}

// Circuit with 4 qubits
void circuit_4_qubits() {
  Qureg qureg = createForcedQureg(4);
  reportQuregParams(qureg);
  initRandomPureState(qureg);
  reportQureg(qureg);
  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability (4 qubits)", prob);
  destroyQureg(qureg);
}

// Circuit with 5 qubits
void circuit_5_qubits() {
  Qureg qureg = createForcedQureg(5);
  reportQuregParams(qureg);
  initRandomPureState(qureg);
  reportQureg(qureg);
  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability (5 qubits)", prob);
  destroyQureg(qureg);
}

// Circuit with 6 qubits
void circuit_6_qubits() {
  Qureg qureg = createForcedQureg(6);
  reportQuregParams(qureg);
  initRandomPureState(qureg);
  reportQureg(qureg);
  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability (6 qubits)", prob);
  destroyQureg(qureg);
}

int main(int argc, char *argv[]) {
  initQuESTEnv();
  reportQuESTEnv();
  circuit_executor_init(argc, argv);

  circuit_executor_add(circuit_2_qubits);
  circuit_executor_add(circuit_6_qubits);
  circuit_executor_add(circuit_4_qubits);
  circuit_executor_add(circuit_5_qubits);
  circuit_executor_add(circuit_3_qubits);
  circuit_executor_run();

  circuit_executor_destroy();
  finalizeQuESTEnv();
  return 0;
}
