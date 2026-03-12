#!/bin/bash
#SBATCH --time=00:10:00
#SBATCH --exclusive

# Number of nodes

# Edit configuration details if needed below
#SBATCH --qos=gp_bsccs
#SBATCH -A bsc85

#SBATCH -N32
#SBATCH --output=quest_%j.log
export DMR_PROCS_PER_NODE=1
NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)
NODELIST_WITH_COUNTS="${NODELIST_WITH_COUNTS%,}"

set -x

export DMR_PATH=$HOME/.local/dmr_jobs

export OMP_NUM_THREADS=112
$DMR_PATH/bin/dmr_wrapper prterun --host $NODELIST_WITH_COUNTS  build-dmr/examples/dmr/example_without_c
