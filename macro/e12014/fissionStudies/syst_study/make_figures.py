#!/usr/bin/env python3
"""Figures of the systematics study from dump_fit.C's per-event CSVs (syst_report.py --dump-dir DIR).

    ./make_figures.py DIR figures [--changes changes.csv]

Writes <out>/angle.png: the fitted light-fragment Z against the true decay angle, from the baseline's
recommended fit (rec, 1 deg bins within 10 deg of 90) and the 10-30 deg sample (wide_lo, wide_hi, 2 deg
bins). Top: mean dZ with its standard error and the straight line fitted within 8 deg of 90. Bottom: the
fractions with the correct Z, within 1 and off by 3 or more. With --changes (make_tables.sh's changes.csv):
<out>/shifts.png, the change of the mean dZ within 2 and of the mean dZ of all fits for each variation of the study,
grouped by input. Run from fissionStudies/syst_study/.
"""
import argparse
import csv
import math
import os
import statistics
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tune_report as tr  # noqa: E402

SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"]  # Reference categorical palette, fixed order
INK, INK2, MUTED, GRID = "#0b0b0b", "#52514e", "#8a8984", "#e4e3df"
ZCN = 85


def load(dump, truth_path):
    """Used events (fitted, matched, fittable) of one fit: (theta_CM - 90, unfolded dZ)."""
    truth = tr.read_truth(truth_path)
    num = lambda r, k, f=float: f(r[k]) if r[k] else None
    with open(dump) as f:
        events = [{"file": r["file"], "entry": r["entry"], "fitted": r["fitted"] == "1", "obj": num(r, "obj"),
                   "z0fit": num(r, "z0fit", int), "ph0": num(r, "ph0"), "ph1": num(r, "ph1"),
                   "thcm": float(r["thcm"])} for r in csv.DictReader(f)]
    tr.unfold(events, truth, ZCN, False)
    return [(e["thcm"] - 90, e["dzu"]) for e in events
            if e["fitted"] and e["dzu"] is not None and not tr.unfittable(e)]


def bins(events, edges):
    """[(center, n, mean dZ, its error, correct, within 1, off by >= 3)] per bin of theta_CM - 90."""
    out = []
    for lo, hi in zip(edges, edges[1:]):
        d = [dz for x, dz in events if lo <= x < hi]
        if len(d) < 2:
            continue
        n = len(d)
        out.append(((lo + hi) / 2, n, statistics.fmean(d), statistics.stdev(d) / math.sqrt(n),
                    sum(x == 0 for x in d) / n, sum(abs(x) <= 1 for x in d) / n, sum(abs(x) >= 3 for x in d) / n))
    return out


def slope(events, limit):
    """Least-squares line dZ = a + b (theta - 90) over |theta - 90| < limit: (b, its error, a)."""
    pts = [(x, y) for x, y in events if abs(x) < limit]
    mx, my = statistics.fmean(p[0] for p in pts), statistics.fmean(p[1] for p in pts)
    sxx = sum((x - mx) ** 2 for x, _ in pts)
    b = sum((x - mx) * (y - my) for x, y in pts) / sxx
    s2 = sum((y - my - b * (x - mx)) ** 2 for x, y in pts) / (len(pts) - 2)
    return b, math.sqrt(s2 / sxx), my - b * mx


def style(ax, ylabel):
    ax.set_ylabel(ylabel, color=INK2)
    ax.tick_params(colors=INK2, labelsize=9)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(MUTED)


# (group, label, base, sample) of every variation in the shifts figure, top to bottom
SHIFTS = [
    ("Stopping power", "SRIM data, LISE fit", "rec", "srim_lise"),
    ("Stopping power", "LISE data, SRIM fit", "rec", "lise_srim"),
    ("Stopping power", "Ziegler data, LISE fit", "rec", "zieg_lise"),
    ("Stopping power", "LISE data, Ziegler fit", "rec", "lise_zieg"),
    ("Stopping power", "LISE dE/dx × 0.95", "rec", "s095"),
    ("Stopping power", "LISE dE/dx × 1.05", "rec", "s105"),
    ("Space charge", "λ / 2 in the data", "nom", "half"),
    ("Space charge", "2λ in the data", "nom", "double"),
    ("Space charge", "charge constant along the beam", "nom", "constant"),
    ("Space charge", "charge spread over the beam radius", "nom", "uniform"),
    ("Space charge", "beam line 5 mm off in y", "nom", "y5"),
    ("Space charge", "no correction", "rec", "nocorr"),
    ("Gas", "drift velocity × 0.98", "rec", "vdm2"),
    ("Gas", "drift velocity × 1.02", "rec", "vdp2"),
    ("Gas", "transverse diffusion × 0.8", "rec", "tm20"),
    ("Gas", "transverse diffusion × 1.2", "rec", "tp20"),
    ("Gas", "longitudinal diffusion × 1.2", "rec", "lp20"),
    ("Kinematics", "TKE × 0.98", "rec", "tke098"),
    ("Kinematics", "TKE × 1.02", "rec", "tke102"),
    ("Kinematics", "light fragment's A + 1", "rec", "ap1"),
    ("Kinematics", "beam spot × 2", "rec", "spot2"),
    ("Pads and electronics", "dead AsAd (CoBo 2, AsAd 3)", "rec", "asad"),
    ("Pads and electronics", "dead AsAd + 10% of pads in patches", "rec", "asad_p10"),
    ("Pads and electronics", "gain per AsAd, σ = 10%", "rec", "asad10"),
    ("Pads and electronics", "gain per pad, σ = 10%", "rec", "pad10"),
    ("Pads and electronics", "noise, σ = 5 ADC", "rec", "noise5"),
    ("Pads and electronics", "noise, σ = 10 ADC", "rec", "noise10"),
]


