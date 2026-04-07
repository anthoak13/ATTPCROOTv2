# 16C(p,p) UKF Macro Suite

Simulation and reconstruction macros for the `16C + p → 16C + p` elastic scattering reaction at 8 MeV/u in the AT-TPC. The full pipeline runs Geant4 Monte Carlo → digitization → PSA → pattern recognition → UKF track fitting → kinematic analysis.

All macros run from the `macro/Simulation/ATTPC/16C_pp/` directory after sourcing the environment:

```bash
source build/config.sh
cd macro/Simulation/ATTPC/16C_pp
```

---

## Pipeline Overview

```
C16_pp_sim.C          → data/attpcsim.root
run_digi_attpc.C      → data/output_digi.root   (digi + PSA + PR in one shot)
run_ukf_only.C        → data/output_ukf_only.root
```

Or, to iterate on pattern recognition parameters without re-digitizing:

```
C16_pp_sim.C          → data/attpcsim.root
run_digi_noPR.C       → data/output_psa.root
run_pr_only.C         → data/output_digi.root   (PR only, fast re-run)
run_ukf_only.C        → data/output_ukf_only.root
```

For a single-command full pipeline run:

```
C16_pp_sim.C          → data/attpcsim.root
run_reco_ukf.C        → data/output_reco_ukf.root
```

---

## Macros

### Simulation

#### `C16_pp_sim.C`
Generates Geant4 Monte Carlo events for the 16C(p,p) reaction. Fires a `16C` beam at 8 MeV/u into an H₂ target (300 torr) and simulates 2-body elastic scattering via `AtTPC2Body`. Outputs MC points and tracks to `data/attpcsim.root`.

```bash
root -b -q 'C16_pp_sim.C(1000)'   # simulate 1000 events
```

**Verify:** Output file exists and `cbmsim` tree has `MCTrack` and `AtTpcPoint` branches.

---

### Digitization

#### `run_digi_attpc.C`
Full digitization + PSA + pattern recognition chain in one macro. Runs `AtClusterizeTask → AtPulseTask → AtPSAtask → AtPRAtask` on the MC output. The main macro for producing `output_digi.root` for subsequent fitting steps.

```bash
root -b -q run_digi_attpc.C          # all events, tCluster=8 mm
root -b -q 'run_digi_attpc.C(500)'   # 500 events
```

**Verify:** `output_digi.root` has `AtRawEvent`, `AtEventH`, and `AtPatternEvent` branches.

#### `run_digi_noPR.C`
Digitization + PSA only — no pattern recognition. Produces `output_psa.root`. Use this when you want to iterate on PR parameters without re-running the slow digitization step.

```bash
root -b -q run_digi_noPR.C
```

**Verify:** `output_psa.root` has `AtRawEvent` and `AtEventH` branches.

#### `run_pr_only.C`
Runs `AtPRAtask` on the pre-digitized PSA output (`output_psa.root`) and writes `output_digi.root`. Allows fast scanning of the `tCluster` parameter without re-digitizing.

```bash
root -b -q 'run_pr_only.C(1000, 6.0)'  # 1000 events, tCluster=6 mm
```

**Verify:** `output_digi.root` is updated with `AtPatternEvent`; run `count_tracks.C` to check track counts.

---

### Fitting

#### `run_ukf_only.C`
Runs only the UKF fitter (`AtFitterTask`) on pre-digitized data (`output_digi.root`). Reads `AtPatternEvent` and writes `AtTrackingEvent` to `output_ukf_only.root`. The standard step after digitization for producing fitted kinematics.

```bash
root -b -q run_ukf_only.C
```

**Verify:** `output_ukf_only.root` has `AtTrackingEvent`; run `show_kinematics.C` or `analyze_kinematics.C` to inspect results.

#### `run_reco_ukf.C`
Single-macro full pipeline: digitization + PSA + PR + UKF fitting in one run, from `attpcsim.root` to `output_reco_ukf.root`. Convenient for batch production but slower to iterate on than the split workflow.

```bash
root -b -q run_reco_ukf.C
root -b -q 'run_reco_ukf.C(200)'
```

**Verify:** `output_reco_ukf.root` has all branches including `AtTrackingEvent`.

#### `run_digi_ukf.C`
Debug/development macro: reads an already-digitized file (`output_digi.root`) and runs only the UKF fitter task on 5 events. Used to test fitter configuration in isolation without touching the full chain.

```bash
root -b -q run_digi_ukf.C
```

**Verify:** Runs 5 events without crashing; prints timing.

---

### Analysis & Validation

#### `run_ukf_digi.C`
End-to-end UKF validation script. Reads MC truth and digitized data, seeds the UKF fitter with MC truth momentum, runs the filter/smoother on the proton track clusters, and compares reconstructed kinematics against truth. Reports momentum and angle errors with histograms saved to `data/ukf_digi_validation.png`.

Useful parameters: `maxEvents`, `eLossScale` (scale CATIMA dE/dx), `minSpacing` (cluster spacing filter in mm), `usePerClusterCov` (use cluster covariance matrix as measurement noise).

```bash
root -b -q run_ukf_digi.C                          # all events
root -b -q 'run_ukf_digi.C(50)'                    # 50 events
root -b -q 'run_ukf_digi.C(-1, 1.0, 2.0)'          # minSpacing=2 mm
root -b -q 'run_ukf_digi.C(-1, 1.0, 2.0, true)'    # per-cluster covariance
```

**Verify:** Prints per-event `p_true` vs `p_reco` and a summary table; saves `ukf_digi_validation.png`.

