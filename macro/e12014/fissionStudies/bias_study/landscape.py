#!/usr/bin/env python3
"""Objective landscape of the MC fission fit, per event, from dump_landscape.C's CSV of every sampled point.

The fit fixes the fragment directions and the vertex to the Y-pattern and samples only the Z of fragment 0,
so the sampled objectives of one event at one Z are independent draws of the objective's noise there. For
each event this gives the mean objective m(Z) at every sampled Z, and so separates two things:
  * where the mean objective has its minimum (a property of the model, independent of the search), and
  * which Z the fit picks: the lowest single draw, which also depends on the noise and on how often each Z
    was sampled.

    ./landscape.py samples.csv truth.csv [--min-n 3] [--json out.json]

All Z here are of the true light fragment: fitted fragments are matched to true ones by azimuth (as
tune_report.py --unfolded does), and dZ = true light Z - fitted Z of that same fragment.
"""
import argparse
import collections
import csv
import json
import math
import sys

import numpy as np

ZCN = 85
AMBIGUOUS = 0.5


def gap(a, b):
    return abs((a - b + math.pi) % (2 * math.pi) - math.pi)


def read_truth(path):
    with open(path) as f:
        return {(r["chunk"], int(r["entry"])): (int(float(r["z0"])), float(r["phi0"]), float(r["phi1"]))
                for r in csv.DictReader(f)}


def load_events(samples, truth):
    """{(chunk, entry): dict(zl=true light Z, z=array of fitted light Z per sample, obj=array)}"""
    raw = collections.defaultdict(list)
    meta = {}
    with open(samples) as f:
        for r in csv.DictReader(f):
            key = (r["chunk"], int(r["entry"]))
            raw[key].append((int(float(r["z0"])), float(r["obj"]), float(r["amp"])))
            meta[key] = (float(r["ph0"]), float(r["ph1"]), float(r["th0"]), float(r["th1"]), float(r["vz"]))
    events, dropped = {}, 0
    for key, rows in raw.items():
        z0t, phi0, phi1 = truth[key]
        ph0, ph1, th0, th1, vz = meta[key]
        direct = gap(phi0, ph0) + gap(phi1, ph1)
        swapped = gap(phi0, ph1) + gap(phi1, ph0)
        if abs(direct - swapped) < AMBIGUOUS:
            dropped += 1
            continue
        # Pattern fragment 0 is true fragment 0 (the heavy one, sim index 0) if the direct pairing wins
        p0_is_true0 = direct < swapped
        light_true0 = z0t < ZCN - z0t
        zl = min(z0t, ZCN - z0t)
        # Pattern fragment 0 is the true light fragment?
        p0_light = p0_is_true0 == light_true0
        z = np.array([r[0] if p0_light else ZCN - r[0] for r in rows])
        obj = np.array([r[1] for r in rows])
        amp = np.array([r[2] for r in rows])
        ok = obj < 1e30
        events[key] = dict(zl=zl, z=z[ok], obj=obj[ok], amp=amp[ok], p0_light=p0_light,
                           th_light=th0 if p0_light else th1, th_heavy=th1 if p0_light else th0, vz=vz)
    return events, dropped


def profile(ev, min_n):
    """Mean objective per sampled Z with at least min_n draws: (zs, mean, sd, n)."""
    zs, means, sds, ns = [], [], [], []
    for zv in np.unique(ev["z"]):
        o = ev["obj"][ev["z"] == zv]
        if len(o) >= min_n:
            zs.append(zv)
            means.append(o.mean())
            sds.append(o.std(ddof=1))
            ns.append(len(o))
    return np.array(zs), np.array(means), np.array(sds), np.array(ns)


def vertex(zs, means):
    """Minimum of the parabola through the lowest mean and its two neighbours (in Z), or None."""
    i = int(np.argmin(means))
    if i == 0 or i == len(zs) - 1 or zs[i] - zs[i - 1] != 1 or zs[i + 1] - zs[i] != 1:
        return None
    a, b, c = means[i - 1], means[i], means[i + 1]
    den = a - 2 * b + c
    if den <= 0:
        return None
    return zs[i] + 0.5 * (a - c) / den