def shifts(changes, out):
    """Horizontal dot plot of the whole-sample change of both means for every variation in SHIFTS."""
    rows = {}
    with open(changes) as f:
        for r in csv.DictReader(f):
            if r["binning"] == "all":
                rows[(r["base"], r["sample"])] = r
    items = [(g, label, rows[(b, smp)]) for g, label, b, smp in SHIFTS if (b, smp) in rows]
    fig, ax = plt.subplots(figsize=(7.5, 0.27 * len(items) + 1.4))
    ys = list(range(len(items)))[::-1]
    for key, color, mfc, dy, label in (("bias", SERIES[0], SERIES[0], 0.14, "mean dZ within 2"),
                                       ("meanall", SERIES[1], "white", -0.14, "mean dZ of all fits")):
        ax.errorbar([float(r[key]) for _, _, r in items], [y + dy for y in ys],
                    xerr=[float(r[key + "_err"]) for _, _, r in items], fmt="o", color=color, mfc=mfc,
                    markersize=4.5, elinewidth=1, capsize=0, label=label)
    ax.axvline(0, color=INK2, linewidth=0.8)
    ax.set_yticks(ys)
    ax.set_yticklabels([label for _, label, _ in items], fontsize=8.5, color=INK2)
    group_prev = None
    for y, (g, _, _) in zip(ys, items):
        if g != group_prev:
            ax.text(0.01, y + 0.42, g, transform=ax.get_yaxis_transform(), ha="left", va="bottom", fontsize=8,
                    color=MUTED, fontweight="bold")
            if group_prev is not None:
                ax.axhline(y + 0.5, color=GRID, linewidth=0.8)
            group_prev = g
    ax.set_ylim(-0.7, len(items) - 0.05)
    style(ax, "")
    ax.grid(True, axis="x", color=GRID, linewidth=0.8)
    ax.grid(False, axis="y")
    ax.set_xlabel("change of dZ against the sample the variation is compared with (Z)\n"
                  "positive: the fit gives the light fragment a lower Z", color=INK2, fontsize=9)
    ax.legend(frameon=False, fontsize=8.5, loc="lower left", bbox_to_anchor=(0, 1.0), ncol=2, labelcolor=INK2)
    fig.suptitle("Shift of the fitted light-fragment Z for each variation", color=INK, fontsize=10.5, x=0.02,
                 ha="left")
    fig.tight_layout()
    fig.savefig(os.path.join(out, "shifts.png"), dpi=150)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dumps", help="syst_report.py's --dump-dir")
    parser.add_argument("out", help="directory for the figures")
    parser.add_argument("--changes", help="make_tables.sh's changes.csv: also draw shifts.png")
    args = parser.parse_args()
    os.makedirs(args.out, exist_ok=True)
    if args.changes:
        shifts(args.changes, args.out)

    dump = lambda name: os.path.join(args.dumps, f"hpc-syst-syst_base-{name}.csv")
    base = load(dump("rec"), "truth_syst_base-base.csv")
    wide = load(dump("wide_lo"), "truth_syst_base-wide_lo.csv") + load(dump("wide_hi"), "truth_syst_base-wide_hi.csv")
    near = bins(base, list(range(-10, 11)))
    far_lo, far_hi = bins(wide, list(range(-30, -9, 2))), bins(wide, list(range(10, 31, 2)))
    far = far_lo + far_hi
    b, b_err, a = slope(base, 8)
    print(f"slope within 8 deg: {b:+.4f} +- {b_err:.4f} Z/deg, intercept {a:+.3f}")

    fig, (top, bottom) = plt.subplots(2, 1, figsize=(7.5, 6.2), sharex=True, gridspec_kw={"height_ratios": [1.2, 1]})
    for data, marker, label in ((near, "o", "±10° sample, 1° bins"), (far, "s", "10–30° sample, 2° bins")):
        top.errorbar([c[0] for c in data], [c[2] for c in data], yerr=[c[3] for c in data], fmt=marker,
                     color=SERIES[0], markersize=4.5, elinewidth=1, capsize=0, label=label,
                     mfc=SERIES[0] if marker == "o" else "white")
    xs = [-30, 30]
    top.plot(xs, [a + b * x for x in xs], color=MUTED, linewidth=1, linestyle="--", zorder=0,
             label=f"line fitted within 8°: {b:+.3f} Z per degree")
    top.axhline(0, color=INK2, linewidth=0.8)
    style(top, "mean dZ (true − fitted Z)")
    top.legend(frameon=False, fontsize=8.5, loc="upper left", labelcolor=INK2)
    top.set_title("Fitted light-fragment Z against the true decay angle (recommended fit)", color=INK, fontsize=10.5,
                  loc="left")

    for k, (idx, label) in enumerate(((4, "correct Z"), (5, "within 1"), (6, "off by 3 or more"))):
        for data, marker in ((near, "o"), (far_lo, "s"), (far_hi, "s")):
            bottom.plot([c[0] for c in data], [100 * c[idx] for c in data], marker=marker, linestyle="-",
                        color=SERIES[k], markersize=4, linewidth=1.5, label=label if marker == "o" else None,
                        mfc=SERIES[k] if marker == "o" else "white")
    style(bottom, "events (%)")
    bottom.set_xlabel("true decay angle in the CoM frame − 90° (deg); below 0 the heavy fragment goes forward",
                      color=INK2)
    bottom.set_ylim(-3, 103)
    bottom.legend(frameon=False, fontsize=8.5, loc="center right", labelcolor=INK2)
    fig.tight_layout()
    fig.savefig(os.path.join(args.out, "angle.png"), dpi=150)


if __name__ == "__main__":
    main()
