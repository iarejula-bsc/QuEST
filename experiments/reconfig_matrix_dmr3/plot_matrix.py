#!/usr/bin/env python3
"""Heatmap (from x to) of the DMR reconfiguration overhead, from the CSV that
make_matrix.py --csv writes. Cells hold the mean over repetitions.

Usage: python3 plot_matrix.py <csv> <output base> [title]
Writes <output base>.png and <output base>.pdf.
"""
import csv
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import Rectangle

NODES = [1, 2, 4, 8, 16, 32, 64]
BLUE = ["#cde2fb", "#9ec5f4", "#6da7ec", "#3987e5", "#256abf", "#184f95", "#0d366b"]
INK, MUTED, SURFACE, EMPTY = "#1f1f1e", "#6b6a66", "#ffffff", "#f0efec"


def load(path):
    values = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            if row["overhead_s"]:
                values.setdefault((int(row["from"]), int(row["to"])), []).append(
                    float(row["overhead_s"]))
    return values


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    values = load(sys.argv[1])
    title = sys.argv[3] if len(sys.argv) > 3 else "DMR reconfiguration overhead"
    cmap = LinearSegmentedColormap.from_list("seq_blue", BLUE)
    vmax = max(np.mean(v) for v in values.values())
    reps = max(len(v) for v in values.values())

    plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10})
    fig, ax = plt.subplots(figsize=(7.2, 6.4), facecolor=SURFACE)
    for i, a in enumerate(NODES):
        for j, b in enumerate(NODES):
            if a == b:
                ax.text(j + .5, i + .5, "—", ha="center", va="center", color=MUTED)
                continue
            v = values.get((a, b))
            if not v:
                ax.add_patch(Rectangle((j, i), 1, 1, facecolor=EMPTY, edgecolor=SURFACE,
                                       lw=2, hatch="////"))
                continue
            mean = np.mean(v)
            t = mean / vmax
            ax.add_patch(Rectangle((j, i), 1, 1, facecolor=cmap(t), edgecolor=SURFACE, lw=2))
            label = f"{mean:.2f}"
            if len(v) > 1:
                label += f"\n±{np.std(v, ddof=1):.2f}"
            ax.text(j + .5, i + .5, label, ha="center", va="center",
                    color="#ffffff" if t > .55 else INK, fontsize=9 if len(v) > 1 else 10)

    ax.set_xlim(0, len(NODES)); ax.set_ylim(len(NODES), 0)
    ax.set_xticks(np.arange(len(NODES)) + .5, NODES)
    ax.set_yticks(np.arange(len(NODES)) + .5, NODES)
    ax.xaxis.tick_top(); ax.xaxis.set_label_position("top")
    ax.set_xlabel("to (nodes)", color=MUTED); ax.set_ylabel("from (nodes)", color=MUTED)
    ax.tick_params(colors=INK, length=0)
    for spine in ax.spines.values():
        spine.set_visible(False)
    ax.set_aspect("equal")
    ax.set_title(f"{title}\nseconds, from the decision to reconfigure to RECONFIG_COMPLETE",
                 loc="left", color=INK, fontsize=11, pad=12)

    sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(0, vmax))
    cb = fig.colorbar(sm, ax=ax, fraction=.046, pad=.03)
    cb.outline.set_visible(False)
    cb.ax.tick_params(colors=MUTED, length=0)

    note = "Expansions above the diagonal, shrinks below. Hatched: not measured."
    if reps > 1:
        note += " Mean ± std."
    fig.text(.02, .02, note, color=MUTED, fontsize=8.5)
    fig.tight_layout(rect=(0, .04, 1, 1))
    fig.savefig(sys.argv[2] + ".png", dpi=200, facecolor=SURFACE)
    fig.savefig(sys.argv[2] + ".pdf", facecolor=SURFACE)


if __name__ == "__main__":
    main()
