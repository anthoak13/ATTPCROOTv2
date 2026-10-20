# Systematic Uncertainties of the Fitted Fragment Z: Plan

**Goal.** Estimate the systematic uncertainties on the fragment Z fitted to the experimental e12014 data. The fit relies on a model of the detector and the reaction. For each input to that model, the study asks three questions: how far the fitted Z moves when the real value differs from the one the fit assumes, how large that difference plausibly is, and how the shift depends on the true Z and the decay angle.

**Why now.** Skyler studied several of these effects, but only on small samples: one Z per run and a few hundred events (see [What was done before](#what-was-done-before)). At the time, the fit cost about 530 CPU-s per event. The fit speed and fit tuning studies ([README](../tune_study/README.md)) cut that to about 10 CPU-s per event without a meaningful loss of accuracy. One variation on 3000 events now costs about 15 CPU-hours.

**What is not known.** The fit assumes the fragments decay at 90° in the center-of-mass frame, and the tuning study found it accurate only within a few degrees of that. We do not know the decay angles of the events in the experimental data. The analysis of the experimental data will keep events near 90° with a cut on the folding angle versus depth, but that cut has not been defined yet. Nothing in the code stands in for it: the cuts in `eventSim.h` and `plot_fit.C` are only rough approximations of physical effects. The study therefore treats the decay angle like the true Z and reports every result as a function of it. Once the selection is defined, the results are averaged over the angles it keeps.

## Method

### Change one input, fit with the nominal model

For each input to the model, such as a stopping-power table, the drift velocity or a pad's gain:

1. Simulate data (pseudo-data) with that input changed from its nominal value by a chosen amount δ and everything else nominal.
2. Fit the pseudo-data with the nominal model at the recommended settings. These are the faster settings chosen by the tuning study: a 2 mm step, a fixed pattern of 100 sampling points, longitudinal diffusion integrated over ±2σ, no gain fluctuations, and 3 rounds of 40 iterations.
3. Compare the fit event by event with the fit of the nominal data, using `tune_report.py`.

The shift in the fitted Z is the systematic uncertainty caused by an error of δ in that input. It is reported as a function of the true light-fragment Z and the decay angle. Some inputs can also be changed in the fit. For those, a second fit with the input changed by δ in both the data and the fit separates the effect of the mismatch from the effect of the changed model itself.

**Pads the fit uses.** The fit compares charge only on good pads: pads with a hit in the experimental event that are small inner pads, not inhibited (bad or low-gain), and not saturated in that event (hit charge above 4200). The simulated charge is summed over the same pads (`E12014::FillHitSums`). Every effect that removes or saturates pads therefore changes which parts of each track the fit sees.

**Pairing.** Each variation uses the same random seeds as the baseline, so it simulates the same events. The simulation draws random numbers only for the vertex depth, the Z split and the decay angle. The beam and fragment tracks are deterministic. Changing an input therefore changes the tracks and the hits, but not the events. The driver gives each section its own seed by default, so `sim.seed` and `digi.seed` must be set explicitly to the same values in both sections.

The hits themselves are reproduced only in some cases. A gain scale, the electronics response or a threshold leaves every random draw unchanged. The hits then differ only by the effect being studied, and the change in bias is measured to about 0.003 Z, as in the tuning study. A change to the simulation alters the number of charge lines, and noise adds draws of its own. From the first changed draw on, the hits are independent. At 3000 events the change in bias is then measured to about 0.005 Z, and the change in the fraction with the correct Z to about 0.5 percentage points (pp). Diffusion, drift velocity and dead pads lie between these two cases. Keeping the same events still helps when the hits are independent. With about 50 events per cell, a different set of angles and depths would add its own scatter to the difference.

### What each variation reports

- The change in bias, as a function of true Z and decay angle.
- The change in the fraction of events with the correct Z, and in the fraction off by 3 or more, as a function of true Z and decay angle.
- The change in the folding angle from the Y-pattern. The fit infers the beam velocity, and from it the fragment velocities, from the folding angle, and the stopping power depends strongly on velocity. Drift velocity, space charge and Y-pattern errors act on Z mostly through this path.
- For inputs with a continuous value, such as a stopping-power scale or the drift velocity, the change in bias per unit change of the input. This comes from runs at two or more values. It lets the result be scaled once the input's uncertainty is known.

### The baseline

The baseline is 3000 events simulated with every input at its nominal value. Each variation changes one input, simulates the same 3000 events again, and compares its fitted Z with the baseline's.

- **Fragment Z.** Each event has a single light fragment whose Z is chosen uniformly between 31 and 42 (`sim.zToSimMax`, `sim.massDev = 0`), as in the tuning study. Every result can then be given for each true Z.
- **Decay angle.** θ_CM is uniform between 80° and 100°. Over this range, uniform in θ and uniform in cos θ agree to better than 1%. The range includes every angle where the fitted Z is usable: it is correct up to about 6° from 90°, and off by at most one up to about 8°.
- **Vertex depth.** The depth is uniform between the window and `zCutoff`, as in every simulation, and results are given in depth bands. Events near the window are kept, because the experimental-data cut there has not been defined either.

With 3000 events, the change in bias is measured to about 0.03 Z for each true Z, using the events within 4° of 90°, and to about 0.005 Z for the whole sample. Split by both Z and angle, each cell holds about 50 events, enough to see shifts of half a Z or more.

A separate sample of 1500 events with decay angles 10–30° from 90° is fitted once and never varied. It shows how badly the fit does at those angles, in case the experimental-data selection lets such events in.

**Checks on the nominal data.** The recommended settings were tested only at 90°, so the nominal data are fitted twice more. The first fit uses the reference settings, the production settings the recommended ones were tested against: a 1 mm step, 1000 random sampling points, longitudinal diffusion over ±3σ, gain fluctuations on, and 2 rounds of 100 iterations. It costs about 90 CPU-hours and confirms the recommended settings on the sample used for every systematic. The second fit repeats the recommended settings with a different seed. The difference between the two recommended fits is the fit's own random scatter.

### Running

Each variation is a section in a study file `studies/syst_*.ini`, run on the `hpc` cluster with the fission driver ([MANUAL.md](../MANUAL.md), "Running on a cluster"):

```
./fission.py --remote hpc --sync                              # build the committed code there
./fission.py studies/syst_<name>.ini --remote hpc --slurm     # submit
./fission.py studies/syst_<name>.ini --remote hpc --status    # check
./fission.py studies/syst_<name>.ini --remote hpc --fetch     # copy the results here
```

As in the tuning study, each sample is 3000 events in 60 chunks, and fits use `fit.threads = 4`. The study uses at most 128 cores at once. Jobs are not pinned to nodes: they are submitted held and released oldest first, once their inputs exist, while this study's running jobs use fewer than 128 cores. The tuning study's throttle script does this (see the [appendix](#appendix-cluster-paths)); it picks jobs by study-file name, so it needs `syst_*` added.

## Prerequisites

The probes are grouped in [Physics to probe](#physics-to-probe): A is space charge, drift and diffusion, B energy loss, C kinematics, D pads and electronics. Groups A to D are in order of importance. The last column lists the groups that need each item. Groups B and C can start as soon as their items are done.

| # | Item | Why it matters | Work | Needed by |
|---|---|---|---|---|
| P1 | **`AtSimpleSimulation::AddHit` places points at 2000 − z when a space-charge model is set** (`AtSimpleSimulation.cxx:177–179`: z → 1000 − z, `ApplySpaceCharge` keeps z, then `SetZ(1000 + z)`). | Every simulation with space charge is wrong. That includes the real-data fit in `adam/determineZ/run_cut.C`, which uses a line-charge model. No space-charge study can run until this is fixed. | Fix it (`SetZ(1000 - corr.Z())`), add a unit test, and recheck the `run_cut.C` results. | A |
| P2 | **The pseudo-data chain has no space charge.** The models exist but are unused in `run_simp_fiss.C`, `run_digi_fiss.C` and `run_fit.C`. The correction task is never added, RANSAC reads uncorrected hits, and λ = 0. | Experimental data are corrected with a line-charge model, with λ from the beam rate. Pseudo-data without space charge cannot test that correction. | Add the settings `sim.spaceCharge`, `sim.lambda`, `digi.scCorrection`, `digi.lambda` and `fit.spaceCharge`, building the chain the same way as the experimental-data analysis (`adam/unpack/unpack_linked.C`, `run_cut.C`). | A |
| P3 | **The simulation can use only a single fixed decay angle or the `fAsym` distribution.** | The baseline needs angles drawn uniformly over a range. | Add `sim.decayAngleMin` and `sim.decayAngleMax` to `eventSim.h`, drawing θ_CM uniformly between them. `decayAngle` on its own keeps its current meaning. | all |
| P4 | **Pseudo-data have no dead pads.** | Dead pads remove parts of the tracks from the experimental data. Without them, the pseudo-data cannot show how that affects the fitted Z. | Add a setting that makes a chosen set of pads dead in the digitization. | D |
| P5 | **The simulation and the fit use different mass units:** 939.0 MeV per nucleon in the simulation (`eventSim.h:29`) and 931.5 in the fit (`AtKinematics.cxx`). | Every pseudo-data set so far has fragment masses 0.8% off, and so the wrong velocities for a given kinetic energy. | Measure the effect once with the old unit (C4), then use the atomic mass unit in both before making the baseline. | all |
| P6 | **The local copy of the beam table is wrong.** On the desktop, `tpcSharedInfo/eLoss/LISE/83_200.txt` is actually the ¹⁴²Pr table. The cluster's copy is the correct ²⁰⁰Bi table, so cluster runs are unaffected. | Local runs simulate the beam with Pr stopping powers. The fit itself never uses a beam table: it infers the beam velocity from the folding angle. | Copy the table from the cluster to the local machine. | local runs |
| P7 | **There is no setting to scale the stopping power.** `AtELossTable::SetDensity` can scale a table, but the macros load the LISE tables with density 0, which `SetDensity` rejects. | B2 needs it. | Load the tables with the gas density or add a plain scale factor, and add a `sim.elossScale` setting. | B |
| P8 | **The digitization has no electronics noise, and the noise code is broken.** `AtPulse::ApplyNoise` (`AtPulse.cxx:126`) multiplies each sample by `Gaus(0, σ)` instead of adding it, so any nonzero `fNoiseSigma` wipes out the trace. | Experimental traces have noise. | Make the noise additive, add a unit test, and add a setting for it (D4). | D |
| P9 | **The digitization uses one average response for every pad.** `run_digi_fiss.C` makes every trace with the averaged response in `respAvg.root` and deconvolves with the same response. | Each pad has its own measured response, so the pseudo-data cannot test how much the deconvolution depends on it. | Make each pad's trace with that pad's measured response, and add a setting to deconvolve with either the per-pad responses or the average. | D |

## Physics to probe

The priority column suggests an order, with 1 first. The "Known from" column says what sets the size of each variation.

### A. Space charge, drift and diffusion

| # | Probe | Variation | Known from | Priority |
|---|---|---|---|---|
| A1 | Space charge: size | data with a line charge of λ_nominal, ½λ and 2λ; corrected and fitted with λ_nominal. λ comes from the beam rate: IC rate × 3 × 10⁻¹² + 4 × 10⁻⁹. | the spread of λ in the rate calibration | 1 (after P1, P2) |
| A2 | Space charge: shape | data distorted with other radial fields through `AtRadialChargeModel`: the charge spread uniformly over the beam's radius, so the field grows linearly inside it instead of being zero, and a field constant in z instead of linear in z; corrected and fitted with the line-charge model | — | 1 (after P1, P2) |
| A3 | Space charge: beam position | data with the beam line moved. The correction assumes the beam runs from (0, −6) at the window to (10, 0) at the pad plane. Nominal correction. | — | 1 (after P1, P2) |
| A4 | Space charge: no correction | data with space charge and no correction, to show how much the correction matters | — | 1 (after P1, P2) |
| A5 | Drift velocity | data with the drift velocity changed by ±1% and ±2%; nominal fit | — | 3 |
| A6 | Diffusion | data with the transverse and longitudinal coefficients changed by ±20%; nominal fit | — | 3 |

### B. Energy loss

The fit gets Z from the fragments' Bragg curves, so the stopping power is the most important input. Neither LISE nor SRIM describes the real stopping powers, and no better model exists.

| # | Probe | Variation | Known from | Priority |
|---|---|---|---|---|
| B1 | Fragment tables, LISE versus SRIM | LISE data fitted with SRIM, and SRIM data fitted with LISE, on the baseline at the recommended settings. At 90° only, the tuning study found −0.46 Z and −42 pp correct Z for LISE data fitted with SRIM, and +0.67 Z and −60 pp for the reverse. This study replaces those numbers. | the two models as upper and lower bounds | 1 |
| B2 | Fragment stopping-power scale | every fragment's dE/dx scaled by 0.95, 0.98, 1.02 and 1.05 in the data; nominal fit | the spread between LISE and SRIM | 1 |

B1 is the energy-loss systematic: the shift in Z for the difference between the two models, including how that difference changes with Z and velocity. B2 is mainly a check. The fit's amplitude is shared by both fragments, so it absorbs a stopping-power scale common to both; what remains comes from how fast the fragments slow down. B2 also gives a slope, the shift in Z per percent of stopping power, in case the models' overall scale is pinned down later.

### C. Reaction and fission kinematics

The fit fixes most of the kinematics. We assume the decay angle is 90° and the total kinetic energy (TKE) comes from Viola systematics. The masses follow from the charges, assuming the fragments keep the compound nucleus's charge-to-mass ratio: A = round(A_CN·Z/Z_CN). The compound nucleus is Z_CN = 85, A_CN = 204. The real reaction may differ from any of these. Errors in the TKE, the mass split, the mass unit or the decay angle all change the fragment velocities the fit assumes, and the stopping power depends strongly on velocity.

| # | Probe | Variation | Known from | Priority |
|---|---|---|---|---|
| C1 | Decay angle | the baseline itself: accuracy in 2° bands out to 10° from 90°, plus the 10–30° sample. Every other probe's shift per angle band comes from the same tables. | — | 1 (this is the baseline) |
| C2 | Total kinetic energy | data with the TKE shifted by ±2%, and with a spread around Viola (σ about 8% of the TKE, as in measured fission TKE distributions); fit with Viola | published TKE systematics for this mass region | 2 |
| C3 | Charge-to-mass assignment | data with the fragment A shifted by ±0.5 to ±1 units from the value above (charge polarization, the old `massDev`); nominal fit | charge-polarization systematics | 2 |
| C4 | Mass unit (P5) | data at 939.0 and at 931.5 MeV per nucleon; fit at 931.5; run before the baseline is made | — | 1 (once, with P5) |
| C5 | Beam direction and spot | data with a different beam spot and spread of beam directions (`eventSim.h`); the fit takes its directions from the Y-pattern | — | 3 |

### D. Pads, gain and electronics

| # | Probe | Variation | Known from | Priority |
|---|---|---|---|---|
| D1 | **Dead pads** | data with one whole AsAd dead (272 channels) and with scattered single dead pads, against data without dead pads, reported by whether a fragment crosses the dead pads. The fit sums simulated charge only over pads with an experimental hit, so a dead pad drops out whether or not it is flagged. | — | 1 |
| D2 | Response consistency | each pad's trace made with its own measured response, then deconvolved with the average response instead of pad by pad; nominal fit. This bounds how much the deconvolution depends on using each pad's own response. | the measured per-pad responses | 1 |
| D3 | Pad-to-pad gain variation | data with each pad's gain scaled by a fixed random factor (σ = 5% and 10%), and with a common gain offset for each AsAd or AGET (σ = 5% and 10%). Each time bucket sums many pads, so random per-pad factors mostly average out. An offset shared by a region does not: a fragment crossing it gets a step in its charge curve that looks like a change in stopping power. The experimental data are not gain-calibrated: `AtFilterCalibrate` is turned off in `unpack_linked.C`. | — | 3 |
| D4 | Electronics noise (P8) | data with noise at a nominal level and at twice that level | — | 3 |

## Order of work

1. **P3, P5 with C4, P6 and P7**, then the baseline. That means the ±10° sample, with one reference fit, the recommended fit and a second recommended fit with another seed, and the 10–30° sample.
2. **Space charge**, after P1 and P2: A1, A2, A3, A4. Energy loss (B1, B2) can run while P1 and P2 are being fixed.
3. **P4, P8 and P9**, then dead pads (D1), response consistency (D2) and kinematics: C2, C3, C5.
4. **The rest:** A5, A6, D3, D4.

Each step's study files go in `studies/syst_*.ini`. Its results and decisions go in this folder's README and LOG, as in the tuning study.

## Cost

At the recommended settings, the fit costs about 10 CPU-s per event and the simulation about 6 CPU-s per event. Every simulated event falls inside the angle range, so every one is fitted. One variation with 3000 events costs about 15 CPU-hours. The roughly 55 variations above, counting each value of δ, come to about 800 CPU-hours. The one-off fits of the baseline add about 150 CPU-hours: the reference fit, the second seed and the 10–30° sample. In total that is about seven hours on the cluster at the 128-core limit.

## Output

The result is a table of systematic uncertainties on the fitted light-fragment Z, for each true Z and each decay-angle band. For each input, it gives the shift in Z for that input's uncertainty, from the slope where the input is continuous, and the change in the fraction with the correct Z. Once the analysis of the experimental data settles on an angle selection, the table is averaged over it. Inputs with the same effect are combined before everything is added in quadrature. The difference between the energy-loss models (B1) is kept separate.

## Future work

**Pattern recognition and the fit itself.** These probes are left for later; they matter less than the effects above for now.

| # | Probe | Variation |
|---|---|---|
| E1 | Y-pattern settings | RANSAC distance, minimum hit count, charge threshold |
| E2 | Vertex and directions | the fit takes these from the Y-pattern. Compare with the true pattern (the bias study found −0.03 ± 0.02 Z), and with the pattern smeared by its resolution in the pseudo-data. |
| E4 | Objective function | the χ² weights each time bucket by 1/Q_data and uses one amplitude for both fragments. The bias study left a +0.1 Z sub-integer residual unexplained. Alternatives are a separate amplitude for each fragment, or Poisson-like weights. |

**ADC saturation.** Data clipped at the ADC range. The fit excludes pads whose reconstructed hit charge is above 4200, but a saturated trace can deconvolve to a charge below 4200, and that pad then enters the fit with too little charge. The highest-charge pads are most likely the heavy fragment's, so the bias need not be the same for both fragments. This needs clipping added to the digitization.

**First time bucket kept.** Now 80, which drops the 7 buckets nearest the pad plane, so its effect depends on the vertex depth; varied in both the digitization and the fit. The pulse-shape-analysis and χ² thresholds are not varied: they are in charge units, and without a gain map between simulation and experiment their values have no counterpart in the experimental data.

**Sizes from experimental data.** This study uses no experimental data. Where a variation's size would come from experimental data, the table above gives "—", and the size is chosen by hand. With experimental data, a later study could set these sizes:

- **Dead pads (D1):** a complete map of the pads that are actually dead, built as described below.
- **Pad-to-pad gain (D3):** the spread of pad gains in beam or pulser data.
- **Electronics response and noise (D2, D4):** the responses of single pads and the pedestal widths.
- **Drift velocity (A5):** the time buckets of the window and the pad plane (TBEntrance 457).
- **Diffusion (A6):** the hit widths of beam and fragment tracks.
- **Beam spot, direction and position (C5, A3):** beam tracks.

**Building a dead-pad map.** The repo has no map of dead pads measured from data. The smart-zap list holds low-gain pads, `getInhibitPads.C` picks pads by radius, and `getConstPads.C` finds pads that are always on. A complete map could be built from experimental data:

1. Over many runs, count the events in which each pad has a hit, and compare with its neighbors at the same radius and in the same φ region. Beam-related and fragment tracks cover the pad plane unevenly, so compare with the occupancy expected at that pad's position, not a global average.
2. Pads hit far less often than their neighbors are dead. Pads with far less charge than their neighbors on the same tracks have low gain; compare them with the smart-zap list. Pads that are always on are noisy; the existing `getConstPads.C` finds these.
3. Check that the map is stable from run to run, since pads can die or recover.

The map would then set the dead pads in D1.

## What was done before

Skyler's handoff notes (`skyler/NOTES.md` on `skyler-fission`) list the studies below. Each used a single true Z per run and a few hundred events. All of them used pseudo-data digitized with the stretched electronics response (the `response.h` bug), so none of their numbers still hold.

| study | result (width of true − fitted Z) |
|---|---|
| symmetric split, one Z | σ 0.41, unbiased |
| decay angle fixed at 90° and at 120° | σ 0.17 (best case) |
| mass deviation 2 (`massDev`) | σ 0.68, centered |
| asymmetric split | σ 0.69, centered |
| simulation SRIM, fit LISE | σ 1.14, two peaks at ±1 |
| diffusion off (git history, 2025-02) | no recorded result |

The study files `decay_angle.ini`, `mass_dev.ini` and `eloss_mismatch.ini` roughly reproduce the first five. `digi_scan.ini` and `fit_scan.ini` scan `respScale`, with no recorded result.

## Open questions for the analysis

- How the experimental sample is selected in angle (the folding-angle-versus-depth cut) and near the window. Once both cuts are defined, their efficiencies are needed, and those depend on the anisotropy coefficients in `eventSim.h` and where they come from.
- What uncertainty to assign to λ (from the beam-rate calibration), to the drift velocity and to the diffusion coefficients.
- Whether e12014 has a pad-gain calibration. The calibration task exists but is turned off in the unpacking.
- Which references to use for the TKE (C2).

## Appendix: cluster paths

- Throttle script from the tuning study: `/home/faculty/aanthony/fission/adam/tune/throttle.sh` on `hpc`, with its log in the same directory.
