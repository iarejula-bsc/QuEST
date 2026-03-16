#!/bin/bash
#SBATCH --time=00:10:00
#SBATCH --exclusive


# Number of nodes
#SBATCH -N16
#SBATCH --output=quest_malleable_%j.log

export DMR_PROCS_PER_NODE=1

NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)

echo $NODELIST_WITH_COUNTS
echo $SLURM_JOB_NODELIST
scontrol show hostnames "$SLURM_JOB_NODELIST"
set -x

#export DMR_PATH=$HOME/.local/dmr_jobs

export OMP_NUM_THREADS=112
$DMR_PATH/bin/dmr_wrapper mpirun --host $NODELIST_WITH_COUNTS  build-malleable-slurm4dmr/examples/dmr/example_without_c
