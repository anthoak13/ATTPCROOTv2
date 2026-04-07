# Unscented Kalman Filter (UKF) Development Guide

## Overview

The UKF implementation reconstructs charged particle tracks in the AT-TPC detector. It propagates a 6D state vector through magnetic/electric fields and corrects against measured hit positions using the Unscented Kalman Filter algorithm.

## Architecture

The implementation is layered in three tiers:

```
kf::UnscentedKalmanFilter<DIM_X, DIM_Z, DIM_V, DIM_N>   (generic UKF)
        |
kf::TrackFitterUKF                                        (physics-specific)
        |
EventFit::AtFitterUKF                                     (FairRoot integration)
```

### Layer 1: Generic UKF (`AtReconstruction/AtFitter/OpenKF/`)

Header-only, templated Kalman filter library with no physics dependencies.

| File | Description |
|------|-------------|
| `types.h` | Eigen type aliases (`Vector<N>`, `Matrix<R,C>`) |
| `kf_util.h` | Matrix utilities, Cholesky updates, linear solvers |
| `kalman_filter/kalman_filter.h` | Abstract base class for all KF variants |
| `kalman_filter/unscented_kalman_filter.h` | Full UKF with augmented state for process/measurement noise |
| `kalman_filter/unscented_transform.h` | Standalone sigma-point transform |
| `kalman_filter/square_root_ukf.h` | Numerically stable variant using Cholesky factors |

### Layer 2: Track Fitter (`AtReconstruction/AtFitter/OpenKF/kalman_filter/`)

| File | Description |
|------|-------------|
| `TrackFitterUKF.h` | Template base + concrete `TrackFitterUKF` class |
| `TrackFitterUKF.cxx` | Physics process/measurement models, RTS smoother |

**State vector (6D):** `[x, y, z, p_mag, theta, phi]`
**Measurement vector (3D):** `[x, y, z]` (hit position)
**Process noise (1D):** energy straggling
**Measurement noise (3D):** position uncertainty

Key methods:
- `funcF()` - Process model: propagates state through B/E fields via `AtPropagator`
- `funcH()` - Measurement model: projects state to measurement space
- `predictUKF()` - Prediction step to next measurement plane
- `correctUKF()` - Correction step with observed hit
- `smoothUKF()` - Rauch-Tung-Striebel backward smoother

### Layer 3: FairRoot Wrapper (`AtReconstruction/AtFitter/`)

| File | Description |
|------|-------------|
| `AtFitterUKF.h/cxx` | Bridges `kf::TrackFitterUKF` to the FairRoot task pipeline |

Configurable parameters:
- Magnetic/electric field vectors
- UKF sigma-point weights (alpha, beta, kappa)
- Position measurement sigma (default: 1.0 mm)
- Momentum uncertainty fraction (default: 0.1)
- Energy straggling toggle

## Dependencies

- **Eigen3** - Required. The entire OpenKF module is conditionally compiled when Eigen3 is found.
- **AtPropagator** - RK4 adaptive stepper for track propagation through fields.
- **AtELossModel** - Energy loss calculation (CATIMA backend).

## Tests

| Test file | What it covers |
|-----------|---------------|
| `OpenKF/kalman_filter/kalman_filter_test.cxx` | Base KF linear/extended |
| `OpenKF/kalman_filter/unscented_kalman_filter_test.cxx` | Generic UKF |
| `OpenKF/kalman_filter/unscented_trasform_test.cpp` | Sigma-point transform |
| `OpenKF/kalman_filter/square_root_ukf_test.cpp` | Square root variant |
| `OpenKF/kalman_filter/TrackFitterUKFTest.cxx` | Track-specific UKF |
| `AtFitterUKFTest.cxx` | FairRoot integration |

Run tests:
```bash
cd build && ctest -V -R UKF
# Or directly:
./build/tests/AtReconstructionTests
```

## Macros

Integration/demo macros in `macro/tests/UKF/`:
- `UKFTask.C` - Full task pipeline demo
- `UKFSingleTrack.C` - Single track fitting
- `SimulateManyTracks.C` / `TestManyTracks.C` - Batch validation

## Development Notes

### Resolved Issues (identified via `macro/tests/UKF/UKFDiagnostics.C`)

**Bug 1: Model noise Qmod applied uniformly across dimensions** — FIXED
- `SetInitialState` now sets `m_matQmod` per-dimension using `fPosModelNoise` (positions),
  `fMomModelNoise` (momentum), and `fAngModelNoise` (angles).
- Defaults: `fPosModelNoise=1e-4`, `fMomModelNoise=1e-2`, `fAngModelNoise=1e-4`.

**Bug 2: Sigma-point weight explosion with alpha=1e-3** — INVESTIGATED
- Default `fAlpha=1e-3` is KEPT. Testing showed that larger alpha values (>=0.1)
  cause sigma points to miss measurement planes in the highly nonlinear magnetic
  field propagation, leading to Cholesky failures.
- The weight explosion with small alpha is a theoretical concern but does not cause
  practical issues because sigma points stay close to the mean and the propagation
  model is well-behaved locally.
