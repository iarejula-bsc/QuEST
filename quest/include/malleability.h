/** @file
 * Public malleability API for QuEST.
 * Safe to include from both C and C++ translation units.
 *
 * Users only interact with circuit_executor_* functions.
 * All malleability logic (DMR, node resizing) is internal.
 *
 * @author Íñigo Aréjula Aísa
 */

#ifndef MALLEABILITY_H
#define MALLEABILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/** Function pointer type for a circuit (no args, no return). */
typedef void (*Circuit)(void);

/** Initialise the circuit executor. Must be called after initQuESTEnv().
 *  Initialises DMR internally. */
void circuit_executor_init(int argc, char** argv);

/** Add a circuit to be executed. */
void circuit_executor_add(Circuit circuit);

/** Run all circuits in order. Malleability is handled transparently. */
void circuit_executor_run(void);

/** Finalise the executor. Must be called before finalizeQuESTEnv().
 *  Finalises DMR internally. */
void circuit_executor_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* MALLEABILITY_H */
