/** @file
 * A minimum C/C++-agnostic example of running
 * QuEST, reporting the execution environment
 * and preparing 20-qubit random pure state.
 *
 * @author Tyson Jones
 */

#include "quest.h"

int main(void) {

  initQuESTEnv();
  reportQuESTEnv();

  // First circuit 32 nodes
  /*
  Qureg qureg = createForcedQureg(5);
  reportQuregParams(qureg);

  initRandomPureState(qureg);
  reportQureg(qureg);

  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability 1 (5 qbits)", prob);

  destroyQureg(qureg);
*/
  // Second circuit 16 nodes
  Qureg qureg = createForcedQureg(4);
  reportQuregParams(qureg);

  initRandomPureState(qureg);
  reportQureg(qureg);

  qreal prob = calcTotalProb(qureg);
  reportScalar("Total probability 2 (4 qbits)", prob);

  destroyQureg(qureg);
  // First circuit 32 nodes
  qureg = createForcedQureg(3);
  reportQuregParams(qureg);

  initRandomPureState(qureg);
  reportQureg(qureg);

  prob = calcTotalProb(qureg);
  reportScalar("Total probability 3 (3 qbits)", prob);

  destroyQureg(qureg);



  finalizeQuESTEnv();

  return 0;
}
