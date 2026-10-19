#!/usr/bin/env python3
"""Figures of the bias study (see README.md), from landscape.py's --json output, vertex_check.C's CSVs and a
trace CSV (columns tb, q, adc_asis, adc_resp).

    ./make_figures.py --landscape base.json --bias-z name=label=file.json ... --vertex label=file.csv ...
                      --trace trace.csv --out figures
"""
import argparse
import csv
import json
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"]
MARKERS = ["o", "s", "^", "D"]
INK, INK2, MUTED, GRID = "#0b0b0b", "#52514e", "#8a8984", "#e4e3df"


def style(ax, xlabel, ylabel):
    ax.set_xlabel(xlabel, color=INK2)
    ax.set_ylabel(ylabel, color=INK2)
    ax.grid(True, color=GRID, lw=0.8)
    ax.set_axisbelow(True)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(MUTED)
    ax.tick_params(colors=INK2)


def landscape(path, out):
    d = json.load(open(path))
    rel = {int(k): v for k, v in d["rel"].items() if abs(int(k)) <= 4}
    ks = sorted(rel)
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.errorbar(ks, [rel[k]["mean"] for k in ks], yerr=[rel[k]["err"] for k in ks], color=SERIES[0], marker="o",
                ms=6, lw=2, capsize=0, label="mean objective, relative to the true Z")
    ax.plot(ks, [rel[k]["noise_sd"] for k in ks], color=SERIES[1], marker="s", ms=6, lw=2, ls="--",
            label="spread of single draws at that Z")
    style(ax, "fitted light Z - true light Z", "objective")
    ax.set_xticks(ks)
    ax.legend(frameon=False, labelcolor=INK2)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def bias_z(specs, out, keys=("best",), title=None):
    fig, ax = plt.subplots(figsize=(6.5, 4))
    i = 0
    for label, path, key in specs:
        d = json.load(open(path))["per_z"]
        zs = sorted(int(z) for z in d)
        off = (i - (len(specs) - 1) / 2) * 0.12
        ax.errorbar([z + off for z in zs], [d[str(z)][key][0] for z in zs], yerr=[d[str(z)][key][1] for z in zs],
                    color=SERIES[i % 4], marker=MARKERS[i % 4], ms=6, lw=2, capsize=0, label=label)
        i += 1
    ax.axhline(0, color=MUTED, lw=1)
    style(ax, "true light-fragment Z", "bias: true - fitted light Z")
    ax.legend(frameon=False, labelcolor=INK2)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def vertex(specs, out):
    fig, ax = plt.subplots(figsize=(6, 4))
    for i, (label, path) in enumerate(specs):
        d = [float(r["vz"]) - float(r["tvz"]) for r in csv.DictReader(open(path)) if r["vz"]]
        d = [x for x in d if abs(x) < 40]
        ax.hist(d, bins=80, range=(-40, 40), histtype="step", lw=2, color=SERIES[i], label=label)
    style(ax, "Y-pattern vertex - true vertex, along the beam (mm)", "events")
    ax.legend(frameon=False, labelcolor=INK2)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def trace(path, out):
    rows = list(csv.DictReader(open(path)))
    tb = [int(r["tb"]) for r in rows]
    fig, ax = plt.subplots(figsize=(6.5, 4))
    for i, (k, label) in enumerate((("q", "charge arriving on the pad"), ("adc_asis", "pulse, response per microsecond (as made)"),
                                    ("adc_resp", "pulse, response per time bucket"))):
        v = [float(r[k]) for r in rows]
        m = max(v)
        ax.plot(tb, [x / m for x in v], color=SERIES[i], lw=2, label=label)
    lo = min(t for t, r in zip(tb, rows) if float(r["q"]) > 0) - 10
    ax.set_xlim(lo, lo + 60)
    style(ax, "time bucket", "normalized to its maximum")
    ax.set_ylim(-0.05, 1.45)
    ax.legend(frameon=False, labelcolor=INK2, loc="upper right", fontsize=9)
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--landscape")
    ap.add_argument("--search", nargs="*", default=[], metavar="LABEL=JSON",
                    help="per-Z bias of the best draw and of the mean-objective minimum, from one JSON")
    ap.add_argument("--bias-z", nargs="*", default=[], metavar="LABEL=JSON", help="per-Z bias of the best draw")
    ap.add_argument("--bias-z-out", default="bias_by_z.png")
    ap.add_argument("--vertex", nargs="*", default=[], metavar="LABEL=CSV")
    ap.add_argument("--trace")
    ap.add_argument("--out", default="figures")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    if a.landscape:
        landscape(a.landscape, os.path.join(a.out, "landscape.png"))
    for s in a.search:
        label, path = s.split("=", 1)
        bias_z([("lowest single draw (the fit)", path, "best"), ("minimum of the mean objective", path, "mean_min")],
               os.path.join(a.out, "search_vs_model.png"))
    if a.bias_z:
        bias_z([(s.split("=", 1)[0], s.split("=", 1)[1], "best") for s in a.bias_z], os.path.join(a.out, a.bias_z_out))
    if a.vertex:
        vertex([tuple(s.split("=", 1)) for s in a.vertex], os.path.join(a.out, "vertex.png"))
    if a.trace:
        trace(a.trace, os.path.join(a.out, "trace.png"))


if __name__ == "__main__":
    main()
