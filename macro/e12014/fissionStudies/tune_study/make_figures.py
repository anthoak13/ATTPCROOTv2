#!/usr/bin/env python3
"""Figures of the fit tuning study from tune_report.py's CSVs (--csv and --z-csv).

    ./make_figures.py phaseA.csv phaseA_z.csv figures/phaseA --base base --floor base2

Sections are grouped into families by name: a family is a list of (section, label) pairs, given in
FAMILIES below or by --family NAME=sec:label,sec:label (repeatable, replaces FAMILIES).

Writes <prefix>_cost.png (the paired core bias shift and failure-rate change against the base, and the
core width, each against CPU per event), <prefix>_bias_z.png (dZ mean against true Z, one panel per family) and, with --angle-csv,
<prefix>_bias_angle.png (dZ mean against |90 deg - true decay angle|).
"""
import argparse
import csv
import math

import matplotlib
import matplotlib.ticker

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.colors import LinearSegmentedColormap  # noqa: E402

# Reference categorical palette (fixed order) and text inks
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"]
MARKERS = ["o", "s", "^", "D"]
INK, INK2, MUTED, GRID = "#0b0b0b", "#52514e", "#8a8984", "#e4e3df"

FAMILIES = {
    "step (mm)": [("step05", "0.5"), ("base", "1"), ("step2", "2"), ("step3", "3"), ("step4", "4"), ("step6", "6")],
    "random points": [("base", "1000"), ("pts300", "300"), ("pts100", "100"), ("pts30", "30"), ("pts10", "10")],
    "fixed pattern": [("pat300", "300"), ("pat100", "100"), ("pat30", "30"), ("pat10", "10")],
    "switches": [("zs2", "2σ window"), ("gain0", "no gain fluct.")],
}


def read_rows(path):
    with open(path) as f:
        return {r["fit"]: {k: (float(v) if v not in ("", None) and k != "fit" else v) for k, v in r.items()}
                for r in csv.DictReader(f)}


def read_binned(path, column, xof):
    """{fit: [(x, dZ mean, error)]} from tune_report.py's --z-csv or --angle-csv."""
    out = {}
    with open(path) as f:
        for r in csv.DictReader(f):
            out.setdefault(r["fit"], []).append((xof(r[column]), float(r["dz_mean"]), float(r["dz_mean_err"] or "nan")))
    return out


def angle_center(label):
    lo, hi = map(float, label.split("-"))
    return (lo + hi) / 2


def style(ax, xlabel, ylabel):
    ax.set_xlabel(xlabel, color=INK2)
    ax.set_ylabel(ylabel, color=INK2)
    ax.tick_params(colors=INK2, labelsize=9)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(MUTED)


