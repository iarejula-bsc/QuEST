/** @file
 * A minimum C/C++-agnostic example of running
 * QuEST, reporting the execution environment
 * and preparing 20-qubit random pure state.
 *
 * @author Tyson Jones
 */
#include "quest.h"
#include <dmr.h>
#include <stdio.h>


/*
 * This is the function that is called when DMR has restarted
 * the program due to a reconfiguration
 */
void restart(int rank, char *processor_name) {
  printf("%s rank %d restarted. In a real program, the current process would "
         "read some data.\n",
         processor_name, rank);
}
/*
 * This is the function that is called when the current process
 * has already given up its data and needs to take some final
 * action before terminating, e.g. cleaning up data
 */
void finalize(int rank, char *processor_name) {
  printf("%s rank %d is about to exit. In a real program, the current process "
         "might call free(...)\n",
         processor_name, rank);
}
int main(int argc, char *argv[]) {

  initQuESTEnv();
  reportQuESTEnv();

  int my_rank;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Get_processor_name(processor_name, &name_len);

  DMR_AUTO(dmr_init(argc, argv), (void)NULL, restart(my_rank, processor_name),
           (void)NULL);

  int reconfig_count = dmr_get_reconfig_count();

  if (reconfig_count == 0) {

    // First circuit 32 nodes
    Qureg qureg = createForcedQureg(5);
    reportQuregParams(qureg);

    initRandomPureState(qureg);
    reportQureg(qureg);

    qreal prob = calcTotalProb(qureg);
    reportScalar("Total probability 1 (5 qbits)", prob);

    destroyQureg(qureg);
    int current_nodes = dmr_get_current_node_count();
    dmr_set_nodes_next_shrink(current_nodes / 2);
    DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL,
             finalize(my_rank, processor_name));
  }

  if (reconfig_count == 1) {

    // Second circuit 16 nodes
    Qureg qureg = createForcedQureg(4);
    reportQuregParams(qureg);

    initRandomPureState(qureg);
    reportQureg(qureg);

    qreal prob = calcTotalProb(qureg);
    reportScalar("Total probability 2 (4 qbits)", prob);

    destroyQureg(qureg);

    int current_nodes = dmr_get_current_node_count();
    dmr_set_nodes_next_shrink(current_nodes / 2);
    DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL,
             finalize(my_rank, processor_name));
  }
  // First circuit 32 nodes
  Qureg qureg = createForcedQureg(3);
  reportQuregParams(qureg);

  initRandomPureState(qureg);
  reportQureg(qureg);

  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability 3 (3 qbits)", prob);

  destroyQureg(qureg);

  finalizeQuESTEnv();

  return 0;
}
