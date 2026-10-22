#!/bin/bash
# Run the MiniDMR reproduction from the host: start watch_node.sh in every
# slurmd container, submit run_minidmr.sh, wait for it, stop the watchers.
# Logs: repro_minidmr_<job>.log and prte_<job>.log in the QuEST root, and
# watch_<start time>_<node>.log, started before the job is submitted.
#   bash experiments/reconfig_matrix_dmr3/repro/run_host.sh [cycles]
set -u
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
nodes=$(docker ps --format '{{.Names}}' | grep -E '^mc-slurmd-[0-9]+$' | sort)

tag=$(date +%Y%m%d_%H%M%S)
for n in $nodes; do
	docker exec "$n" bash -c 'rm -f /dev/shm/DLB_*'
	docker exec -d "$n" bash "$PWD/experiments/reconfig_matrix_dmr3/repro/watch_node.sh" "$PWD/watch_${tag}_$n.log"
done
job=$(minidmr exec -- bash -lc "cd $PWD && sbatch --parsable experiments/reconfig_matrix_dmr3/repro/run_minidmr.sh ${1:-25}" | tail -1)
echo "job $job, watch logs watch_${tag}_*.log"

while minidmr exec -- bash -lc "squeue -h -j $job" 2>/dev/null | grep -q .; do sleep 5; done
for n in $nodes; do
	docker exec "$n" bash -c 'for p in /proc/[0-9]*; do grep -qa watch_node.sh "$p/cmdline" 2>/dev/null && kill "${p#/proc/}"; done'
done
echo "job $job finished"
