#!/usr/bin/env python3
"""One line per fit (dump_landscape.C CSV): events, unfittable (no simulation gave a valid objective),
bias and spread of the lowest draw over |dZ| <= 2, fraction with dZ = 0, fraction off by 3 or more (of the
fittable events), and the parabola-minimum bias.

    ./summary.py truth.csv label=samples.csv ...
"""
import sys

import numpy as np

import landscape as L

truth = L.read_truth(sys.argv[1])
for spec in sys.argv[2:]:
    label, path = spec.split("=", 1)
    ev, dropped = L.load_events(path, truth)
    dead = sum(1 for e in ev.values() if len(e["obj"]) == 0)
    dz = np.array([e["zl"] - e["z"][np.argmin(e["obj"])] for e in ev.values() if len(e["obj"])])
    core = dz[np.abs(dz) <= 2]
    verts = []
    for e in ev.values():
        if len(e["obj"]):
            zs, m, _, _ = L.profile(e, 3)
            v = L.vertex(zs, m) if len(zs) else None
            if v is not None and abs(e["zl"] - v) <= 2.5:
                verts.append(e["zl"] - v)
    print(f"{label}: events {len(ev)} unfittable {dead} | bias {core.mean():+.3f} +- {core.std(ddof=1)/np.sqrt(len(core)):.3f}"
          f" spread {core.std(ddof=1):.2f} exact {np.mean(dz == 0):.3f} off>=3 {np.mean(np.abs(dz) >= 3):.3f}"
          f" | parabola {np.mean(verts):+.3f} +- {np.std(verts, ddof=1)/np.sqrt(len(verts)):.3f}")
