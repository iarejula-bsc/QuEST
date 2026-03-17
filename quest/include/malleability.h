/** @file
 * API signatures for DMR malleability support.
 */
#ifndef MALLEABILITY_H
#define MALLEABILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/// Store argc/argv for later use by dmr_init
void store_argc_argv(int argc, char **argv);


#ifdef __cplusplus
}
#endif

#endif // MALLEABILITY_H
