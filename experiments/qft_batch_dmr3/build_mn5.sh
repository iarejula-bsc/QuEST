#!/bin/bash
# Build qft_batch_c on MN5 against the shared DMR v3 module (dmrv3-dlb-gpp).
# Run on a login node from the QuEST root:
#   bash experiments/qft_batch_dmr3/build_mn5.sh
set -euo pipefail

QUEST_BUILD=${QUEST_BUILD:-build-dmr3-mn5}

module purge
module load cmake/3.30.5 gcc/14.1.0_binutils241
module use /apps/GPP/DMR/dmr-modules
module load dmrv3-dlb-gpp
# The module preloads DLB for the MPI runs; the compilers do not need it
unset LD_PRELOAD

echo "# dmr:   $DMR_PATH ($(dmr_wrapper --version))"
echo "# mpicc: $(which mpicc)"
echo "# gcc:   $(gcc --version | head -1)"

rm -rf "$QUEST_BUILD"
cmake -B "$QUEST_BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DFLOAT_PRECISION=2 \
  -DENABLE_MULTITHREADING=ON \
  -DENABLE_DISTRIBUTION=ON \
  -DENABLE_MALLEABILITY=ON \
  -DDMR_DIR="$DMR_PATH" \
  -DBUILD_EXAMPLES=ON \
  -DENABLE_TESTING=OFF
cmake --build "$QUEST_BUILD" --target qft_batch_c --parallel 16

grep -E "^MPI_C_HEADER_DIR" "$QUEST_BUILD/CMakeCache.txt"
ls -la "$QUEST_BUILD/examples/dmr/qft_batch_c"
