#!/usr/bin/env python3
"""Build the from x to reconfiguration matrices from the log of
examples/dmr/reconfig_matrix_cpp run on DMR v3.

Usage: python3 make_matrix.py <log> [<log> ...] [--latex] [--csv FILE]

Several logs (e.g. one job per pair of node counts) are merged into one
matrix.

Overhead: from the dmr_check that decides the reconfiguration
(DMR_EVENT_EXPAND_RESOURCES_START or DMR_EVENT_SHRINK_TRIGGERED) to
DMR_EVENT_RECONFIG_COMPLETE in the new processes. It covers resource
acquisition, the respawn and the restart of the new processes; what follows
(creating the Qureg again) a non-malleable run would do as well.
Reconfiguration: the <reconfiguration time> field DMR reports with
DMR_EVENT_RECONFIG_COMPLETE (from the start of the reconfiguration, once the
new nodes are granted for expansions, until the respawned processes complete
it). Acquisition (expansions only): DMR_EVENT_EXPAND_RESOURCES_START ->
DMR_EVENT_EXPAND_RESOURCES_END. Cells hold the mean over repetitions, with
the standard deviation when there is more than one sample.

When the log has the "[matrix] process times" lines, the respawn is split at
rank 0 of the new processes into launch (DMR_EVENT_SPAWN_START -> start of
main), QuEST environment (start of main -> end of initQuESTEnv, which includes
MPI_Init) and DMR init (end of initQuESTEnv -> DMR_EVENT_RECONFIG_COMPLETE).

The "allocations" column is the number of resource-manager allocations
(nested Slurm jobs) the new processes find when they start, from DMR's
"PMIx resource discovery found N allocation(s)" debug line.
"""
import re
import statistics
import sys

ANALYTICS = re.compile(
    r"^\[DMR ANALYTICS\],([0-9.]+),([^,]+),([^,]+),(-?\d+),(-?\d+),(-?[0-9.]+),")
HEADER = re.compile(r"^# matrix (nodes|sequence): (.*)$")
PROCESS = re.compile(r"^\[matrix\] process times: main ([0-9.]+) quest_env ([0-9.]+)")
ALLOCATIONS = re.compile(r"PMIx resource discovery found (\d+) allocation")


def parse(path):
    nodes, sequence, samples = None, None, []
    current = None
    acquire_start = acquire = decided = spawn_start = process = None
    allocations = None
    with open(path, errors="replace") as f:
        for line in f:
            m = ALLOCATIONS.search(line)
            if m:
                allocations = int(m.group(1))
                continue
            m = HEADER.match(line)
            if m:
                values = [int(v) for v in m.group(2).split()]
                if m.group(1) == "nodes" and nodes is None:
                    nodes = values
                elif m.group(1) == "sequence" and sequence is None:
                    sequence = values
                    current = values[0]
                continue
            m = PROCESS.match(line)
            if m and spawn_start is not None:
                process = (float(m.group(1)), float(m.group(2)))
                continue
            m = ANALYTICS.match(line)
            if not m:
                continue
            ts, _, event, world, _, reconf = m.groups()
            ts, world, reconf = float(ts), int(world), float(reconf)
            # non-root ranks print the same events with world size 0
            if world <= 0:
                continue
            if event == "DMR_EVENT_EXPAND_RESOURCES_START":
                acquire_start = decided = ts
            elif event == "DMR_EVENT_SHRINK_TRIGGERED":
                decided = ts
            elif event == "DMR_EVENT_SPAWN_START":
                spawn_start = ts
            elif event == "DMR_EVENT_EXPAND_RESOURCES_END" and acquire_start is not None:
                acquire = ts - acquire_start
                acquire_start = None
            elif event == "DMR_EVENT_RECONFIG_COMPLETE" and current is not None:
                overhead = ts - decided if decided is not None else None
                launch = quest_env = dmr_init = None
                if process and spawn_start is not None:
                    launch = process[0] - spawn_start
                    quest_env = process[1] - process[0]
                    dmr_init = ts - process[1]
                samples.append((current, world, reconf, acquire, overhead,
                                launch, quest_env, dmr_init, allocations))
                current, acquire, decided = world, None, None
                spawn_start = process = None
    return nodes, sequence, samples


