#!/bin/bash
#SBATCH -N1
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH -o /dev/null

# qft_batch in the nested Slurm4DMR v3 instance, submitted by sbatch_mn5.sh.
# It starts on 1 node and DMR expands it to the size of the first circuit; the
# rest of the instance's nodes stay free for the expansions. Nested job ids
# restart at 1 on every instance, so the log is named after the host job
# instead (QFT_LOG, set by sbatch_mn5.sh).
#   q < 28 -> 1 node, q >= 28 -> 2^(q-26) nodes, capped at 2^QUEST_MAL_MAX_NODE_EXP (64)
# Default batch 28 30 24 34 -> 4, 16, 1, 64 nodes.

exec > "${QFT_LOG:-${SLURM_SUBMIT_DIR}/qft_batch_dmr3_nested$SLURM_JOB_ID.log}" 2>&1

QUEST_ROOT=${QUEST_ROOT:-$SLURM_SUBMIT_DIR}
BINARY=${BINARY:-$QUEST_ROOT/build-dmr3-mn5/examples/dmr/qft_batch_c}

module purge
module load gcc/14.1.0_binutils241
module use /apps/GPP/DMR/dmr-modules
module load dmrv3-dlb-gpp

export DMR_PRINT_ANALYTICS=1
export DMR_DEBUG_LEVEL=1
export DMR_PROCS_PER_NODE=1
# Wait in dmr_check until an expansion is granted, so each circuit runs at
# the size the policy chose
export DMR_BLOCKING_REQ=1
# Seconds to wait before each expand so the previous shrink's release finishes
export QUEST_MAL_EXPAND_DELAY_S=${QUEST_MAL_EXPAND_DELAY_S:-10}
export OMP_NUM_THREADS=112
export TZ=Europe/Madrid

QUBITS=("$@")
[ ${#QUBITS[@]} -eq 0 ] && QUBITS=(28 30 24 34)

cd "$QUEST_ROOT"
rm -f quest_circuit_checkpoint.bin

NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)

echo "# dmr: $DMR_PATH ($(dmr_wrapper --version), libdmr $(readlink $DMR_PATH/lib/libdmr.so), $(stat -c %y $DMR_PATH/lib/libdmr.so.3.0.0))"
echo "# quest: $(git -C "$QUEST_ROOT" rev-parse --short HEAD)"
echo "# qubits: ${QUBITS[*]}"
echo "# nodes: $SLURM_JOB_NUM_NODES"

set -x
dmr_wrapper mpirun --host "$NODELIST_WITH_COUNTS" "$BINARY" "${QUBITS[@]}"
