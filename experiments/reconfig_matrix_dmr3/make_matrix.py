#!/usr/bin/env python3
"""Build the from x to reconfiguration matrices from the log of
examples/dmr/reconfig_matrix_cpp run on DMR v3.

Usage: python3 make_matrix.py <log> [--latex] [--csv FILE]

Reconfiguration: the <reconfiguration time> field DMR reports with
DMR_EVENT_RECONFIG_COMPLETE (from the start of the reconfiguration, once the
new nodes are granted for expansions, until the respawned processes complete
it). Acquisition (expansions only): DMR_EVENT_EXPAND_RESOURCES_START ->
DMR_EVENT_EXPAND_RESOURCES_END. Cells hold the mean over repetitions, with
the standard deviation when there is more than one sample.
"""
import re
import statistics
import sys

ANALYTICS = re.compile(
    r"^\[DMR ANALYTICS\],([0-9.]+),([^,]+),([^,]+),(-?\d+),(-?\d+),(-?[0-9.]+),")
HEADER = re.compile(r"^# matrix (nodes|sequence): (.*)$")


def parse(path):
    nodes, sequence, samples = None, None, []
    current = None
    acquire_start = acquire = None
    with open(path, errors="replace") as f:
        for line in f:
            m = HEADER.match(line)
            if m:
                values = [int(v) for v in m.group(2).split()]
                if m.group(1) == "nodes" and nodes is None:
                    nodes = values
                elif m.group(1) == "sequence" and sequence is None:
                    sequence = values
                    current = values[0]
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
                acquire_start = ts
            elif event == "DMR_EVENT_EXPAND_RESOURCES_END" and acquire_start is not None:
                acquire = ts - acquire_start
                acquire_start = None
            elif event == "DMR_EVENT_RECONFIG_COMPLETE" and current is not None:
                samples.append((current, world, reconf, acquire))
                current, acquire = world, None
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
    nodes, sequence, samples = parse(sys.argv[1])
    if not sequence:
        sys.exit("no '# matrix sequence' line in the log")

    expected = list(zip(sequence, sequence[1:]))
    observed = [(s[0], s[1]) for s in samples]
    if observed != expected[:len(observed)]:
        print("warning: reconfigurations do not follow the planned sequence",
              file=sys.stderr)
    print(f"# {len(observed)} of {len(expected)} planned reconfigurations",
          file=sys.stderr)

    reconf = matrix(nodes, samples, 2)
    acquire = matrix(nodes, samples, 3)

    if "--csv" in sys.argv:
        with open(sys.argv[sys.argv.index("--csv") + 1], "w") as f:
            f.write("from,to,reconfiguration_s,acquisition_s\n")
            for a, b, r, q in samples:
                f.write(f"{a},{b},{r},{'' if q is None else f'{q:.6f}'}\n")

    if "--latex" in sys.argv:
        print_latex(nodes, reconf)
    else:
        print_markdown("Reconfiguration time (s)", nodes, reconf)
        print_markdown("Acquisition time (s), expansions", nodes, acquire)


if __name__ == "__main__":
    main()
