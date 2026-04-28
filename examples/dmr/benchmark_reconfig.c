/** @file
 * Benchmark of the circuit executor covering all qubit-count reconfiguration
 * transitions up to 5 qubits: 2-3, 2-4, 2-5, 3-4, 3-5, 4-5.
 *
 * Each pair runs a circuit at the lower qubit count followed by a circuit at
 * the higher qubit count, letting the executor handle the reconfiguration
 * transparently. Wall-clock time is reported per circuit and per pair.
 *
 * @author Íñigo Aréjula Aísa
 */
#include "quest.h"
#include <stdio.h>


static void run_timed_circuit(int n) {
    Qureg qureg = createQureg(n);
    initRandomPureState(qureg);
    qreal prob = calcTotalProb(qureg);
    reportScalar("  total probability", prob);
    destroyQureg(qureg);
}

static void circuit_2(void) { run_timed_circuit(2); }
static void circuit_3(void) { run_timed_circuit(3); }
static void circuit_4(void) { run_timed_circuit(4); }
static void circuit_5(void) { run_timed_circuit(5); }
static void circuit_6(void) { run_timed_circuit(6); }


static void hdr_2_3(void) {  printf("\n[Pair 2 -> 3]\n"); }
static void hdr_2_4(void) {  printf("\n[Pair 2 -> 4]\n"); }
static void hdr_2_5(void) {  printf("\n[Pair 2 -> 5]\n"); }
static void hdr_2_6(void) {  printf("\n[Pair 2 -> 6]\n"); }
static void hdr_3_4(void) {  printf("\n[Pair 3 -> 4]\n"); }
static void hdr_3_5(void) {  printf("\n[Pair 3 -> 5]\n"); }
static void hdr_3_6(void) {  printf("\n[Pair 3 -> 6]\n"); }
static void hdr_4_5(void) {  printf("\n[Pair 4 -> 5]\n"); }
static void hdr_4_6(void) {  printf("\n[Pair 4 -> 6]\n"); }
static void hdr_5_6(void) {  printf("\n[Pair 5 -> 6]\n"); }




int main(int argc, char *argv[]) {
    initQuESTEnv();
    reportQuESTEnv();
    circuit_executor_init(argc, argv);

    printf("\n=== Benchmark: all reconfiguration transitions up to 5 qubits ===\n");

    circuit_executor_add(hdr_2_3); circuit_executor_add(circuit_2); circuit_executor_add(circuit_3);
    circuit_executor_add(hdr_2_4); circuit_executor_add(circuit_2); circuit_executor_add(circuit_4);
    circuit_executor_add(hdr_2_5); circuit_executor_add(circuit_2); circuit_executor_add(circuit_5);
    circuit_executor_add(hdr_2_6); circuit_executor_add(circuit_2); circuit_executor_add(circuit_6);

    circuit_executor_add(hdr_3_4); circuit_executor_add(circuit_3); circuit_executor_add(circuit_4);
    circuit_executor_add(hdr_3_5); circuit_executor_add(circuit_3); circuit_executor_add(circuit_5);
    circuit_executor_add(hdr_3_6); circuit_executor_add(circuit_3); circuit_executor_add(circuit_6);

    circuit_executor_add(hdr_4_5); circuit_executor_add(circuit_4); circuit_executor_add(circuit_5);
    circuit_executor_add(hdr_4_6); circuit_executor_add(circuit_4); circuit_executor_add(circuit_6);
    circuit_executor_add(hdr_5_6); circuit_executor_add(circuit_5); circuit_executor_add(circuit_6);


    circuit_executor_run();


    circuit_executor_destroy();
    finalizeQuESTEnv();
    return 0;
}
