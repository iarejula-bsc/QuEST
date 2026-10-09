#!/bin/bash
# Build DMR v3 (with the wait-for-releases patch) and qft_batch_c on MN5.
# Run on a login node from the QuEST root:
#   bash experiments/qft_batch_dmr3/build_mn5.sh
set -euo pipefail

QUEST_ROOT=$(pwd)
DMR_COMMIT=${DMR_COMMIT:-56cca91}
DMR_SRC=${DMR_SRC:-$HOME/workspaces/dmrv3-qft}
DMR_PREFIX=${DMR_PREFIX:-$HOME/.local/dmrv3-qft}
QUEST_BUILD=${QUEST_BUILD:-build-dmr3-mn5}
PATCH=$QUEST_ROOT/experiments/qft_batch_dmr3/dmr-v3-wait-releases-before-expand.patch

module purge
module load cmake/3.30.5 gcc/14.1.0_binutils241
module use /apps/GPP/DMR/dmr-modules
module load openpmix-for-dmr prrte-for-dmr openmpi-for-dmr

echo "# mpicc: $(which mpicc)"
echo "# gcc:   $(gcc --version | head -1)"
echo "# pmix:  $(pkg-config --modversion pmix)"

# DMR v3 from the checkout behind the shared MN5 install
if [ ! -d "$DMR_SRC" ]; then
  git -c safe.directory='*' clone /apps/GPP/DMR/source_petter/dmrv3 "$DMR_SRC"
fi
cd "$DMR_SRC"
git checkout -q "$DMR_COMMIT"
git checkout -q -- src/dmr.c
patch -p1 < "$PATCH"
rm -rf build
cmake -B build \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_INSTALL_PREFIX="$DMR_PREFIX" \
  -DDMR_CHECKPOINT_RESTART=1 \
  -DDMR_USE_TALP=0
cmake --build build -j16
cmake --install build
echo "$DMR_COMMIT+wait-releases-before-expand" > "$DMR_PREFIX/COMMIT"

# QuEST
cd "$QUEST_ROOT"
rm -rf "$QUEST_BUILD"
cmake -B "$QUEST_BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DFLOAT_PRECISION=2 \
  -DENABLE_MULTITHREADING=ON \
  -DENABLE_DISTRIBUTION=ON \
  -DENABLE_MALLEABILITY=ON \
  -DDMR_DIR="$DMR_PREFIX" \
  -DBUILD_EXAMPLES=ON \
  -DENABLE_TESTING=OFF
cmake --build "$QUEST_BUILD" --target qft_batch_c --parallel 16

grep -E "^MPI_C_HEADER_DIR" "$QUEST_BUILD/CMakeCache.txt"
ls -la "$QUEST_BUILD/examples/dmr/qft_batch_c"
