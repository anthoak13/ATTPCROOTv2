> **Status (2026-04-06): PARTIAL** — Phase 1 (digitized proton tracks via `AtSimpleSimulation`) is complete. Systematic PSA method comparison and gas pressure scans are not yet done. The body below remains a valid forward plan.

# Synthetic Data Pipeline for UKF Validation

## Overview

The UKF track fitter currently validates against raw GEANT4 MC points (`hits.txt`)
that bypass the full detector simulation. This produces unrealistically precise hits
(sub-mm resolution) that don't reflect the actual measurement uncertainties from the
AT-TPC detector. To properly validate and tune the UKF, we need hits that pass through
the full simulation chain: ionization → drift → diffusion → pad response → PSA reconstruction.

## Current State

### What `hits.txt` contains
- 315 raw MC points from a ~1 MeV proton track in 300 torr H2
- Columns: x, y, z (cm), energy loss (MeV)
- Every 5th point is clustered for UKF input (63 hits)
- **No digitization effects**: perfect position resolution, no pad granularity,
  no diffusion, no electronics noise

## Simulation Pipeline

The full pipeline follows the reference implementation in
`macro/Simulation/ATTPC/16C_pp/`:

```
Step 1: C16_pp_sim.C          Step 2: run_digi_attpc.C
┌──────────────────┐     ┌──────────────────────────────────────────┐
│  FairRunSim       │     │  FairRunAna                              │
│  ├─ AtTpc (det)   │     │  ├─ AtClusterizeTask                    │
│  ├─ AtConstField  │     │  │   E_loss → N_electrons (Fano)        │
│  ├─ AtTPCIonGen   │     │  │   z → drift time                     │
│  └─ AtTPC2Body    │     │  │   Gaussian diffusion (transverse/    │
│                   │     │  │   longitudinal)                       │
│  Output:          │     │  ├─ AtPulseTask                         │
│  attpcsim.root    │     │  │   (x,y) → pad number (AtMap)         │
│  - AtMCPoint[]    │──→  │  │   Polya gain distribution             │
│  - AtMCTrack[]    │     │  │   Time binning + response convolution │
│                   │     │  │   Electronics noise                   │
└──────────────────┘     │  ├─ AtPSAtask (AtPSAMax)                │
                          │  │   Peak finding on pad traces          │
                          │  │   Hit position from pad + time bucket │
                          │  └─ AtPRAtask (pattern recognition)     │
                          │                                          │
                          │  Output: output_digi.root                │
                          │  - AtRawEvent (pad traces)               │
                          │  - AtEvent (AtHit[])                     │
                          │  - AtPatternEvent (AtTrack[])            │
                          └──────────────────────────────────────────┘
```

### Key Digitization Parameters (from `AtDigiPar`)

| Parameter | Description | Typical value |
|-----------|-------------|---------------|
| `fEIonize` | Effective ionization energy | ~37 eV (H2) |
| `fFano` | Fano factor | ~0.2 |
| `fVelDrift` | Electron drift velocity | 1–2 cm/µs |
| `fCoefT` | Transverse diffusion coefficient | cm²/µs |
| `fCoefL` | Longitudinal diffusion coefficient | cm²/µs |
| `fGain` | Micromegas/amplifier gain | 1000–5000 |
| `fGETGain` | GET electronics gain | fC |
| `fPeakingTime` | Electronics shaping time | 100–200 ns |
| `fTBTime` | Time bucket duration | 51 ns |
| `fZPadPlane` | Pad plane Z position | mm |

### What Digitization Adds

1. **Pad granularity**: Continuous (x,y) mapped to discrete pad positions (~10 mm pads)
2. **Diffusion**: Gaussian smearing grows with drift distance, broadens charge cloud
3. **Gain fluctuations**: Polya-distributed gain per electron cluster
4. **Electronics response**: Shaped pulse convolution, finite time resolution
5. **Noise**: Gaussian baseline noise on each pad trace
6. **Dead/inhibited pads**: Some pads have zero or reduced gain
7. **Time-to-Z conversion**: Z position reconstructed from time bucket, not true Z

## Plan

### Phase 1: Generate Digitized Proton Tracks

Adapt the `16C_pp` pipeline to generate proton tracks suitable for UKF validation:

1. **Simulation macro**: Generate proton tracks in H2 at 300 torr with B = 2.85 T
   - Range of energies (0.5–5 MeV) to test near and away from Bragg peak
   - Fixed and randomized angles
   - Store MC truth for comparison

2. **Digitization macro**: Run full chain (clusterize → pulse → PSA)
   - Use realistic AT-TPC parameters
   - Store both `AtEvent` (reconstructed hits) and MC truth

3. **Extraction script**: Convert `AtHit` objects to format usable by UKF tests
   - Position (x, y, z) in mm
   - Charge (for energy loss estimation)
   - MC truth position for residual calculation

### Phase 2: Characterize Hit Resolution

With digitized hits:

1. **Measure actual hit resolution**: Compare reconstructed vs MC truth positions
   - Expect σ ~ 1–3 mm depending on pad size, diffusion, PSA method
   - Resolution varies with drift distance (diffusion grows with √t)

2. **Resolution as function of**:
   - Drift distance (diffusion effect)
   - Track angle relative to pad plane
   - Energy / proximity to Bragg peak

### Phase 3: Systematic Studies

1. **PSA method comparison**: Test with hits from AtPSAMax vs AtPSAFull vs AtPSADeconv
2. **Gas pressure scan**: Different pressures change diffusion and energy loss
3. **Track angle dependence**: Resolution depends on track orientation vs pad plane
4. **Clustering strategy**: Optimize number of hits per cluster for track fitting

## File References

| File | Purpose |
|------|---------|
| `macro/Simulation/ATTPC/16C_pp/C16_pp_sim.C` | Reference GEANT4 simulation |
| `macro/Simulation/ATTPC/16C_pp/run_digi_attpc.C` | Reference digitization chain |
| `AtDigitization/AtClusterize.{h,cxx}` | MC point → electron clusters |
| `AtDigitization/AtPulse.{h,cxx}` | Electron clusters → pad signals |
| `AtReconstruction/AtPulseAnalyzer/AtPSA.h` | Abstract PSA base class |
| `AtReconstruction/AtPulseAnalyzer/AtPSAMax.cxx` | Simple peak-finding PSA |
| `AtParameter/AtDigiPar.h` | Digitization parameter container |
| `AtData/AtHit.h` | Reconstructed hit data class |
| `macro/tests/UKF/UKFTestHelpers.h` | Current UKF test infrastructure |
