#!/bin/bash
#SBATCH -N65
#SBATCH --exclusive
#SBATCH --time=01:30:00
#SBATCH --qos=gp_bsccs
#SBATCH -A bsc85
#SBATCH -o slurm4dmr_%j.out

# Reconfiguration matrix as one nested job per pair of node counts, all inside
# a single Slurm4DMR v3 instance (first node: nested controller, other 64:
# compute). Each pair a<b runs a,b,a,b,...,a in its own DVM, launched on a
# nodes, so every a→b and b→a is measured with the same short history. From
# the QuEST root:
#   sbatch experiments/reconfig_matrix_dmr3/sbatch_pairs_mn5.sh [reps] [node counts...]
# Up to 32 nodes in debug (the controller shares a node):
#   SDV3_CTLD_DEDICATED=0 sbatch -N32 --qos=gp_debug --time=01:00:00 \
#     experiments/reconfig_matrix_dmr3/sbatch_pairs_mn5.sh 3 1 2 4 8 16 32
# Logs: reconfig_matrix_dmr3_<this job id>_<a>_<b>.log in the QuEST root.

module use /apps/GPP/DMR/dmr-modules
module load slurm4dmr

export QUEST_ROOT=$SLURM_SUBMIT_DIR
# The nested instance runs the driver without the host job's SLURM_* variables
export MATRIX_HOST_JOB=$SLURM_JOB_ID
export TZ=Europe/Madrid
# Flat PRRTE routing tree: works around the dropped-WIREUP hang after a grow
# (validated not to change the measured times up to 16 nodes)
export PRTE_MCA_rml_base_radix=${PRTE_MCA_rml_base_radix:-1024}

cd "$QUEST_ROOT"
bash "$SDV3_PATH/share/nested-slurm/start-nested-slurm.sh" \
  bash "$QUEST_ROOT/experiments/reconfig_matrix_dmr3/run_pairs_nested.sh" "$@"
