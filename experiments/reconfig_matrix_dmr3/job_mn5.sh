#!/bin/bash
#SBATCH -N1
#SBATCH --exclusive
#SBATCH --time=01:00:00
#SBATCH -o /dev/null

# reconfig_matrix in the nested Slurm4DMR v3 instance, submitted by
# sbatch_mn5.sh. It starts on 1 node and goes through every ordered pair of
# node counts (default 1 2 4 8 16 32 64), REPS times (default 3). Each step
# only creates a Qureg with 2^26 amplitudes per node:
#   1 node -> 26 qubits, n >= 2 nodes -> 26 + log2(n) qubits

exec > "${MATRIX_LOG:-${SLURM_SUBMIT_DIR}/reconfig_matrix_dmr3_nested$SLURM_JOB_ID.log}" 2>&1

QUEST_ROOT=${QUEST_ROOT:-$SLURM_SUBMIT_DIR}
BINARY=${BINARY:-$QUEST_ROOT/build-dmr3-mn5/examples/dmr/reconfig_matrix_cpp}

module purge
module load gcc/14.1.0_binutils241
module use /apps/GPP/DMR/dmr-modules
module load dmrv3-dlb-gpp

export DMR_PRINT_ANALYTICS=1
export DMR_DEBUG_LEVEL=1
export DMR_PROCS_PER_NODE=1
# Wait in dmr_check until an expansion is granted, so each step runs at
# the size the policy chose
export DMR_BLOCKING_REQ=1
# Seconds to wait before each expand so the previous shrink's release finishes
export QUEST_MAL_EXPAND_DELAY_S=${QUEST_MAL_EXPAND_DELAY_S:-10}
# 27 qubits -> 2 nodes, so every power of two up to 64 is reachable
export QUEST_MAL_MIN_QUBITS=27
export QUEST_MAL_BASE_QUBITS=26
export QUEST_MAL_MAX_NODE_EXP=6
export OMP_NUM_THREADS=112
export TZ=Europe/Madrid

ARGS=("$@")
[ ${#ARGS[@]} -eq 0 ] && ARGS=(--reps=${REPS:-3})

cd "$QUEST_ROOT"
rm -f quest_circuit_checkpoint.bin

NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)

echo "# dmr: $DMR_PATH ($(dmr_wrapper --version), $(stat -c %y $DMR_PATH/lib/libdmr.so.3.0.0))"
echo "# prrte: $(prte_info 2>/dev/null | sed -n 's/.*PRRTE repo revision: //p')"
echo "# quest: $(git -C "$QUEST_ROOT" rev-parse --short HEAD)"
echo "# args: ${ARGS[*]}"

set -x
dmr_wrapper mpirun --host "$NODELIST_WITH_COUNTS" "$BINARY" "${ARGS[@]}"
