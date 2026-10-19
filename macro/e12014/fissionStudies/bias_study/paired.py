#!/usr/bin/env python3
"""Paired comparison of two fits of the same events, from dump_landscape.C CSVs: mean change of the best-draw
dZ (and of the parabola-vertex dZ) over events that are in the core (|dZ| <= 2) of both.

    ./paired.py a.csv b.csv truth.csv
"""
import sys

import numpy as np

import landscape as L

a, b, t = sys.argv[1:4]
truth = L.read_truth(t)
ea, _ = L.load_events(a, truth)
eb, _ = L.load_events(b, truth)


def best(e):
    return e["zl"] - e["z"][np.argmin(e["obj"])] if len(e["obj"]) else None


def vert(e):
    zs, m, _, _ = L.profile(e, 3)
    v = L.vertex(zs, m) if len(zs) else None
    return None if v is None else e["zl"] - v


for name, f in (("best draw", best), ("parabola vertex", vert)):
    d = []
    for k in ea.keys() & eb.keys():
        x, y = f(ea[k]), f(eb[k])
        if x is not None and y is not None and abs(x) <= 2 and abs(y) <= 2:
            d.append(y - x)
    d = np.array(d)
    print(f"{name}: change {d.mean():+.3f} +- {d.std(ddof=1) / np.sqrt(len(d)):.3f} (n={len(d)})")
