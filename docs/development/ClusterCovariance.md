> **Status (2026-04-06): RESOLVED** — Diffusion coefficients and pad resolution are now wired from `AtDigiPar` into `AtSmooth3DClusterer` and `AtTrackTransformer` via `AtPRAtask::Init()`. The residual science question (per-cluster covariance 1.27% vs fixed sigma 1.04% RMS) is documented in the body below.

# Cluster Covariance Infrastructure

## Problem

`AtTrackTransformer::ClusterizeSmooth3D` computes per-cluster covariance matrices
using hardcoded diffusion coefficients that are 10–1000x too large compared to the
actual detector parameters in `AtDigiPar`:

| Parameter | Hardcoded | AtDigiPar (ATTPC.e20009_sim.par) | Factor |
|-----------|-----------|----------------------------------|--------|
| `d_t` (transverse diffusion) | 0.0009 cm²/µs | 0.00009 cm²/µs | 10x |
| `d_l` (longitudinal diffusion) | 0.0009 cm²/µs | 0.0000009 cm²/µs | 1000x |
| `driftVel` | 1.0 cm/µs | 1.0 cm/µs | OK |
| `samplingRate` | 0.320 µs | 3.0 (raw ADC units) | mismatch |

This inflates the cluster position uncertainties, making the per-cluster covariance
unreliable for the UKF. When `fUsePerClusterCov=true`, the UKF gets measurement
covariances that are too large, causing it to over-trust predictions and slightly
degrading momentum resolution (1.19% RMS vs 1.04% with fixed sigma).

## Current Implementation

`AtTrackTransformer::ClusterizeSmooth3D` (AtTools/AtTrackTransformer.cxx:42-48):

```cpp
// Diffusion coefficients (TODO: Get them from the parameter file)
Double_t driftVel = 1.0;       // cm/us
Double_t samplingRate = 0.320; // us
Double_t d_t = 0.0009;         // cm^2/us
Double_t d_l = 0.0009;         // cm^2/us
Double_t D_T = TMath::Sqrt((2.0 * d_t) / driftVel);
Double_t D_L = TMath::Sqrt((2.0 * d_l) / driftVel);
```

The covariance per cluster is then (lines 99-106):

```
sigma_x = charge_weighted sum of: sqrt(0.04 + z * D_T^2)   // 0.2 mm pad resolution + diffusion
sigma_y = sigma_x
sigma_z = average of: sqrt((1/6) * (driftVel*samplingRate)^2 + z * D_L^2)
```

## Plan

### Step 1: Pass AtDigiPar to AtTrackTransformer

Add a method to set diffusion parameters from AtDigiPar:

```cpp
void AtTrackTransformer::SetDiffusionParams(double coefT, double coefL,
                                             double driftVel, double tbTime);
```

Or pass `AtDigiPar*` directly. The parameters needed are:
- `CoefT` — transverse diffusion coefficient (cm²/µs)
- `CoefL` — longitudinal diffusion coefficient (cm²/µs)
- `DriftVelocity` — electron drift velocity (cm/µs)
- `TBTime` = 1000/SamplingRate — time bucket duration (ns → µs)

### Step 2: Propagate through AtPRA → AtPRAtask

`AtPRAtask` creates the `AtTrackTransformer` and calls `ClusterizeSmooth3D`.
It already has access to `AtDigiPar` via FairRuntimeDb. Wire it through:

```
AtPRAtask::Init()
  → fPar = (AtDigiPar*)rtdb->getContainer("AtDigiPar")
  → fPRA->fTrackTransformer->SetDiffusionParams(...)
```

### Step 3: Pad resolution from AtMap

The hardcoded 0.2 mm pad position resolution should ideally come from the pad
geometry (pad size / sqrt(12) for uniform distribution, or from a calibration).
For rectangular pads of size 8×12 mm:
- σ_x ≈ 8/√12 ≈ 2.3 mm
- σ_z ≈ 12/√12 ≈ 3.5 mm

The current 0.2 mm is far too small — this is another source of underestimated
covariance.

### Step 4: Validate with UKF

After fixing the covariance calculation:
1. Re-run per-cluster covariance UKF validation
2. Compare against fixed sigma baseline
3. Check if chi2/ndf improves toward 1

## File References

| File | Role |
|------|------|
| `AtTools/AtTrackTransformer.h/cxx` | ClusterizeSmooth3D with hardcoded params |
| `AtParameter/AtDigiPar.h` | Detector parameter container |
| `parameters/ATTPC.e20009_sim.par` | Example parameter file |
| `AtReconstruction/AtPRAtask.cxx` | Creates AtTrackTransformer |
| `AtReconstruction/AtPatternRecognition/AtPRA.h` | Owns fTrackTransformer |
| `AtReconstruction/AtFitter/AtFitterUKF.h` | fUsePerClusterCov flag |
