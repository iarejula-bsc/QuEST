#!/bin/bash
#SBATCH --time=00:05:00
#SBATCH --exclusive


# Number of nodes
#SBATCH -N8
#SBATCH --output=quest_malleable_%j.log

echo $DMR_PATH
echo $SLURM_ROOT

export PATH=$SLURM_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$DMR_PATH/build/lib:$LD_LIBRARY_PATH

# Processes per node (PPN) to use at launch. DMR will also read this environment variable and use it for expands.
export DMR_PROCS_PER_NODE=1

# Formatted hostlist to give to PRRTE, e.g. my_host1:10,myhost2:10
NODELIST_WITH_COUNTS=$(scontrol show hostnames "$SLURM_JOB_NODELIST" | awk -v n="$DMR_PROCS_PER_NODE" '{print $1 ":" n}' | paste -sd,)


set -x

#export DMR_PATH=$HOME/.local/dmr_jobs

export OMP_NUM_THREADS=112
$DMR_PATH/bin/dmr_wrapper mpirun --host $NODELIST_WITH_COUNTS  build-malleable-slurm4dmr/examples/dmr/example_multi_c
