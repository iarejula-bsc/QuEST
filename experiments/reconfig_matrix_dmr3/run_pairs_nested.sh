#!/bin/bash
# Run by sbatch_pairs_mn5.sh inside the nested Slurm instance: submit
# job_mn5.sh once per pair of node counts a<b, one after another, each on a
# nodes with the sequence a,b,a,b,...,a (reps round trips).
#   run_pairs_nested.sh [reps] [node counts...]

REPS=${1:-3}
shift
NODES=("$@")
[ ${#NODES[@]} -eq 0 ] && NODES=(1 2 4 8 16 32 64)
# Seconds between pairs, so the nodes the previous pair released are back to
# idle before the next one asks for them
PAIR_PAUSE_S=${PAIR_PAUSE_S:-20}
# Time limit of each pair's nested job, so a hang costs one pair, not the run
PAIR_TIME=${PAIR_TIME:-00:15:00}

for ((i = 0; i < ${#NODES[@]}; i++)); do
  for ((j = i + 1; j < ${#NODES[@]}; j++)); do
    a=${NODES[i]} b=${NODES[j]}
    seq=$a
    for ((r = 0; r < REPS; r++)); do seq+=,$b,$a; done
    export MATRIX_LOG=$QUEST_ROOT/reconfig_matrix_dmr3_${MATRIX_HOST_JOB}_${a}_${b}.log
    echo "# pair $a $b: $(date +%T) -> $MATRIX_LOG"
    sbatch --wait -N "$a" --time="$PAIR_TIME" \
      "$QUEST_ROOT/experiments/reconfig_matrix_dmr3/job_mn5.sh" --sequence="$seq"
    echo "# pair $a $b: exit $? at $(date +%T)"
    sleep "$PAIR_PAUSE_S"
  done
done
