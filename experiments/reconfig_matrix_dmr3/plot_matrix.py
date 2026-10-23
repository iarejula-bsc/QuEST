#!/usr/bin/env python3
"""Heatmap (from x to) of the DMR reconfiguration overhead, from the CSV that
make_matrix.py --csv writes. Cells hold the mean over repetitions.

Usage: python3 plot_matrix.py <csv> <output base> [title] [extra note]
       python3 plot_matrix.py <csv> <output base> --bare

--bare draws only the matrix and the colour bar, sized for one column of a
paper, for a figure whose caption carries the explanation.
Writes <output base>.png and <output base>.pdf.
"""
import csv
import sys
import textwrap

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import Rectangle

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
    bare = "--bare" in sys.argv
    values = load(sys.argv[1])
    NODES = sorted({n for pair in values for n in pair})
    title = sys.argv[3] if len(sys.argv) > 3 and not bare else "DMR reconfiguration overhead"
    cmap = LinearSegmentedColormap.from_list("seq_blue", BLUE)
    vmax = max(np.mean(v) for v in values.values())
    reps = max(len(v) for v in values.values())

    plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 8 if bare else 10})
    side = 1.6 + 0.8 * len(NODES)
    figsize = (3.5, 3.0) if bare else (max(side + 1.6, 7.0), side + 1.2)
    fig, ax = plt.subplots(figsize=figsize, facecolor=SURFACE)
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
            ax.text(j + .5, i + .5, f"{mean:.2f}", ha="center", va="center",
                    color="#ffffff" if t > .55 else INK, fontsize=7 if bare else 10)

    ax.set_xlim(0, len(NODES)); ax.set_ylim(len(NODES), 0)
    ax.set_xticks(np.arange(len(NODES)) + .5, NODES)
    ax.set_yticks(np.arange(len(NODES)) + .5, NODES)
    ax.xaxis.tick_top(); ax.xaxis.set_label_position("top")
    ax.set_xlabel("to (nodes)", color=MUTED); ax.set_ylabel("from (nodes)", color=MUTED)
    ax.tick_params(colors=INK, length=0)
    for spine in ax.spines.values():
        spine.set_visible(False)
    ax.set_aspect("equal")
    if not bare:
        fig.text(.02, .985, f"{title}\nseconds, from the decision to reconfigure to RECONFIG_COMPLETE",
                 ha="left", va="top", color=INK, fontsize=11)

    sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(0, vmax))
    cb = fig.colorbar(sm, ax=ax, fraction=.046, pad=.03)
    cb.outline.set_visible(False)
    cb.ax.tick_params(colors=MUTED, length=0)

    if bare:
        cb.set_label("overhead (s)", color=MUTED)
        fig.tight_layout(pad=0.3)
    else:
        note = "Expansions above the diagonal, shrinks below. Hatched: not measured."
        if reps > 1:
            note += f" Mean of {reps} runs."
        if len(sys.argv) > 4:
            note += "\n" + textwrap.fill(sys.argv[4], 105)
        fig.text(.02, .02, note, color=MUTED, fontsize=8.5)
        fig.tight_layout(rect=(0, .02 + .02 * note.count("\n"), 1, .9))
    crop = {"bbox_inches": "tight", "pad_inches": 0.03} if bare else {}
    fig.savefig(sys.argv[2] + ".png", dpi=200, facecolor=SURFACE, **crop)
    fig.savefig(sys.argv[2] + ".pdf", facecolor=SURFACE, **crop)


if __name__ == "__main__":
    main()
