#!/bin/bash
#SBATCH -N1
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH -o reconfig_matrix_minidmr_%j.log

# Scaled-down reconfig_matrix run on MiniDMR (4 nodes): node counts 1 2 4,
# submitted on 1 node so the other 3 stay free for the expansions, with 2^10
# amplitudes per node.
#   1 node -> 10 qubits, n >= 2 nodes -> 10 + log2(n) qubits
#
# Submit from the QuEST root:
#   minidmr sbatch -- experiments/reconfig_matrix_dmr3/run_minidmr.sh [--reps=R] [node counts...]

QUEST_ROOT=${QUEST_ROOT:-$SLURM_SUBMIT_DIR}
# The MiniDMR image exports DMR_PATH=/usr/local (its own DMR), so use our own variable
DMR_PATH=${DMR_V3_PATH:-$HOME/.local/dmrv3-modlike}
BINARY=${BINARY:-$QUEST_ROOT/build-minidmr-modlike/examples/dmr/reconfig_matrix_cpp}

export PATH=$DMR_PATH/bin:$PATH
export LD_LIBRARY_PATH=$DMR_PATH/lib:$LD_LIBRARY_PATH

export DMR_PRINT_ANALYTICS=1
export DMR_DEBUG_LEVEL=1
export DMR_PROCS_PER_NODE=1
export DMR_BLOCKING_REQ=1

export QUEST_MAL_MIN_QUBITS=11
export QUEST_MAL_BASE_QUBITS=10
export QUEST_MAL_MAX_NODE_EXP=2

export OMP_NUM_THREADS=1

# Same as the MN5 dmrv3-dlb-gpp module: DLB/TALP preloaded
export LD_PRELOAD=/usr/local/lib/libdlb_mpi.so
export DLB_ARGS="--talp --quiet"

ARGS=("$@")
[ ${#ARGS[@]} -eq 0 ] && ARGS=(--reps=1 1 2 4)

cd "$QUEST_ROOT"
# One executor checkpoint per job: jobs share this working directory
export QUEST_MAL_CHECKPOINT_FILE=$QUEST_ROOT/quest_circuit_checkpoint_${MATRIX_HOST_JOB:+${MATRIX_HOST_JOB}_}$SLURM_JOB_ID.bin
rm -f "$QUEST_MAL_CHECKPOINT_FILE"

NODE=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | head -1)

echo "# dmr: $(dmr_wrapper --version) at $DMR_PATH"
echo "# quest: $(git -C "$QUEST_ROOT" rev-parse --short HEAD)"
echo "# args: ${ARGS[*]}"

set -x
dmr_wrapper mpirun --host "$NODE:$DMR_PROCS_PER_NODE" "$BINARY" "${ARGS[@]}"
