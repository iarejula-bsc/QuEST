#!/bin/bash
#SBATCH -N65
#SBATCH --exclusive
#SBATCH --time=00:50:00
#SBATCH --qos=gp_bsccs
#SBATCH -A bsc85
#SBATCH -o slurm4dmr_%j.out

# Bring up Slurm4DMR v3 on this allocation (first node: nested controller,
# other 64: compute) and run job_mn5.sh inside it. From the QuEST root:
#   sbatch experiments/reconfig_matrix_dmr3/sbatch_mn5.sh [--reps=R] [node counts...]
# The malleable job's log is reconfig_matrix_dmr3_<this job id>.log in the QuEST root.

module use /apps/GPP/DMR/dmr-modules
module load slurm4dmr

export QUEST_ROOT=$SLURM_SUBMIT_DIR
export MATRIX_LOG=$QUEST_ROOT/reconfig_matrix_dmr3_$SLURM_JOB_ID.log
export TZ=Europe/Madrid

cd "$QUEST_ROOT"
bash "$SDV3_PATH/share/nested-slurm/start_slurm4dmr.sh" \
  "$QUEST_ROOT/experiments/reconfig_matrix_dmr3/job_mn5.sh" "$@"