#### `analyze_kinematics.C`
Reads MC truth (`attpcsim.root`), pattern recognition (`output_digi.root`), and UKF fitted (`output_ukf_only.root`) output to produce kinematic comparison plots: MC truth, Brho seed, and UKF reconstructed θ_lab vs KE scatter plots, plus KE and θ error distributions. Saves three PNGs to `data/`.

```bash
root -b -q analyze_kinematics.C   # batch
root -l analyze_kinematics.C      # interactive
```

**Verify:** Prints event/fit/good counts and saves `kinematics_curves.png`, `kinematics_errors.png`, `kinematics_overlay.png`.

#### `show_kinematics.C`
Interactive kinematic display. Overlays MC truth and UKF-reconstructed θ_lab vs KE on a single canvas, shows KE and θ error histograms with Gaussian fits, draws a zoomed view of low-energy (< 5 MeV) events with lines connecting truth to reco for outliers, and displays the reconstructed vertex XY distribution.

```bash
root -l show_kinematics.C
```

**Verify:** Four canvases open; terminal prints count of proton/reconstructed events and any deviating low-energy events.

#### `analyze.C`
Quick diagnostic: opens `output_digi.root`, iterates over events, and dumps all hit positions, timestamps, and charges to `hits.txt`. Useful for inspecting raw hit content after digitization.

```bash
root -b -q analyze.C
```

**Verify:** `hits.txt` is written; terminal prints hit count per event.

#### `count_tracks.C`
Counts how many tracks the pattern recognition found per event (0, 1, 2, or >2) and prints the totals. Useful for quickly assessing PR efficiency after changing `tCluster`.

```bash
root -b -q count_tracks.C                           # default: output_digi.root
root -b -q 'count_tracks.C("data/my_file.root")'   # custom file
```

**Verify:** Prints `0trk=N 1trk=N 2trk=N >2trk=N`.

#### `test_clustering.C`
Documents the results of a cluster spacing parameter scan (`minSpacing` in `run_ukf_digi.C`). The macro body just prints usage instructions; the actual scan is run as a shell loop. Includes a results table showing bias and RMS vs. spacing for 16C(p,p).

```bash
# View documented results:
root -b -q test_clustering.C

# Run the scan yourself:
for s in 1.0 2.0 3.0 5.0 7.0 10.0 15.0; do
  echo "=== Spacing $s mm ==="
  root -b -q "run_ukf_digi.C(-1, 1.0, $s)" 2>&1 | grep -E "Fitted|Failed|Mom|Avg"
done
```

---

### Visualization

#### `display_ukf.C`
Standalone interactive track display. Reads `output_digi.root` and `output_ukf_only.root`, shows cluster hit projections (XY, XZ, YZ) overlaid with UKF smoothed track positions, plus a diagnostics canvas with residuals vs. cluster index, arc-length profile, and cluster charge profile. Navigation commands available at the prompt.

```bash
root -l display_ukf.C              # start at event 1
root -l 'display_ukf.C(5)'        # start at event 5
# In ROOT prompt:
# DrawEvent(10)   → go to event 10
# DrawEvent(-1)   → next event with a track
# DrawEvent(-2)   → previous event with a track
```

**Verify:** Two canvases open with cluster and fit overlaid; diagnostic canvas shows residuals and charge profile.

#### `run_ukf_display.C`
Thin wrapper that launches the `AtUKFDisplay` singleton, loading `output_digi.root`, `output_ukf_only.root`, and `attpcsim.root`. Delegates all display logic to `AtUKFDisplay`.

```bash
root -l run_ukf_display.C
root -l 'run_ukf_display.C(3)'    # start at event 3
```

**Verify:** Display opens at the specified event.

#### `eventDisplay.C`
FairRoot `FairEventManager`-based display for raw MC simulation output. Shows `AtTpcPoint` MC hits as blue squares in the detector geometry. Requires a working OpenGL/Eve environment.

```bash
root -l eventDisplay.C
```

**Verify:** Eve window opens with detector geometry and MC points visible.

#### `run_eve.C`
Launches the `AtEventManager` interactive event display on digitized data (`output_digi.root` by default). Shows `AtRawEvent` pad signals and `AtEventH` hits in the detector. Requires OpenGL.

```bash
root -l run_eve.C
root -l 'run_eve.C("output_digi.root", "out.root", "/Simulation/ATTPC/16C_pp/data/")'
```

**Verify:** Event manager window opens with hit display.

---

## Typical Workflow to Verify Everything Still Works

```bash
source build/config.sh
cd macro/Simulation/ATTPC/16C_pp

# 1. Simulate 200 events
root -b -q 'C16_pp_sim.C(200)'

# 2. Digitize + PR
root -b -q 'run_digi_attpc.C(200)'

# 3. Check track counts
root -b -q count_tracks.C

# 4. Run UKF fitter
root -b -q 'run_ukf_only.C(200)'

# 5. Check kinematic reconstruction
root -b -q analyze_kinematics.C

# 6. Validate UKF with MC truth seeding (use 1000 events for stable statistics)
root -b -q 'run_ukf_digi.C(1000)'
```

Expected results after step 3: roughly half the events with 1 track, half with 0 (beam-only or PR-rejected).
Expected results after step 6 (1000-event run, 16C(p,p) at 20–90° CMS, H₂ 300 torr, MC truth seed):

| Metric | Expected |
|---|---|
| Convergence | 100% (0 failures) |
| Momentum bias | ~+0.3% |
| Momentum RMS | ~1.1% |
| Theta error mean | < 0.05 deg |
| Mean cluster residual | ~1 mm |
| Avg clusters/track | ~40 |

_Last verified 2026-04-06, 1000 events, 480 proton tracks fitted, after PRA seam-1 refactor._
