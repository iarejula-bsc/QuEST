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
# START_NODES=N launches the malleable job on N nodes of the nested instance
# instead of 1, through a copy of job_mn5.sh with that node count: the nested
# sbatch takes no -N from the environment
JOB_SCRIPT=$QUEST_ROOT/experiments/reconfig_matrix_dmr3/job_mn5.sh
if [ "${START_NODES:-1}" != 1 ]; then
  START_SCRIPT=$QUEST_ROOT/experiments/reconfig_matrix_dmr3/.job_mn5_start${START_NODES}_$SLURM_JOB_ID.sh
  sed "s/^#SBATCH -N1\$/#SBATCH -N$START_NODES/" "$JOB_SCRIPT" > "$START_SCRIPT"
  JOB_SCRIPT=$START_SCRIPT
fi
bash "$SDV3_PATH/share/nested-slurm/start_slurm4dmr.sh" "$JOB_SCRIPT" "$@"
[ -n "${START_SCRIPT:-}" ] && rm -f "$START_SCRIPT"
