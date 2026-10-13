#!/bin/bash
#SBATCH -N4
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH -o qft_batch_minidmr_%j.log

# Scaled-down qft_batch run on MiniDMR: same resize policy shape as on MN5,
# with qubit thresholds small enough for laptop containers.
#   q < 11 -> 1 node, q >= 11 -> 2^(q-10) nodes, capped at 2^2 = 4
# Default batch 11 12 8 13 -> 2, 4, 1, 4 nodes (launch at 4, so the first
# circuit shrinks, then expand, shrink, expand).
#
# Submit from the QuEST root:
#   minidmr sbatch -- experiments/qft_batch_dmr3/run_minidmr.sh [qubits...]

QUEST_ROOT=${QUEST_ROOT:-$SLURM_SUBMIT_DIR}
# The MiniDMR image exports DMR_PATH=/usr/local (its own DMR), so use our own variable
DMR_PATH=${DMR_V3_PATH:-$HOME/.local/dmrv3-modlike}
BINARY=${BINARY:-$QUEST_ROOT/build-minidmr-modlike/examples/dmr/qft_batch_c}

export PATH=$DMR_PATH/bin:$PATH
export LD_LIBRARY_PATH=$DMR_PATH/lib:$LD_LIBRARY_PATH

export DMR_PRINT_ANALYTICS=1
export DMR_DEBUG_LEVEL=1
export DMR_PROCS_PER_NODE=1
# Wait in dmr_check until an expansion is granted, so each circuit runs at
# the size the policy chose
export DMR_BLOCKING_REQ=1

export QUEST_MAL_MIN_QUBITS=11
export QUEST_MAL_BASE_QUBITS=10
export QUEST_MAL_MAX_NODE_EXP=2

export OMP_NUM_THREADS=1

# Same as the MN5 dmrv3-dlb-gpp module: DLB/TALP preloaded
export LD_PRELOAD=/usr/local/lib/libdlb_mpi.so
export DLB_ARGS="--talp --quiet"

QUBITS=("$@")
[ ${#QUBITS[@]} -eq 0 ] && QUBITS=(11 12 8 13)

cd "$QUEST_ROOT"
rm -f quest_circuit_checkpoint.bin

NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)

echo "# dmr: $(dmr_wrapper --version) at $DMR_PATH"
echo "# quest: $(git -C "$QUEST_ROOT" rev-parse --short HEAD)"
echo "# qubits: ${QUBITS[*]}"

set -x
dmr_wrapper mpirun --host "$NODELIST_WITH_COUNTS" "$BINARY" "${QUBITS[@]}"
