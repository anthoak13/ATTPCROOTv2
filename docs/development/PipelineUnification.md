> **Status (2026-04-06): PARTIAL** — Clustering defaults were unified (r20d15, ~98% convergence) and documented in `AtSmooth3DClusterer` header defaults. Full macro consolidation and centralized settings documentation are still incomplete.

# UKF Pipeline Unification

## Goal

Consolidate all findings from UKF development into a clean, unified pipeline.
The current code has optimizations scattered across `run_ukf_digi.C`,
`AtFitterUKF`, `AtUKFDisplay`, and compiled tests — each with slightly
different logic. This document defines the single canonical implementation.

## Validated Settings

| Parameter | Value | Source |
|-----------|-------|--------|
| Clustering radius | 10 mm | Clustering scan: 98% convergence |
| Clustering distance | 20 mm | Clustering scan: 98% convergence |
| Min cluster spacing | 3 mm | AtFitterUKF filtering |
| Bragg peak trim | Last 10% of clusters | Prevents stopped-particle failures |
| Cluster ordering | Nearest-neighbor from vertex end | Critical: 83% vs 31% without |
| Vertex identification | Last PRA cluster (highest Z_digi) | Validated on 16C(p,p) |
| Z conversion | Z_lab = ZPadPlane - Z_digi | AT-TPC coordinate convention |
| Alpha | 1e-3 | Larger values cause sigma point divergence |
| Straggling | OFF | Negligible effect, saves computation |
| Measurement sigma | 2.0 mm (fixed) | Per-cluster slightly better (0.92% vs 1.04% RMS) |
| Momentum sigma frac | 0.3 (30%) | Initial covariance on momentum |
| E-loss scale | 1.0 (1.08 for MC bias correction) | CATIMA/GEANT4 mismatch |
| Max prop iterations | 500 | With clip-to-surface loop limit of 20 |
| Max fit time | 2000 ms per track | Timeout for stuck fits |
| fMaxPropIter | 500 | Safety guard |
| Clip-to-surface limit | 20 iterations | Fixes infinite loop bug |

## Current State (scattered)

```
run_digi_attpc.C          → output_digi.root
  AtClusterizeTask → AtPulseTask → AtPSAtask → AtPRAtask
  (clustering: r15 d30.5 hardcoded in AtTrackFinderHC)

run_ukf_only.C            → output_ukf_only.root
  AtFitterTask(AtFitterUKF)
  (ordering + filtering done inside AtFitterUKF::GetFittedTrack)

run_ukf_digi.C            → standalone validation
  (has its OWN ordering + filtering logic, separate from AtFitterUKF)

run_ukf_display.C         → interactive GUI
  (re-clusters with user params, calls AtFitterUKF internally)
```

## Target State (unified)

### 1. Default clustering parameters

Change `AtTrackFinderHC` default from `r15 d30.5` to `r10 d20`:

```cpp
// AtTrackFinderHC.cxx line 157
ClusterizeSmooth3D(track, 10.0, 20.0); // was 15.0, 30.5
```

This ensures the RANSAC circle fit sees better clusters → better Brho seed.

### 2. Single ordering + filtering in AtFitterUKF

`AtFitterUKF::GetFittedTrack` already implements:
- Z conversion (SetZPadPlane)
- Nearest-neighbor walk from vertex end
- Minimum spacing filter (3mm)
- Bragg peak trim (last 10%)
- Momentum direction from first two ordered clusters

This is the canonical implementation. Remove duplicate logic from `run_ukf_digi.C`.

### 3. Clean macro set

| Macro | Purpose |
|-------|---------|
| `C16_pp_sim.C` | GEANT4 simulation (no changes) |
| `run_digi_attpc.C` | Digi + PSA + PR with r10d20 clustering |
| `run_ukf_only.C` | UKF fitting via AtFitterTask (canonical) |
| `run_ukf_display.C` | Interactive GUI |
| `run_ukf_digi.C` | Keep for MC truth validation only |

### 4. Clustering interface

The clustering method is pluggable. Current: `ClusterizeSmooth3D`.
A new algorithm can be swapped by:
- Implementing a new method in `AtTrackTransformer`
- Calling it from `AtTrackFinderHC/TC` instead of `ClusterizeSmooth3D`
- The rest of the pipeline (ordering, filtering, UKF) stays the same

### 5. Brho seed

After unification with r10d20 clustering, re-evaluate:
- Does the RANSAC circle fit give a better radius with better clusters?
- Compare Brho-seeded p vs MC truth p across events
- If still >10% off, consider alternative seeding (energy loss, track length)

## Implementation Steps

1. Change `AtTrackFinderHC` default clustering to r10 d20
2. Regenerate digi data with new defaults
3. Run `run_ukf_only.C` and measure convergence
4. Update `run_ukf_only.C` defaults to match validated settings
5. Clean up `run_ukf_digi.C` (remove duplicate ordering logic)
6. Run full validation: convergence, momentum bias, residuals
7. Commit unified pipeline
