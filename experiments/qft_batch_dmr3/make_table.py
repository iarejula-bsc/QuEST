#!/usr/bin/env python3
"""Build the QFT-batch table (execution time + reconfiguration time per circuit)
from the log of examples/dmr/qft_batch_c run on DMR v3.

Usage: python3 make_table.py <log> [--latex]

Reconfiguration time follows DMR v3's own <reconfiguration time> field:
  expand: DMR_EVENT_EXPAND_RESOURCES_END -> DMR_EVENT_RECONFIG_COMPLETE
  shrink: DMR_EVENT_SHRINK_TRIGGERED    -> DMR_EVENT_RECONFIG_COMPLETE
The wait for the resource manager and the DVM
(EXPAND_RESOURCES_START -> EXPAND_RESOURCES_END) is reported separately as
"Acquisition Time".
"""
import re
import sys

ANALYTICS = re.compile(r"^\[DMR ANALYTICS\],([0-9.]+),([^,]+),([^,]+),(-?\d+),(-?\d+),")
PROB = re.compile(r"^prob of \|0\.\.0> \((\d+) qubits\)")


def parse(path):
    qubits, circuits, reconfs = [], [], []
    open_circuit = None
    reconf = {}
    with open(path, errors="replace") as f:
        for line in f:
            m = PROB.match(line)
            if m:
                qubits.append(int(m.group(1)))
                continue
            m = ANALYTICS.match(line)
            if not m:
                continue
            ts, _, event, world, nodes = m.groups()
            ts, world, nodes = float(ts), int(world), int(nodes)
            # non-root ranks print the same user events with world size 0
            if world <= 0:
                continue
            if event == "circuit_start":
                open_circuit = (ts, nodes)
            elif event == "circuit_end" and open_circuit:
                circuits.append((open_circuit[1], ts - open_circuit[0]))
                open_circuit = None
            elif event == "DMR_EVENT_EXPAND_RESOURCES_START":
                reconf = {"kind": "expand", "acquire_start": ts}
            elif event == "DMR_EVENT_EXPAND_RESOURCES_END" and reconf:
                reconf["start"] = ts
            elif event == "DMR_EVENT_SHRINK_TRIGGERED":
                reconf = {"kind": "shrink", "start": ts}
            elif event == "DMR_EVENT_RECONFIG_COMPLETE" and "start" in reconf:
                acquire = (reconf["start"] - reconf["acquire_start"]
                           if "acquire_start" in reconf else None)
                reconfs.append((nodes, ts - reconf["start"], acquire))
                reconf = {}
    return qubits, circuits, reconfs


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    qubits, circuits, reconfs = parse(sys.argv[1])
    if len(qubits) != len(circuits):
        print(f"warning: {len(qubits)} qubit labels vs {len(circuits)} circuits",
              file=sys.stderr)

    # A reconfiguration happens before every circuit whose node count differs
    # from the previous one; the first circuit is compared with the launch size,
    # which the first reconfiguration's start does not record, so it is assumed
    # to have reconfigured whenever there are more reconfigurations than
    # transitions between circuits.
    transitions = sum(1 for i in range(1, len(circuits))
                      if circuits[i][0] != circuits[i - 1][0])
    initial = reconfs.pop(0) if len(reconfs) > transitions else None

    rows = []
    for i, (nodes, exec_t) in enumerate(circuits):
        q = qubits[i] if i < len(qubits) else "?"
        if i + 1 < len(circuits) and circuits[i + 1][0] != nodes:
            to_nodes, reconf_t, acquire_t = reconfs.pop(0)
            acq = f"{acquire_t:.3f}" if acquire_t is not None else "–"
            rows.append((q, nodes, exec_t, f"{nodes} → {to_nodes}", f"{reconf_t:.3f}", acq))
        else:
            rows.append((q, nodes, exec_t, "–", "–", "–"))

    if initial:
        acq = f", acquisition {initial[2]:.3f} s" if initial[2] is not None else ""
        print(f"# initial reconfiguration (launch → {initial[0]} nodes): "
              f"{initial[1]:.3f} s{acq}", file=sys.stderr)

    if "--latex" in sys.argv:
        print(r"Qubits & Nodes & Execution Time (s) & Transition & Reconfiguration Time (s) \\ \hline")
        for q, n, t, tr, rt, _ in rows:
            tr = tr.replace("→", r"$\rightarrow$")
            print(f"{q} & {n} & {t:.3f} & {tr} & {rt} \\\\")
    else:
        print("| Qubits | Nodes | Execution Time (s) | Transition | Reconfiguration Time (s) | Acquisition Time (s) |")
        print("|---|---|---|---|---|---|")
        for q, n, t, tr, rt, at in rows:
            print(f"| {q} | {n} | {t:.3f} | {tr} | {rt} | {at} |")


if __name__ == "__main__":
    main()