def mean_err(x):
    x = np.asarray(x, float)
    return (x.mean(), x.std(ddof=1) / math.sqrt(len(x))) if len(x) > 1 else (math.nan, math.nan)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("samples")
    ap.add_argument("truth")
    ap.add_argument("--min-n", type=int, default=3)
    ap.add_argument("--json")
    a = ap.parse_args()
    events, dropped = load_events(a.samples, read_truth(a.truth))
    out = {"events": len(events), "dropped": dropped}

    best_dz, mean_dz, vert_dz, zl_all = [], [], [], []
    per_z = collections.defaultdict(lambda: collections.defaultdict(list))
    rel = collections.defaultdict(list)  # mean objective relative to the true Z's, by offset
    noise = collections.defaultdict(list)
    counts = collections.defaultdict(list)
    for key, ev in events.items():
        zl = ev["zl"]
        if len(ev["obj"]) == 0:
            continue
        ib = int(np.argmin(ev["obj"]))
        db = zl - ev["z"][ib]
        zs, m, sd, n = profile(ev, a.min_n)
        if len(zs) == 0:
            continue
        dm = zl - zs[int(np.argmin(m))]
        v = vertex(zs, m)
        best_dz.append(db)
        mean_dz.append(dm)
        zl_all.append(zl)
        per_z[zl]["best"].append(db)
        per_z[zl]["mean"].append(dm)
        if v is not None and abs(zl - v) <= 2.5:
            vert_dz.append(zl - v)
            per_z[zl]["vert"].append(zl - v)
        if zl in zs:
            m0 = m[list(zs).index(zl)]
            for zv, mv, sv, nv in zip(zs, m, sd, n):
                rel[int(zv - zl)].append(mv - m0)
                noise[int(zv - zl)].append(sv)
        for zv in np.unique(ev["z"]):
            counts[int(zv - zl)].append(int((ev["z"] == zv).sum()))

    def core(x):
        x = np.asarray(x)
        c = x[np.abs(x) <= 2]
        return mean_err(c) + (len(c) / len(x),)

    for name, x in (("best", best_dz), ("mean_min", mean_dz), ("parabola", vert_dz)):
        m, e, f = core(x)
        out[name] = {"core_bias": m, "err": e, "core_frac": f, "n": len(x)}
        print(f"{name:9s} core bias {m:+.3f} +- {e:.3f}  (core fraction {f:.3f}, n={len(x)})")
    print("\nper true light Z: bias of the best draw / of the mean-objective minimum / of the parabola vertex")
    out["per_z"] = {}
    for zl in sorted(per_z):
        b = core(per_z[zl]["best"])
        mm = core(per_z[zl]["mean"])
        vv = mean_err(per_z[zl]["vert"]) if per_z[zl]["vert"] else (math.nan, math.nan)
        out["per_z"][int(zl)] = {"best": b[:2], "mean_min": mm[:2], "parabola": vv}
        print(f"Z={zl}: {b[0]:+.2f}+-{b[1]:.2f}  {mm[0]:+.2f}+-{mm[1]:.2f}  {vv[0]:+.2f}+-{vv[1]:.2f}")
    print("\nmean objective minus that at the true Z, by fitted - true light Z (events with the true Z sampled)")
    out["rel"] = {}
    for d in sorted(rel):
        if len(rel[d]) > 20:
            m, e = mean_err(rel[d])
            med = float(np.median(rel[d]))
            ns = float(np.mean(noise[d]))
            out["rel"][d] = {"mean": m, "err": e, "median": med, "noise_sd": ns, "n": len(rel[d])}
            print(f"{d:+d}: {m:+.3f} +- {e:.3f}  median {med:+.3f}  noise sd {ns:.3f}  n={len(rel[d])}")
    print("\nmean draws per event at fitted - true light Z")
    out["counts"] = {}
    for d in sorted(counts):
        if len(counts[d]) > 20:
            out["counts"][d] = float(np.mean(counts[d]))
            print(f"{d:+d}: {np.mean(counts[d]):.1f} (events {len(counts[d])})")
    if a.json:
        with open(a.json, "w") as f:
            json.dump(out, f, indent=1, default=float)


if __name__ == "__main__":
    main()