def cell(values):
    if not values:
        return "–"
    if len(values) == 1:
        return f"{values[0]:.2f}"
    return f"{statistics.mean(values):.2f} ± {statistics.stdev(values):.2f}"


def matrix(nodes, samples, column):
    return {(a, b): [s[column] for s in samples
                     if s[0] == a and s[1] == b and s[column] is not None]
            for a in nodes for b in nodes}


def print_markdown(title, nodes, values):
    print(f"\n{title} (rows: from, columns: to)\n")
    print("| from \\ to | " + " | ".join(str(b) for b in nodes) + " |")
    print("|---" * (len(nodes) + 1) + "|")
    for a in nodes:
        row = ["" if a == b else cell(values[a, b]) for b in nodes]
        print(f"| **{a}** | " + " | ".join(row) + " |")


def print_latex(nodes, values):
    print(r"\begin{tabular}{r" + "c" * len(nodes) + "}")
    print(r"\toprule")
    print(r"From $\backslash$ To & " + " & ".join(str(b) for b in nodes) + r" \\")
    print(r"\midrule")
    for a in nodes:
        row = ["--" if a == b else
               (f"{statistics.mean(values[a, b]):.2f}" if values[a, b] else "")
               for b in nodes]
        print(f"{a} & " + " & ".join(row) + r" \\")
    print(r"\bottomrule")
    print(r"\end{tabular}")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    logs, skip = [], False
    for arg in sys.argv[1:]:
        if skip:
            skip = False
        elif arg == "--csv":
            skip = True
        elif not arg.startswith("--"):
            logs.append(arg)

    nodes, samples = set(), []
    for log in logs:
        log_nodes, sequence, log_samples = parse(log)
        if not sequence:
            sys.exit(f"{log}: no '# matrix sequence' line")
        expected = list(zip(sequence, sequence[1:]))
        observed = [(s[0], s[1]) for s in log_samples]
        if observed != expected[:len(observed)]:
            print(f"warning: {log}: reconfigurations do not follow the planned "
                  "sequence", file=sys.stderr)
        print(f"# {log}: {len(observed)} of {len(expected)} planned "
              "reconfigurations", file=sys.stderr)
        nodes.update(log_nodes)
        samples += log_samples
    nodes = sorted(nodes)

    reconf = matrix(nodes, samples, 2)
    acquire = matrix(nodes, samples, 3)
    overhead = matrix(nodes, samples, 4)

    if "--csv" in sys.argv:
        with open(sys.argv[sys.argv.index("--csv") + 1], "w") as f:
            f.write("from,to,reconfiguration_s,acquisition_s,overhead_s,"
                    "launch_s,quest_env_s,dmr_init_s,allocations\n")
            for a, b, r, *rest, allocs in samples:
                f.write(f"{a},{b},{r}," + ",".join(
                    "" if v is None else f"{v:.6f}" for v in rest) +
                    f",{'' if allocs is None else allocs}\n")

    if "--latex" in sys.argv:
        print_latex(nodes, overhead)
    else:
        print_markdown("Overhead (s)", nodes, overhead)
        print_markdown("Reconfiguration time (s)", nodes, reconf)
        print_markdown("Acquisition time (s), expansions", nodes, acquire)
        if any(s[6] is not None for s in samples):
            print_markdown("Launch: SPAWN_START -> main (s)", nodes,
                           matrix(nodes, samples, 5))
            print_markdown("QuEST environment: main -> end of initQuESTEnv (s)",
                           nodes, matrix(nodes, samples, 6))
            print_markdown("DMR init: end of initQuESTEnv -> RECONFIG_COMPLETE (s)",
                           nodes, matrix(nodes, samples, 7))
        if any(s[8] is not None for s in samples):
            print_markdown("Allocations found by the new processes", nodes,
                           matrix(nodes, samples, 8))


if __name__ == "__main__":
    main()