- For numerically safer alternative, use the square-root UKF variant.

**Bug 3: kReplaceFirstCov breaks RTS smoother consistency** — FIXED
- Default changed from `true` to `false`. Can still be toggled: `ukf.kReplaceFirstCov = true`.

**Bug 4: GetAugStateVector hardcodes 7 elements** — FIXED
- Now loops over `DIM_A` instead of hardcoding indices.

**Bug 5: Chi2/ndf doesn't subtract fitted parameters** — FIXED
- ndf changed to `3*(nClusters-1) - 6`, accounting for 3 measurements per cluster
  and 6 fitted state parameters.

**Bug 6 (ROOT CAUSE OF SMOOTHER DEGRADATION): Stopped sigma points set momentum to zero** — FIXED
- `AtPropagator.cxx` now preserves momentum direction with small magnitude (`fStopTol`)
  when a particle stops, instead of setting momentum to `(0,0,0)`.
- Falls back to `fLastMom` direction if current momentum is already zero.

### Validation with Digitized Data

End-to-end validation using fully digitized 16C(p,p) events, 20–90° CMS, 1000 events,
MC truth momentum seed at first cluster (`macro/Simulation/ATTPC/16C_pp/run_ukf_digi.C`).
_Last measured 2026-04-06 after PRA seam-1 refactor._

| Metric | Value |
|--------|-------|
| Convergence rate | 100% (480/480 proton tracks, 0 failures) |
| Momentum bias | +0.31% |
| Momentum resolution (RMS) | 1.09% |
| Theta bias | +0.008 deg |
| Mean residual | 0.96 mm |
| Avg clusters per track | 40 |

### History and Current Status of the Momentum Bias

The bias has evolved through several documented stages as fixes were applied:

| Commit | Bias | RMS | Note |
|--------|------|-----|------|
| `fdd4e279` | −1.6% | 1.0% | Original; r10d20 clustering |
| `709fd9ae` | −0.03% | 4.8% | Propagator back-extrapolation added |
| `b9053cbc` | −0.5% | 2.8% | r20d15 overlapping clustering default |
| `aaabc313` | −0.5% | 2.5% | Capped linear back-extrapolation (full pipeline) |
| current | +0.31% | 1.09% | r20d15 clustering + MC truth seed at first cluster |

Two sources were identified and addressed:

**Source 1 (~0.9%): Vertex-to-cluster offset** — RESOLVED
- The first digitized cluster is ~10 mm from the MC vertex due to pad granularity
  and the clusterization radius.
- When seeding the UKF with the vertex momentum but starting at the first cluster
  position, the momentum is already lower than the seed.
- Fix (run_ukf_digi.C): seed with MC truth momentum at the position of the first
  cluster. Fix (AtFitterUKF full pipeline): capped linear back-extrapolation from
  first cluster to beam axis, added in `aaabc313`.

**Source 2 (~0.9%): CATIMA vs GEANT4 energy loss mismatch** — RESIDUAL ~0.3%
- The UKF propagator uses CATIMA for energy loss, while the simulation uses GEANT4's
  built-in stopping power tables. They disagree by ~5% at low energies:

  | Energy | CATIMA dE/dx | GEANT4 avg dE/dx |
  |--------|-------------|------------------|
  | 0.5 MeV | 0.00410 MeV/mm | ~0.00391 MeV/mm |

- CATIMA predicts ~5% higher energy loss → the propagator removes too much energy
  per step → reconstructed momentum is systematically low.
- This is a fundamental model mismatch, not a bug. With the r20d15 clustering
  default and MC truth seeding the net residual bias is +0.31%, near zero.

**Mitigation options** (in order of preference):

1. **Energy loss scaling factor** — Use `TrackFitterUKF::fELossScaleFactor` to
   calibrate the CATIMA dE/dx against the actual energy loss. Scan results on
   16C(p,p) digitized data:

   | Scale | Mom bias | RMS |
   |-------|----------|-----|
   | 1.00 | -1.58% | 1.04% |
   | 1.02 | -1.28% | 1.01% |
   | 1.08 | -0.04% | 0.91% |

   For MC validation with GEANT4, `fELossScaleFactor ≈ 1.08` eliminates the bias.
   For real data, calibrate against known reactions or elastic scattering.
   Usage: `ukf.fELossScaleFactor = 1.08;` or via the macro:
   `root -b -q 'run_ukf_digi.C(-1, 1.08)'`.

2. **Use consistent energy loss tables** — Load the same SRIM tables in both
   simulation (via `AtELossTable`) and reconstruction. This eliminates the model
   mismatch entirely but requires maintaining external table files.

3. **Fit the scaling factor** — Add `fScalingFactor` as a 7th state variable in
   the UKF (or a second augmented noise dimension). The filter would then
   self-calibrate the energy loss model against the measured track curvature.
   This is the most robust approach but increases the augmented state dimension.

4. **Accept the bias** — A 1.6% systematic on momentum is within typical
   experimental uncertainties for AT-TPC measurements. Document and correct
   offline.
