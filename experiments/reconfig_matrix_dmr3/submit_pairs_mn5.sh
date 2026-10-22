#!/bin/bash
# Submit the reconfiguration matrix as one job per pair of node counts a<b:
# each job starts on a nodes and runs a,b,a,b,...,a, so every a→b and b→a is
# measured with the same short history (one extra nested Slurm allocation at
# most). From the QuEST root on an MN5 login node:
#   bash experiments/reconfig_matrix_dmr3/submit_pairs_mn5.sh [reps] [node counts...]
# Defaults: 3 reps, 1 2 4 8 16 32. Extra sbatch options (e.g. --qos=gp_debug)
# go in SBATCH_EXTRA. Writes pairs_<timestamp>.txt with "<jobid> <a> <b>".
set -euo pipefail

REPS=${1:-3}
shift || true
NODES=("$@")
[ ${#NODES[@]} -eq 0 ] && NODES=(1 2 4 8 16 32)

# Flat PRRTE routing tree: works around the dropped-WIREUP hang after a grow
# (validated not to change the measured times up to 16 nodes)
export PRTE_MCA_rml_base_radix=${PRTE_MCA_rml_base_radix:-1024}

list=pairs_$(date +%Y%m%d_%H%M%S).txt
for ((i = 0; i < ${#NODES[@]}; i++)); do
  for ((j = i + 1; j < ${#NODES[@]}; j++)); do
    a=${NODES[i]} b=${NODES[j]}
    seq=$a
    for ((r = 0; r < REPS; r++)); do seq+=,$b,$a; done
    # The controller shares the first node, so the job needs b nodes
    job=$(START_NODES=$a SDV3_CTLD_DEDICATED=0 sbatch --parsable -N "$b" ${SBATCH_EXTRA:-} \
      experiments/reconfig_matrix_dmr3/sbatch_mn5.sh --sequence="$seq")
    echo "$job $a $b" | tee -a "$list"
  done
done
echo "# job list: $list"
