#!/bin/bash
# Build reconfig_matrix_cpp on MN5 against the shared DMR v3 module
# (dmrv3-dlb-gpp), in the same build tree as qft_batch_c. Run on a login node
# from the QuEST root:
#   bash experiments/reconfig_matrix_dmr3/build_mn5.sh
# If build-dmr3-mn5 does not exist yet, run experiments/qft_batch_dmr3/build_mn5.sh first.
set -euo pipefail

QUEST_BUILD=${QUEST_BUILD:-build-dmr3-mn5}

module purge
module load cmake/3.30.5 gcc/14.1.0_binutils241
module use /apps/GPP/DMR/dmr-modules
module load dmrv3-dlb-gpp
# The module preloads DLB for the MPI runs; the compilers do not need it
unset LD_PRELOAD

cmake "$QUEST_BUILD"
cmake --build "$QUEST_BUILD" --target reconfig_matrix_cpp --parallel 16
ls -la "$QUEST_BUILD/examples/dmr/reconfig_matrix_cpp"
