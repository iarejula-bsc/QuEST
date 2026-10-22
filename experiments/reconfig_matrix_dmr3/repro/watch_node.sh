#!/bin/bash
# Every 0.5 s, log the prted daemons alive on this node (with their DVM vpid)
# and the PRRTE session directories under /tmp. Runs until killed.
#   watch_node.sh <log file>
log=$1
while true; do
	ts=$(date +%s.%N)
	for p in /proc/[0-9]*; do
		cmd=$(tr '\0' ' ' < "$p/cmdline" 2>/dev/null) || continue
		case "$cmd" in
		*prted*) echo "$ts prted pid=${p#/proc/} vpid=$(sed -n 's/.*ess_base_vpid \"\{0,1\}\([0-9]*\).*/\1/p' <<< "$cmd") state=$(awk '{print $3}' "$p/stat" 2>/dev/null)" ;;
		esac
	done
	for d in /tmp/prte.*; do
		[ -e "$d" ] && echo "$ts session $d $(find "$d" -mindepth 1 -maxdepth 2 2>/dev/null | wc -l)"
	done
	sleep 0.5
done >> "$log"