def cost_figure(rows, families, base, floor, out):
    fig, axes = plt.subplots(1, 3, figsize=(16, 4.8), constrained_layout=True)
    panels = [("core_shift", "core_shift_err", "change in bias vs today's fit (Z)"),
              ("fail_d", "fail_d_err", "change in fraction off by ≥ 3 vs today's fit"),
              ("core_sigma", None, "spread of fits within 2 of true Z (σ, Z)")]
    for ax, (key, err, ylabel) in zip(axes, panels):
        paired = err is not None
        labeled = set()
        if paired and floor in rows and rows[floor][key] != "":
            f = rows[floor]
            ax.axhspan(-2 * f[err], 2 * f[err], color=GRID, alpha=0.7, lw=0)
            ax.text(0.99, 0.02, "±2 s.e. of today's fit with another seed", transform=ax.transAxes, ha="right", va="bottom",
                    fontsize=8, color=MUTED)
            ax.axhline(0, color=MUTED, lw=0.8)
        for i, (name, members) in enumerate(families.items()):
            pts = [(rows[s]["cpu_s_per_event"], rows[s][key], rows[s][err] if paired else 0.0, lab)
                   for s, lab in members if s in rows and rows[s][key] != "" and not (paired and s == base)]
            if paired:
                pts += [(rows[base]["cpu_s_per_event"], 0.0, 0.0, lab) for s, lab in members if s == base]
            pts.sort()
            if not pts:
                continue
            x, y, e, labs = zip(*pts)
            ax.errorbar(x, y, yerr=e, color=SERIES[i % 4], marker=MARKERS[i % 4], ms=7, lw=2 if len(pts) > 1 else 0,
                        elinewidth=1, capsize=0, label=name, mec="white", mew=1)
            for xi, yi, lab in zip(x, y, labs):
                if (xi, yi) in labeled:  # the base appears in several families: label it once
                    continue
                labeled.add((xi, yi))
                ax.annotate(lab, (xi, yi), xytext=(4, 5), textcoords="offset points", fontsize=8, color=INK2)
        if not paired:
            ax.axhline(rows[base][key], color=MUTED, lw=0.8, ls="--")
        ax.set_xscale("log")
        plain = matplotlib.ticker.FuncFormatter(lambda v, _: f"{v:g}")
        ax.xaxis.set_major_formatter(plain)
        sparse = matplotlib.ticker.FuncFormatter(
            lambda v, _: f"{v:g}" if round(v / 10 ** math.floor(math.log10(v))) in (2, 3, 5) else "")
        ax.xaxis.set_minor_formatter(sparse)
        style(ax, "CPU-s per fitted event (4 threads, log)", ylabel)
    axes[0].legend(frameon=False, fontsize=9, loc="best")
    fig.savefig(out, dpi=150, facecolor="#fcfcfb")
    plt.close(fig)


def bias_figure(zrows, families, base, out, xlabel):
    n = len(families)
    fig, axes = plt.subplots(1, n, figsize=(3.6 * n, 3.8), sharey=True, constrained_layout=True)
    axes = [axes] if n == 1 else axes
    for i, (ax, (name, members)) in enumerate(zip(axes, families.items())):
        ramp = LinearSegmentedColormap.from_list("r", ["#c9dcf3", SERIES[i % 4], "#0d2f57"])
        others = [(s, lab) for s, lab in members if s != base and s in zrows]
        if base in zrows:
            z, m, e = zip(*sorted(zrows[base]))
            ax.errorbar(z, m, yerr=e, color=MUTED, ls="--", lw=1.5, marker="o", ms=4, label="today's fit")
        for j, (s, lab) in enumerate(others):
            z, m, e = zip(*sorted(zrows[s]))
            c = ramp(0.35 + 0.65 * j / max(1, len(others) - 1))
            ax.errorbar(z, m, yerr=e, color=c, lw=2, marker="o", ms=4, elinewidth=0.8, label=lab)
        ax.axhline(0, color=MUTED, lw=0.8)
        ax.set_title(name, color=INK, fontsize=10)
        style(ax, xlabel, "bias: mean true − fitted Z, fits within 2" if i == 0 else "")
        ax.legend(frameon=False, fontsize=7, loc="best")
    fig.savefig(out, dpi=150, facecolor="#fcfcfb")
    plt.close(fig)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("csv")
    p.add_argument("z_csv")
    p.add_argument("--angle-csv", help="tune_report.py's --angle-csv: also write <prefix>_bias_angle.png")
    p.add_argument("prefix")
    p.add_argument("--base", default="base")
    p.add_argument("--floor", default="base2")
    p.add_argument("--family", action="append", metavar="NAME=sec:label,...")
    a = p.parse_args()
    families = FAMILIES
    if a.family:
        families = {}
        for spec in a.family:
            name, _, members = spec.partition("=")
            families[name] = [tuple(m.split(":", 1)) if ":" in m else (m, m) for m in members.split(",")]
    rows = read_rows(a.csv)
    cost_figure(rows, families, a.base, a.floor, a.prefix + "_cost.png")
    bias_figure(read_binned(a.z_csv, "zsim", int), families, a.base, a.prefix + "_bias_z.png", "true light-fragment Z")
    if a.angle_csv:
        bias_figure(read_binned(a.angle_csv, "angle_bin", angle_center), families, a.base, a.prefix + "_bias_angle.png",
                    "|90° − true θCM| (deg, bin centers)")


if __name__ == "__main__":
    main()
