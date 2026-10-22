#!/bin/bash
#SBATCH -N1
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH -o repro_minidmr_%j.log

# Reproduction attempt of the prted failure after a partial shrink, on
# MiniDMR (4 nodes): launched on 1 node, then 1→4 creates a 3-node allocation,
# every 4→2 shrinks it partially and every 2→4 asks for the released nodes
# again. prte runs through make_debug_wrapper.sh, so its output and the
# daemons' land in prte_<jobid>.log.
#
# Submit from the QuEST root, with the node watchers running
# (see run_host.sh):
#   minidmr sbatch -- experiments/reconfig_matrix_dmr3/repro/run_minidmr.sh [cycles]

QUEST_ROOT=${QUEST_ROOT:-$SLURM_SUBMIT_DIR}
DMR_PATH=${DMR_V3_PATH:-$HOME/.local/dmrv3-modlike}
BINARY=${BINARY:-$QUEST_ROOT/build-minidmr-modlike/examples/dmr/reconfig_matrix_cpp}
REPRO_DIR=$QUEST_ROOT/experiments/reconfig_matrix_dmr3/repro

export PATH=$DMR_PATH/bin:$PATH
export LD_LIBRARY_PATH=$DMR_PATH/lib:$LD_LIBRARY_PATH

export DMR_PRINT_ANALYTICS=1
export DMR_DEBUG_LEVEL=1
export DMR_PROCS_PER_NODE=1
export DMR_BLOCKING_REQ=1
export QUEST_MAL_MIN_QUBITS=11
export QUEST_MAL_BASE_QUBITS=10
export QUEST_MAL_MAX_NODE_EXP=2
# Reconfigure back to back; the expand delay only avoids the separate
# "no new nodes" bug of an expand right after a shrink
export QUEST_MATRIX_STEP_SLEEP_S=${QUEST_MATRIX_STEP_SLEEP_S:-0}
export QUEST_MAL_EXPAND_DELAY_S=${QUEST_MAL_EXPAND_DELAY_S:-2}
export OMP_NUM_THREADS=1
export LD_PRELOAD=/usr/local/lib/libdlb_mpi.so
export DLB_ARGS="--talp --quiet"

export DMR_PRTE_LOG=$QUEST_ROOT/prte_$SLURM_JOB_ID.log
export DMR_PRTE_EXTRA=${DMR_PRTE_EXTRA:---leave-session-attached --prtemca plm_base_verbose 5}

CYCLES=${1:-25}
SEQUENCE=1
for ((c = 0; c < CYCLES; c++)); do SEQUENCE+=,4,2; done

cd "$QUEST_ROOT"
rm -f quest_circuit_checkpoint.bin
WRAPPER=$HOME/.cache/dmr_wrapper_debug_$SLURM_JOB_ID
mkdir -p "$(dirname "$WRAPPER")"
bash "$REPRO_DIR/make_debug_wrapper.sh" "$WRAPPER" || exit 1

NODE=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | head -1)
echo "# quest: $(git -C "$QUEST_ROOT" rev-parse --short HEAD)"
echo "# sequence: $SEQUENCE"
echo "# prte log: $DMR_PRTE_LOG"

set -x
"$WRAPPER" mpirun --host "$NODE:$DMR_PROCS_PER_NODE" "$BINARY" "--sequence=$SEQUENCE"
