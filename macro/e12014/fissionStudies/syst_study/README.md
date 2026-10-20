# Systematic Uncertainties of the Fitted Fragment Z: Results

**Scope (2026-10-20).** The report covers the prerequisites, the baseline, and these probes of the [plan](PLAN.md): energy loss (B1 with SRIM and Ziegler stopping powers, B2), space charge (A1–A4), gas (A5, A6), kinematics (C2, C3, C5), and dead pads, pad gains and noise (D1, D3, D4). It does not cover D2, the electronics response of each individual pad, because these responses are not available. All results are from simulated events (pseudo-data). Decision log: [LOG.md](LOG.md).

**Notation.**
- **dZ** = true Z − fitted Z of the light fragment. A positive dZ means the fit gives the light fragment too low a Z.
- **Mean dZ within 2**: the mean of dZ over the fits with |dZ| ≤ 2. **Mean dZ of all fits**: the mean of dZ over every fit. Both are defined fully in [What is measured](#what-is-measured).
- **θ_CM** is the decay angle in the center-of-mass frame, the angle of the heavy fragment's momentum to the beam. Below 90° the heavy fragment goes forward and the light fragment backward.
- **Folding angle**: the angle between the two fragment tracks. The fit infers the beam velocity from it.
- **pp** is percentage points.
- **Δ** (a change) is a variation minus the sample or fit it is compared with, usually the baseline. It is computed event by event and averaged over the events present in both. Unless stated otherwise, both are fitted with the recommended settings.
- In the names of variations, "data" means the pseudo-data, as in "SRIM data fitted with LISE".

## Summary

The fit on the baseline sample, and checks of the fit and the simulation:

- **The decay angle shifts the fitted Z almost linearly: dZ changes by +0.144 ± 0.007 Z per degree of θ_CM − 90°.** The slope is fitted to events within 8° of 90° and is about the same for every true Z. The straight line holds out to about 20° from 90°, where the mean dZ of all fits reaches −2.7 below 90° and +2.3 above it (15–20°). When the heavy fragment goes forward (θ_CM < 90°), the fit gives the light fragment too high a Z; when it goes backward, too low.
- **The fitted Z is correct only within a few degrees of 90°, and the two sides of 90° differ.** Between 88° and 90°, 98% of the fits have the correct Z; between 90° and 92°, 86%. Two to four degrees from 90°, the fraction is 70% below 90° and 42% above it. Up to 8° from 90°, 94% or more of the fits are within one Z. At 15–20° from 90°, 43–69% are off by 3 or more; beyond 20°, 79–96%.
- **Every systematic shift must be reported against the signed angle θ_CM − 90°.** In bins of the unsigned angle |θ_CM − 90°| the shifts on the two sides of 90° cancel: the mean dZ within 2 stays below 0.16 Z while the spread grows to 1.3 Z.
- **Vertices near the window give most of the fits off by 3 or more.** In the baseline sample (decay angles within 10° of 90°), 20% of the fits with the vertex within 25 mm of the window are off by 3 or more, mostly with the two fragments' Z exchanged. With the vertex 25–50 mm from the window, 10% are; beyond 50 mm, 2% or less. Of the sample's 52 fits off by 3 or more, 45 have their vertex within 50 mm of the window.
- **The recommended settings give the same results as the reference settings, except between 90° and 92°.** Against the reference fit of the same events, the recommended fit changes the fraction with the correct Z by −0.4 ± 0.4 pp, the mean dZ within 2 by +0.015 ± 0.005 Z and the fraction off by 3 or more by +0.1 ± 0.1 pp. Between 90° and 92° it has 5.8 ± 1.5 pp fewer fits with the correct Z. The recommended fit takes 10.4 CPU-s per event, the reference fit 109.6.
- **Refitting with another seed changes the results by no more than their statistical errors.** The fraction with the correct Z changes by +0.4 ± 0.5 pp and the mean dZ within 2 by −0.001 ± 0.005 Z.
- **The mass unit used in the simulation does not change the fit (probe C4 of the plan).** Simulating at 939.0 instead of 931.5 MeV per mass unit changes the fraction with the correct Z by +0.8 ± 0.7 pp, the mean dZ within 2 by +0.008 ± 0.008 Z and the folding angle by +0.001 ± 0.001°. The simulation starts from kinetic energies, so scaling every mass leaves every lab kinetic energy unchanged to a relative 10⁻⁵, and the energy-loss tables depend only on kinetic energy.

Systematic shifts, largest first (every value over the whole sample unless a range of decay angle, vertex depth or Z is given):

- **A stopping-power model in the fit that differs from the pseudo-data's shifts the fitted Z: by about 0.6 Z between LISE and SRIM, more for asymmetric splits, and by about 0.05 Z between LISE and Ziegler.** SRIM data fitted with LISE shift the mean dZ within 2 by +0.60 ± 0.01 Z; LISE data fitted with SRIM, by −0.55 ± 0.01 Z. The shift is about the same at every decay angle within 10° of 90°. Within 4° of 90°, it is about 1 Z for a light-fragment true Z of 31–32 and 0.15 Z for Z = 42, falling with Z with one exception (+0.84 Z for SRIM data fitted with LISE at Z = 41). SRIM data fitted with SRIM give the same mean dZ within 2 as the baseline (+0.007 ± 0.009 Z). Ziegler data fitted with LISE shift it by +0.055 ± 0.008 Z, and LISE data fitted with Ziegler by −0.039 ± 0.006 Z. Ziegler differs from LISE mostly below 200 MeV, which only fragments on long tracks reach.
- **Electronics noise of σ = 5 ADC counts shifts the mean dZ of all fits by +0.25 Z.** Gaussian noise on every trace with charge shifts the mean dZ of all fits by +0.250 ± 0.030 Z at σ = 5 ADC and by +0.463 ± 0.042 Z at 10 ADC. At 5 ADC the shift has the same sign in every 2° band within 10° of 90° (+0.06 to +0.34 Z). At 5 ADC and within 4° of 90°, the mean dZ within 2 shifts by +0.26 to +0.35 Z for a light-fragment Z of 31–35 and by +0.05 to +0.08 Z for Z = 40–42. The fit's simulation has no noise.
- **An error in the line charge λ that the space-charge correction assumes shifts the mean dZ within 2 by 0.12 Z per 10⁻⁸ C/m.** The correction and the fit assume λ = 1.54 × 10⁻⁸ C/m. Pseudo-data with half and twice that λ shift the mean dZ within 2 by −0.105 ± 0.008 and +0.165 ± 0.011 Z. The wrong correction changes the folding angle, from which the fit infers the beam velocity, by −0.7° and +1.2°. When the correction uses the pseudo-data's own λ, it removes the effect of the space charge: the mean dZ within 2 differs from that of pseudo-data without space charge by +0.005 ± 0.006 Z. Without any correction, the mean dZ within 2 shifts by +0.095 ± 0.014 Z, and 83.9% of the fits are within one Z, against 92.8% without space charge (see [Fitted Z against the decay angle](#fitted-z-against-the-decay-angle) for the latter).
- **The distribution of the space charge along the beam shifts the mean dZ within 2 by +0.146 Z; its radial shape and the beam position matter only for vertices near the window.** The same total charge spread evenly from the pad plane to the window, instead of growing toward the window as the correction assumes, shifts the mean dZ within 2 by +0.146 ± 0.010 Z. It shifts the mean dZ of all fits by +0.09 to +0.16 Z for vertices 100–600 mm from the window; over the whole sample, extra fits off by 3 or more near the window cancel this (−0.010 ± 0.045 Z). Spreading the charge over the 20 mm beam radius, or moving the beam line by 5 mm or onto the simulated beam's mean track, shifts the mean dZ within 2 by at most 0.015 Z, but adds 2–11 pp of fits off by 3 or more for vertices within 100 mm of the window.
- **A transverse diffusion coefficient 20% off the one the fit simulates shifts the mean dZ within 2 by ±0.11 Z (+0.0055 ± 0.0003 Z per percent).** The shift is largest for vertices far from the pad plane and for asymmetric splits. A longitudinal diffusion coefficient 20% off shifts it by at most 0.04 Z.
- **A drift velocity 1% off the one the hits are reconstructed with shifts the mean dZ within 2 by 0.020 ± 0.002 Z** and the folding angle by about 0.31°.
- **Errors of plausible size in the reaction's kinematics shift the mean dZ within 2 by at most 0.023 Z.** A total kinetic energy (TKE) 2% above or below Viola systematics shifts it by +0.021 ± 0.006 and −0.023 ± 0.006 Z. A spread around Viola's TKE with σ = 8% shifts it by +0.007 ± 0.024 Z and leaves 2.3 pp fewer fits within one Z. Shifting the light fragment's mass number by ±1 (the heavy fragment's by ∓1) shifts it by at most 0.016 Z. Doubling the beam spot, tripling the beam's angular spread or tilting the beam by 10 mrad shifts it by at most 0.009 Z.
- **Scaling every fragment's stopping power by the same factor, 0.95 to 1.05, shifts the mean dZ within 2 by at most 0.014 Z (+0.0025 ± 0.0008 Z per percent).** The fit's shared amplitude absorbs the scale. The mean dZ within 2 moves most, by ±0.05 Z for ±5%, for vertices within 200 mm of the window, whose fragments stop in the gas.

Changes that add failed fits or widen the distribution of dZ:

- **The readout board (AsAd, 256 pads) that is dead in the experimental data puts 32% of the fits 3 or more Z off in the events that lose many pads to it.** With CoBo 2, AsAd 3 dead, 323 events (11%) have at least 10% of their fragments' pads on that AsAd. Of these events, 31.6% are fitted 3 or more Z off, against 1.2% with every pad working. The other events do not change. Further dead pads, up to 10% of the pads the fit uses, scattered or in patches, add at most 0.5 pp of fits off by 3 or more.
- **Gain factors common to an AsAd or an AGET chip widen the distribution of dZ without shifting it.** With a fixed gain factor per AsAd drawn with σ = 10%, 62% of the fits are within one Z (against 93% with equal gains) and 15.5% are off by 3 or more; the mean dZ of all fits changes by −0.050 ± 0.036 Z. With σ = 5% per AsAd or σ = 10% per AGET (64 pads), 78–79% of the fits are within one Z. Independent factors per pad leave 91.5% (σ = 5%) and 88.4% (σ = 10%) of the fits within one Z.

![The fitted light-fragment Z against the true decay angle](figures/angle.png)

## Size of each systematic shift

The main variations of each input, over the whole ±10° sample (3000 events). Each row is compared with the baseline, except the space-charge rows other than "no correction", which are compared with the [λ = 1.54 × 10⁻⁸ C/m space-charge sample](#space-charge-of-the-beam-probes-a1a4): pseudo-data with that λ, corrected and fitted with the same λ. The size of most variations is chosen by hand, because the uncertainties of the inputs are not known. Where an input is continuous, the last column gives the shift per unit of the input, so that the shift can be scaled to the input's uncertainty. The other variations, and the shifts per decay angle and per true Z, are in the sections under [Systematic shifts](#systematic-shifts). For the experimental data, the shifts are to be averaged over the events that its selection in decay angle and vertex depth keeps; this selection is not defined yet.

| input (probe) | variation | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) | scaling or dependence |
|---|---|---|---|---|---|
| Stopping power (B1) | SRIM data fitted with LISE | +0.602 ± 0.011 | +0.657 ± 0.027 | +1.2 ± 0.3 | within 4° of 90°: about 1 Z for light Z 31–32, 0.15 Z for Z 42 |
|  | LISE data fitted with SRIM | −0.553 ± 0.010 | −0.555 ± 0.026 | +0.2 ± 0.1 | as above, opposite sign |
|  | Ziegler data fitted with LISE | +0.055 ± 0.008 | +0.085 ± 0.026 | −0.1 ± 0.2 |  |
|  | LISE data fitted with Ziegler | −0.039 ± 0.006 | −0.003 ± 0.021 | −0.1 ± 0.1 |  |
| Stopping-power scale (B2) | LISE dE/dx × 1.05 | +0.013 ± 0.006 | +0.047 ± 0.023 | −0.2 ± 0.2 | +0.0025 ± 0.0008 Z per percent |
| Space charge: λ (A1) | 2λ in the data | +0.165 ± 0.011 | +0.252 ± 0.026 | +0.1 ± 0.2 | +0.117 ± 0.006 Z per 10⁻⁸ C/m of error in λ |
|  | λ / 2 in the data | −0.105 ± 0.008 | −0.307 ± 0.048 | +1.1 ± 0.2 | 72% of the events within 40 mm of the window are unfittable |
| Space charge: shape (A2) | same charge, constant along the beam | +0.146 ± 0.010 | −0.010 ± 0.045 | +1.8 ± 0.3 | +0.09 to +0.16 Z (all fits) at 100–600 mm from the window |
|  | charge spread over the 20 mm beam radius | +0.003 ± 0.006 | −0.175 ± 0.045 | +1.7 ± 0.3 | only within 100 mm of the window |
| Space charge: beam line (A3) | beam line 5 mm off in y | −0.012 ± 0.006 | −0.138 ± 0.045 | +1.0 ± 0.3 | only within 100 mm of the window |
| Space charge: no correction (A4) | no correction, no space charge in the fit | +0.095 ± 0.014 | +0.166 ± 0.034 | +0.9 ± 0.3 | folding angle +1.46° |
| Drift velocity (A5) | × 1.02 | +0.045 ± 0.006 | +0.097 ± 0.026 | −0.2 ± 0.2 | +0.020 ± 0.002 Z per percent |
| Transverse diffusion (A6) | × 1.2 | +0.106 ± 0.007 | +0.093 ± 0.028 | +0.6 ± 0.2 | +0.0055 ± 0.0003 Z per percent; largest for long drifts |
| Longitudinal diffusion (A6) | × 1.2 | +0.019 ± 0.005 | +0.035 ± 0.024 | +0.1 ± 0.2 |  |
| TKE (C2) | × 1.02 | +0.021 ± 0.006 | +0.040 ± 0.025 | −0.3 ± 0.2 | +0.011 ± 0.002 Z per percent |
|  | 8% spread | +0.007 ± 0.024 | +0.024 ± 0.047 | +0.2 ± 0.3 | 2.3 pp fewer within 1 |
| Mass number (C3) | light fragment's A + 1 | +0.016 ± 0.006 | +0.079 ± 0.028 | −0.2 ± 0.2 |  |
| Beam spot and direction (C5) | beam spot spread × 2 | −0.002 ± 0.007 | +0.024 ± 0.038 | +1.2 ± 0.3 | angular spread × 3 and 10 mrad tilt: at most 0.009 Z (within 2), at most +0.3 pp off ≥ 3 |
| Dead pads (D1) | dead AsAd (CoBo 2, AsAd 3) | −0.016 ± 0.007 | −0.173 ± 0.045 | +3.1 ± 0.4 | 31.6% off by ≥ 3 in the 11% of events with ≥ 10% of their pads on it |
|  | the dead AsAd + 10% of the pads the fit uses, in patches of 16 | −0.023 ± 0.008 | −0.187 ± 0.044 | +3.6 ± 0.4 |  |
| Pad gains (D3) | per AsAd, σ = 10% | −0.041 ± 0.024 | −0.050 ± 0.036 | +13.7 ± 0.6 | 62% within 1 instead of 93% |
|  | per AGET, σ = 10% | −0.015 ± 0.016 | −0.015 ± 0.032 | +2.6 ± 0.3 | 78% within 1 |
|  | per pad, σ = 10% | +0.010 ± 0.010 | +0.004 ± 0.027 | −0.0 ± 0.2 | 88% within 1 |
| Noise (D4) | σ = 5 ADC | +0.198 ± 0.008 | +0.250 ± 0.030 | −0.2 ± 0.2 | within 4° of 90°: +0.26 to +0.35 Z for light Z 31–35, +0.05 to +0.08 Z for Z 40–42 |
|  | σ = 10 ADC | +0.418 ± 0.012 | +0.463 ± 0.042 | +2.2 ± 0.4 |  |

![The change of both means for each variation](figures/shifts.png)

## The baseline

### Samples

All samples use the LISE energy-loss tables and the measured electronics response. They are simulated in more detail than any fit: 0.25 mm steps and 4000 sampling points per charge line. In every event the light fragment's Z is drawn uniformly from 31 to 42 (heavy fragment 54 to 43), with A = round(204 · Z / 85) and no spread in mass. The vertex depth, its distance from the window, is uniform over 0–700 mm, from the window to 300 mm before the pad plane.

| sample | study file | events | θ_CM | used for |
|---|---|---|---|---|
| ±10° (`base`) | `syst_base.ini` | 3000 | uniform over 80–100° | the baseline: every variation is compared with it, directly or through the λ = 1.54 × 10⁻⁸ C/m space-charge sample |
| 10–30° (`wide_lo`, `wide_hi`) | `syst_base.ini` | 750 + 750 | uniform over 60–80° and 100–120° | fitted once, never varied |
| C4 (`mu931`, `mu939`) | `syst_c4.ini` | 1000 at each mass unit | uniform over 80–100° | the mass unit only |

Each Z from 31 to 42 has 231–276 events in the ±10° sample. Every event in every sample has a Y-pattern: the Y-shaped vertex of beam and fragment tracks that the fit starts from. The simulation redraws an event when a fragment's track ends within 50 mm of the beam axis; one of the 5500 events, in the 10–30° sample, is redrawn.

### Fits

| fit | settings | CPU-s / event |
|---|---|---|
| `rec` | recommended: 2 mm step, a fixed pattern of 100 sampling points, longitudinal diffusion over ±2σ, no gain fluctuations, 3 rounds of 40 iterations | 10.4 |
| `rec2` | recommended, with another seed | 10.5 |
| `ref` | reference: 1 mm step, 1000 random sampling points, ±3σ, gain fluctuations, 2 rounds of 100 iterations | 109.6 |

Every fit uses 4 threads. The 10–30° and C4 samples are fitted with the recommended settings only.

### What is measured

Computed by `syst_report.py` over every event of a fit:

- **dZ** compares the fitted Z with the true Z of the same fragment. Fitted and true fragments are matched by azimuth, as in the [tuning study](../tune_study/README.md). No event in any sample has fragments too close in azimuth to match.
- **Correct Z**: dZ = 0. **Within 1**: |dZ| ≤ 1. **Off by 3 or more**: |dZ| ≥ 3.
- **Mean dZ within 2** and **spread**: the mean and the standard deviation of dZ over the fits within 2 of the true Z (|dZ| ≤ 2). Leaving out the fits off by 3 or more keeps the few events with the two fragments' Z exchanged from dominating.
- **Mean dZ of all fits**: the mean of dZ over every fit. The figure, the straight line in θ_CM and the 10–30° table use it, and every table of systematic shifts gives it next to the mean dZ within 2. The two means differ when a change adds fits off by 3 or more, or widens the distribution of dZ. A wider distribution moves fits across the edge of the |dZ| ≤ 2 window, and this pulls the mean dZ within 2 toward zero.
- **Unfittable**: none of the fit's simulations puts charge on a pad that has a hit in the event. These events are counted, then left out of every other measure. The ±10° sample has 4 of them in 3000 events, all within 100 mm of the window.
- **Folding angle**: the angle between the two fragment directions of the Y-pattern. The fit uses it to infer the beam velocity.
- **Changes between two fits** are computed event by event and quoted with one standard error. Events can be paired when both fits are of the same simulated events. They can also be paired between the two C4 samples: their simulations use the same seeds, and all 1000 events have the same Z split and fragment directions in both.

## Fit results on the baseline and mass-unit samples

### Fitted Z against the decay angle

Over the whole ±10° sample, the recommended fit gives 32.3% of the events the correct Z; 92.8% are within one Z and 1.7% off by 3 or more. The mean dZ within 2 is +0.091 ± 0.017 Z and the spread 0.91 Z. In bins of θ_CM − 90°:

| θ_CM − 90° | events | correct Z | within 1 | off by ≥ 3 | mean dZ within 2 (Z) | spread (Z) |
|---|---|---|---|---|---|---|
| −10 to −8° | 286 | 1.0% | 80.1% | 1.4% | −1.18 | 0.41 |
| −8 to −6° | 309 | 4.9% | 97.7% | 0.6% | −0.94 | 0.34 |
| −6 to −4° | 292 | 21.6% | 98.6% | 1.4% | −0.77 | 0.43 |
| −4 to −2° | 291 | 70.1% | 96.9% | 2.4% | −0.28 | 0.47 |
| −2 to 0° | 282 | 98.2% | 98.9% | 1.1% | −0.01 | 0.08 |
| 0 to +2° | 295 | 86.4% | 98.6% | 1.4% | +0.12 | 0.33 |
| +2 to +4° | 320 | 41.9% | 96.6% | 3.1% | +0.55 | 0.52 |
| +4 to +6° | 315 | 4.1% | 97.5% | 1.9% | +0.96 | 0.25 |
| +6 to +8° | 304 | 1.0% | 94.1% | 2.0% | +1.03 | 0.22 |
| +8 to +10° | 306 | 1.0% | 69.0% | 2.0% | +1.27 | 0.51 |

The statistical errors are 1–3 pp on the fractions and 0.01–0.03 Z on the mean dZ within 2.

- **dZ is close to a straight line in θ_CM − 90°.** A straight line fitted to dZ of every event within 8° of 90° has a slope of +0.144 ± 0.007 Z per degree and a value of −0.07 Z at 90°. The fitted Z is an integer, so the fraction with the correct Z stays high until the shift approaches half a unit, then falls steeply. The line crosses zero at about +0.5°, but the fit is most often correct below 90° (next bullet): the line is fitted to every fit, including those off by 3 or more, so it does not mark the angle of the best fits.
- **The fit is most often correct just below 90°, not at 90°.** In 1° bins, 97–99% of the fits between 88° and 90° have the correct Z, against 92% between 90° and 91° and 82% between 91° and 92°.
- **Bins of the unsigned angle |θ_CM − 90°| hide the shift.** In such bins the shifts on the two sides of 90° cancel: the mean dZ within 2 stays below 0.16 Z while the spread grows to 1.3 Z. Systematic shifts must therefore be reported against the signed angle θ_CM − 90°.
- **The fit assumes a 90° decay, so any other decay angle shifts Z.** The fit takes the fragment directions from the data and derives the fragment energies from them, assuming a 90° decay (`AtMCFission`, `thCM`). When the light fragment actually goes backward in the center-of-mass frame (θ_CM < 90°), its lab energy is lower than the fit assumes, and its track is shorter. The fit makes up for the shorter track with a higher Z, which loses energy faster. The heavy fragment goes forward and has more energy than the fit assumes, so the fit gives it a lower Z. The two Z add up to 85, so both effects raise the light fragment's Z. When the light fragment goes forward, the reverse happens.
- **The slope is about the same for every true Z:**

  | true light-fragment Z | 31 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 41 | 42 |
  |---|---|---|---|---|---|---|---|---|---|---|---|---|
  | slope (Z per degree) | 0.20 | 0.12 | 0.15 | 0.09 | 0.16 | 0.12 | 0.15 | 0.15 | 0.14 | 0.15 | 0.16 | 0.16 |
  | error | 0.04 | 0.04 | 0.01 | 0.03 | 0.03 | 0.02 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 |
  | correct Z within 2° of 90° | 78% | 94% | 88% | 94% | 90% | 94% | 89% | 89% | 98% | 98% | 98% | 96% |

  Each true Z has 37–67 events within 2° of 90°, so the fractions in the last row carry errors of 2–7 pp. The asymmetric splits (light-fragment Z 31–38) have the correct Z slightly less often than the near-symmetric ones (39–42).

### Fitted Z at decay angles 10–30° from 90°

The recommended fit on the 10–30° sample:

| θ_CM − 90° | events | correct Z | within 1 | off by ≥ 3 | mean dZ of all fits |
|---|---|---|---|---|---|
| −30 to −25° | 197 | 0.5% | 1.0% | 95.9% | −4.12 ± 0.08 |
| −25 to −20° | 166 | 0.0% | 1.2% | 94.6% | −3.48 ± 0.10 |
| −20 to −15° | 181 | 0.0% | 0.6% | 68.5% | −2.73 ± 0.10 |
| −15 to −10° | 206 | 0.5% | 14.6% | 4.4% | −2.20 ± 0.16 |
| +10 to +15° | 181 | 0.0% | 17.7% | 2.2% | +1.78 ± 0.07 |
| +15 to +20° | 167 | 0.0% | 4.8% | 43.1% | +2.32 ± 0.09 |
| +20 to +25° | 212 | 0.5% | 2.4% | 79.0% | +2.74 ± 0.09 |
| +25 to +30° | 190 | 1.1% | 4.2% | 85.8% | +3.15 ± 0.12 |

- **Between 10° and 15° from 90°, about 80% of the fits are off by exactly 2**, with the sign the straight line predicts.
- **Beyond 20° from 90° most fits are off by 3 or more** (79–96%); at 15–20°, 69% below 90° and 43% above.
- **The mean dZ of all fits follows the straight line out to about 20° on both sides.** Beyond 20° it grows more slowly above 90° than below (+3.15 against −4.12 at 25–30°).

### Fitted Z against the vertex depth

The recommended fit on the ±10° sample, by the true vertex depth:

| vertex depth (mm) | events | correct Z | within 1 | off by ≥ 3 | of these, Z exchanged | correct Z within 2° of 90° | mean folding angle |
|---|---|---|---|---|---|---|---|
| 0–25 | 179 | 29% | 73% | 20.1% | 25 of 36 | 82% (33 events) | 27.1° (0–100 mm) |
| 25–50 | 87 | 25% | 84% | 10.3% | 2 of 9 | 88% (16) | |
| 50–100 | 199 | 27% | 92% | 1.5% | 1 of 3 | 84% (44) | |
| 100–200 | 425 | 30% | 93% | 0.7% | 2 of 3 | 89% (72) | 28.3° |
| 200–300 | 424 | 32% | 92% | 0% | — | 95% (74) | 29.5° |
| 300–400 | 441 | 31% | 94% | 0% | — | 94% (96) | 31.1° |
| 400–500 | 425 | 32% | 94% | 0% | — | 96% (79) | 32.9° |
| 500–600 | 432 | 35% | 97% | 0% | — | 93% (84) | 35.4° |
| 600–700 | 384 | 41% | 98% | 0.3% | 0 of 1 | 96% (78) | 38.4° |

- **Near the window the fit often gives the light fragment the heavy fragment's Z.** Elsewhere this is rare (3 events beyond 50 mm). The cause is not investigated here. Requiring the vertex to be at least 50 mm from the window would remove 45 of the 52 fits off by 3 or more, and 9% of the events.
- **Deeper vertices give slightly better fits.** The fraction within one Z rises from 92% at 100–200 mm to 98% at 600–700 mm. The folding angle grows with depth because the beam slows down on its way from the window.

### Recommended settings compared with the reference settings

The recommended fit minus the reference fit, event by event:

| θ_CM − 90° | Δ correct Z (pp) | Δ mean dZ within 2 (Z) | Δ off by ≥ 3 (pp) |
|---|---|---|---|
| all | −0.4 ± 0.4 | +0.015 ± 0.005 | +0.1 ± 0.1 |
| −2 to 0° | +0.0 ± 0.5 | +0.000 ± 0.005 | +0.4 ± 0.4 |
| 0 to +2° | −5.8 ± 1.5 | +0.059 ± 0.015 | +0.0 ± 0.0 |
| +2 to +4° | −2.5 ± 2.1 | +0.016 ± 0.024 | +0.0 ± 0.4 |
| other 2° bins | −0.3 to +2.1, each within 1.2 standard errors of 0 | −0.011 to +0.030 | −0.7 to +0.3 |

- **Over the whole ±10° sample the two settings agree within the tuning study's limits.** These limits are ±2 pp for the correct Z, ±0.05 Z for the mean dZ within 2, ±1 pp for the fraction off by 3 or more, and ±0.1 Z for the mean dZ within 2 of every true Z. The largest change in the mean dZ within 2 for a single true Z is +0.047 ± 0.024 Z, at Z = 31.
- **Between 90° and 92° the reference fit has the correct Z more often.** There, 12% of the recommended fits are one Z away, so many events lie near the boundary between two Z. The recommended search simulates each Z fewer times (3 rounds of 40 iterations against 2 rounds of 100). A neighboring Z therefore reaches the lowest χ² more often, and more of these events end on the wrong Z. The tuning study's events, all at exactly 90°, show no such loss.

### The recommended fit repeated with another seed

**Refitting with another seed changes the results by no more than their statistical errors.** The recommended fit with another seed changes the fraction with the correct Z by +0.4 ± 0.5 pp, the mean dZ within 2 by −0.001 ± 0.005 Z and the fraction off by 3 or more by −0.1 ± 0.2 pp. In each 2° bin of θ_CM − 90°, the changes in the fraction with the correct Z are at most 2.7 pp and those in the mean dZ within 2 at most 0.03 Z, each within 1.6 standard errors of zero. The folding angle is the same in both fits, since it comes from the data.

### Mass unit of the simulation (probe C4)

The C4 events simulated at 939.0 MeV per mass unit minus the same events simulated at 931.5 MeV per mass unit (the fit's value), event by event, both fitted with the recommended settings:

| | Δ correct Z (pp) | Δ mean dZ within 2 (Z) | Δ off by ≥ 3 (pp) | Δ folding angle (deg) |
|---|---|---|---|---|
| all 1000 events | +0.8 ± 0.7 | +0.008 ± 0.008 | −0.4 ± 0.3 | +0.001 ± 0.001 |
| per 2° bin of \|θ_CM − 90°\| | −0.9 to +2.0 | −0.005 to +0.019 | −1.2 to +1.4 | −0.000 to +0.002 |

- **The fragment tracks are nearly the same at either mass unit.** A fragment's lab kinetic energy is the same at either mass unit to 10⁻⁵ (the same event gives 1141.86 MeV in both). The simulation derives the beam velocity from the beam's kinetic energy at the window, and the fragment velocities in the center-of-mass frame from the total kinetic energy. Both therefore scale as 1/√(mass unit), every lab velocity scales the same way, and ½mv² does not change. The energy-loss tables depend only on the kinetic energy.
- **Pseudo-data simulated at 939.0 MeV per mass unit, such as the tuning study's, are therefore unaffected by the choice of mass unit.**

## Systematic shifts

Each variation changes one input of the simulation or the digitization. It either simulates the baseline's 3000 events again with the same seeds, or digitizes the baseline's simulation again. It is fitted with the recommended settings and the baseline's model of the detector and reaction, which uses LISE stopping powers. Where the fit can also take the changed input, a second variation puts it in both the pseudo-data and the fit. Comparing the two separates the effect of the mismatch between pseudo-data and fit from the effect of the changed input itself.

Each change is taken event by event against the baseline's recommended fit; most space-charge variations are compared with a space-charge sample instead (see [Space charge of the beam](#space-charge-of-the-beam-probes-a1a4)). In each changed simulation, at least 98.9% of the events have the baseline's Z split and decay angle. In the others, a changed track makes the simulation redraw an event, and the redraw shifts every later random draw. Pairing events therefore compares the same reaction in nearly every event. The exception is the sample with a spread in the total kinetic energy: it draws one more random number per event, so its events differ from the baseline's. It is compared as an independent sample, and each change is the difference between the two samples' values. For paired samples, Δ mean dZ within 2 is the mean change of dZ over the events whose fits are within 2 of the true Z in both samples; for independent samples, it is the difference of each sample's mean dZ within 2.

### Stopping-power model and scale (probes B1, B2)

The fit gets Z from the shape of each fragment's charge along its track (its Bragg curve), with one amplitude shared by both fragments. It therefore depends on how the stopping power changes with the fragment's energy and Z. In this report, LISE means the ATIMA 1.2 column of the LISE files (`LISE` in the code); CATIMA reproduces it to 0.1% in dE/dx. The baseline uses LISE, and the beam uses LISE in every variation. SRIM's tables differ from LISE's by up to 13% in dE/dx, by an amount that depends on Z and energy; above 200 MeV they are 5–13% higher. They are computed for a 2.6% lower gas density (1.752 × 10⁻⁴ against 1.7985 × 10⁻⁴ g/cm³). The Ziegler tables are the Ziegler column of the same LISE files, the stopping powers of older SRIM versions. For Z 31–45 they are 5–17% higher than LISE's at 50–200 MeV and within 1.5% of them above 400 MeV; their ranges from 300 and 700 MeV are 3–14% shorter.

| variation | Δ correct (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) |
|---|---|---|---|---|
| SRIM data, LISE fit | −3.0 ± 1.1 | +0.602 ± 0.011 | +0.657 ± 0.027 | +1.2 ± 0.3 |
| LISE data, SRIM fit | +4.5 ± 1.1 | −0.553 ± 0.010 | −0.555 ± 0.026 | +0.2 ± 0.1 |
| SRIM data, SRIM fit | +5.1 ± 0.7 | +0.007 ± 0.009 | +0.041 ± 0.027 | −0.2 ± 0.2 |
| Ziegler data, LISE fit | −4.3 ± 0.5 | +0.055 ± 0.008 | +0.085 ± 0.026 | −0.1 ± 0.2 |
| LISE data, Ziegler fit | −0.2 ± 0.5 | −0.039 ± 0.006 | −0.003 ± 0.021 | −0.1 ± 0.1 |
| Ziegler data, Ziegler fit | −4.8 ± 0.5 | +0.002 ± 0.008 | +0.046 ± 0.025 | −0.1 ± 0.2 |
| LISE dE/dx × 0.95 | +1.1 ± 0.5 | −0.014 ± 0.006 | +0.000 ± 0.028 | +0.2 ± 0.2 |
| LISE dE/dx × 0.98 | −0.1 ± 0.4 | −0.001 ± 0.005 | +0.028 ± 0.023 | +0.1 ± 0.2 |
| LISE dE/dx × 1.02 | −0.2 ± 0.4 | +0.004 ± 0.005 | +0.038 ± 0.026 | −0.1 ± 0.2 |
| LISE dE/dx × 1.05 | −0.4 ± 0.5 | +0.013 ± 0.006 | +0.047 ± 0.023 | −0.2 ± 0.2 |

The folding angle changes by at most 0.003°. It comes from the Y-pattern, which the stopping power does not move.

- **A stopping-power model in the fit that differs from the pseudo-data's shifts Z by about the same amount at every decay angle.** In each 2° band of θ_CM − 90° within 10° of 90°, the mean dZ within 2 shifts by +0.43 to +0.75 Z for SRIM data fitted with LISE, and by −0.36 to −0.79 Z for LISE data fitted with SRIM. The effect of the mismatch alone, SRIM data fitted with LISE minus the same data fitted with SRIM, is +0.596 ± 0.010 Z in the mean dZ within 2. The change in the fraction with the correct Z nevertheless depends on the angle, because the baseline fits (LISE data fitted with LISE) are themselves off by an amount that depends on the angle (see [Fitted Z against the decay angle](#fitted-z-against-the-decay-angle)). From 4° to 10° below 90°, the baseline fits give too high a Z (mean dZ within 2 from −0.77 to −1.18). SRIM data fitted with LISE come out about 0.6 Z lower, so 42–49 pp more of them have the correct Z. Within 4° of 90°, where the baseline fits are mostly correct, the same shift lowers the fraction with the correct Z by 9–60 pp.
- **The shift is larger for asymmetric splits.** Within 4° of 90°:

  | true light-fragment Z | 31 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 41 | 42 |
  |---|---|---|---|---|---|---|---|---|---|---|---|---|
  | SRIM data, LISE fit | +0.91 | +1.23 | +0.73 | +0.69 | +0.66 | +0.67 | +0.44 | +0.43 | +0.36 | +0.29 | +0.84 | +0.15 |
  | LISE data, SRIM fit | −0.98 | −0.89 | −0.69 | −0.64 | −0.61 | −0.61 | −0.59 | −0.37 | −0.40 | −0.58 | −0.36 | −0.13 |

  The values are changes in the mean dZ within 2; their errors are 0.04–0.06 Z. In a near-symmetric split both fragments have similar Z and velocity, so a change of model changes both Bragg curves alike. The fit keeps the two Z adding up to 85, so the fitted Z moves little. In an asymmetric split the two fragments sample different parts of the stopping-power curves.
- **The Ziegler stopping powers shift Z ten times less than SRIM's, because they differ from LISE's mainly where the fragments are slow.** Ziegler data fitted with LISE shift the mean dZ within 2 by +0.055 ± 0.008 Z, and LISE data fitted with Ziegler by −0.039 ± 0.006 Z. The mismatch alone, Ziegler data fitted with LISE minus the same data fitted with Ziegler, is +0.053 ± 0.006 Z. Ziegler differs from LISE below 200 MeV, which a fragment reaches only on a long track. Ziegler data fitted with LISE therefore shift the mean dZ within 2 by +0.09 to +0.13 Z for vertices 100–400 mm from the window, and by +0.01 to +0.04 Z for vertices 400–700 mm from it, closer to the pad plane. SRIM differs from LISE at high energies too, and the shift of SRIM data fitted with LISE falls less steeply with depth, from +0.75 Z at 100–200 mm to +0.35 Z at 600–700 mm.
- **The decay-angle dependence of the fitted Z depends on the stopping-power model.** Ziegler data fitted with Ziegler, against LISE data fitted with LISE, shift the mean dZ within 2 by −0.30 ± 0.03 Z at 8–10° below 90° and +0.35 ± 0.03 Z at 8–10° above it, with the same mean overall (+0.002 ± 0.008 Z): with Ziegler stopping powers, the fitted Z changes faster with the decay angle.
- **The shared amplitude absorbs a stopping-power scale common to both fragments.** A uniform change of dE/dx changes the size of every charge curve, which the shared amplitude takes up, and the length of each track. The length is seen only where a fragment stops in the gas. For vertices within 200 mm of the window, dE/dx × 0.95 and × 1.05 shift the mean dZ within 2 by −0.05 ± 0.02 and +0.05 ± 0.02 Z; for deeper vertices, by at most 0.04 Z. A difference in overall scale between two models therefore moves Z little. A difference in how dE/dx changes with energy and Z moves it.

### Space charge of the beam (probes A1–A4)

The beam ionizes the gas, and the positive ions drift slowly toward the window. Their charge pulls the drifting electrons of the fragment tracks toward the beam. The ions accumulate toward the window, so the line charge grows linearly from 0 at the pad plane to λ at the window. An electron's squared distance from the beam then shrinks by an amount proportional to the square of its drift distance. At λ = 1.54 × 10⁻⁸ C/m, charge that starts 800 mm from the pad plane and within 50 mm of the beam ends on the beam line.

The experimental data are corrected with this model (`AtLineChargeModel`). The correction takes λ from the beam rate and assumes a beam line from (0, −6) mm at the pad plane to (10, 0) mm at the window. In the experimental data λ spans about 0.5 to 2 × 10⁻⁸ C/m (1500–5000 beam particles per second). The Y-pattern is found in the corrected hits. The fit simulates the same space charge, with the same λ, and compares its charge with the uncorrected hits on the same pads. The pseudo-data here are made and analyzed the same way.

Most variations are compared with one space-charge sample, called the λ = 1.54 × 10⁻⁸ C/m sample below. Its pseudo-data have a line charge of λ = 1.54 × 10⁻⁸ C/m along the beam line the correction assumes, and the correction and the fit use the same λ. This sample, and the same pseudo-data analyzed without any correction, against the baseline (no space charge):

| variation | Δ correct (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) | Δ folding angle (deg) |
|---|---|---|---|---|---|
| λ = 1.54 × 10⁻⁸ C/m, corrected | −0.2 ± 0.5 | +0.005 ± 0.006 | +0.053 ± 0.035 | −0.8 ± 0.3 | +0.029 ± 0.003 |
| the same pseudo-data, no correction, no space charge in the fit | −1.1 ± 1.0 | +0.095 ± 0.014 | +0.166 ± 0.034 | +0.9 ± 0.3 | +1.456 ± 0.023 |

The other variations against the λ = 1.54 × 10⁻⁸ C/m sample. Each row changes the pseudo-data. The correction and the fit keep λ = 1.54 × 10⁻⁸ C/m and the assumed beam line unless the row says otherwise:

| variation of the pseudo-data | Δ correct (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) | Δ folding angle (deg) |
|---|---|---|---|---|---|
| λ / 2 | +0.2 ± 0.6 | −0.105 ± 0.008 | −0.307 ± 0.048 | +1.1 ± 0.2 | −0.705 ± 0.007 |
| λ / 2, corrected and fitted with λ / 2 | −0.8 ± 0.5 | −0.005 ± 0.006 | −0.055 ± 0.032 | +1.1 ± 0.2 | −0.013 ± 0.003 |
| 2λ | −1.0 ± 0.9 | +0.165 ± 0.011 | +0.252 ± 0.026 | +0.1 ± 0.2 | +1.218 ± 0.006 |
| 2λ, corrected and fitted with 2λ | −0.1 ± 0.5 | +0.004 ± 0.006 | +0.017 ± 0.031 | −0.3 ± 0.2 | +0.023 ± 0.002 |
| line charge, electrons' drift integrated step by step | −0.8 ± 0.5 | −0.007 ± 0.006 | −0.133 ± 0.044 | +1.6 ± 0.3 | +0.006 ± 0.017 |
| charge spread uniformly over the 20 mm beam radius | −0.7 ± 0.5 | +0.003 ± 0.006 | −0.175 ± 0.045 | +1.7 ± 0.3 | +0.068 ± 0.028 |
| the same total charge, constant from pad plane to window | −1.5 ± 0.8 | +0.146 ± 0.010 | −0.010 ± 0.045 | +1.8 ± 0.3 | +1.835 ± 0.020 |
| beam line moved 5 mm in x | −0.7 ± 0.5 | −0.003 ± 0.006 | −0.068 ± 0.031 | +0.3 ± 0.2 | −0.003 ± 0.004 |
| beam line moved 5 mm in y | −0.3 ± 0.5 | −0.012 ± 0.006 | −0.138 ± 0.045 | +1.0 ± 0.3 | −0.006 ± 0.008 |
| beam line on the simulated beam's mean track | +0.4 ± 0.5 | −0.015 ± 0.007 | −0.036 ± 0.031 | +0.5 ± 0.2 | +0.001 ± 0.005 |

The simulated beam's mean track runs from (−0.31, −9.04) mm at the window to (18.29, −3.34) mm at the pad plane, 10–18 mm from the line the correction assumes.

In both tables the two means differ where a variation adds fits off by 3 or more. These fits come mostly from vertices within 100 mm of the window, where the variations add up to 11 pp of fits off by 3 or more. Beyond 100 mm they add at most 0.5 pp, except the constant charge, which adds 1.4 pp at 500–600 mm and 4.2 pp at 600–700 mm. The mean dZ within 2 leaves these fits out and follows the bulk of the fits; the mean dZ of all fits includes them.

- **Correcting with the pseudo-data's own λ removes the space charge's effect on Z.** Against the baseline, the mean dZ within 2 changes by +0.005 ± 0.006 Z and the folding angle by only 0.03°. Without a correction, the folding angle is 1.46° larger, and 83.9% of the fits are within one Z, against 92.8% without space charge.
- **An error in λ shifts the mean dZ within 2 by +0.117 ± 0.006 Z per 10⁻⁸ C/m, through the folding angle.** The space charge distorts each fragment track most near the vertex, the part of the track farthest from the pad plane, and least near the pad plane. Too little correction leaves the start of each fragment track pulled toward the beam, which makes the track steeper and the folding angle larger. The fit infers the beam velocity, and from it the fragment velocities, from the folding angle. The λ / 2 and 2λ pseudo-data both shift the mean dZ within 2 by 0.14–0.15 Z per degree of folding-angle change. A straight line through these two shifts, and through zero at the λ = 1.54 × 10⁻⁸ C/m sample, has a slope of +0.117 ± 0.006 Z per 10⁻⁸ C/m of error in λ. When the pseudo-data, the correction and the fit all use λ / 2, or all use 2λ, the mean dZ within 2 does not move. Only an error in λ shifts Z; its value does not.
- **The shift from an error in λ has the same sign at every decay angle and is larger for asymmetric splits.** In 2° bands within 10° of 90°, λ / 2 shifts the mean dZ within 2 by −0.02 to −0.23 Z and 2λ by +0.05 to +0.30 Z. Within 4° of 90°, 2λ shifts the mean dZ within 2 by +0.21 to +0.36 Z for a light-fragment Z of 31–36, and by −0.04 to +0.11 Z for Z = 40–42.
- **A correction twice as strong as the pseudo-data's space charge makes events near the window impossible to fit.** With λ / 2 in the pseudo-data and λ = 1.54 × 10⁻⁸ C/m in the correction and the fit, 136 of the 151 events within 20 mm of the window and 33 of the 83 at 20–40 mm are unfittable: no simulation of the fit puts charge on a pad that has a hit in the event. Beyond 40 mm, no event is unfittable. The tables above leave these events out of every measure.
- **The distribution of the charge along the beam matters: the same total charge spread evenly from the pad plane to the window shifts the mean dZ within 2 by +0.146 Z.** The squared distance of drifting charge from the beam shrinks in proportion to the line charge integrated along its drift. With λ / 2 everywhere instead of a charge growing linearly to λ at the window, charge that starts a distance d from the pad plane moves L/d times more than the correction assumes (L = 1 m): the same at the window, more everywhere else. The correction leaves the tracks pulled toward the beam, the folding angle grows by 1.8°, and the mean dZ within 2 shifts by +0.146 ± 0.010 Z. For vertices 100–600 mm from the window, the mean dZ of all fits also shifts, by +0.09 to +0.16 Z. Over the whole sample the mean dZ of all fits does not move (−0.010 ± 0.045 Z): within 100 mm of the window, 7 pp more fits are off by 3 or more, and their dZ has the other sign.
- **The radial shape of the charge and the position of the beam line move the mean dZ within 2 by at most 0.015 Z, but add fits off by 3 or more for vertices near the window.** Spreading the charge over the 20 mm beam radius changes only the charge that ends close to the beam, and so does integrating the electrons' drift step by step, which stops them at the beam radius instead of on the beam line. Every pad within 30 mm of the beam is a low-gain pad, which the fit leaves out, as are 175 of the 213 pads at 30–40 mm. These two variations move the mean dZ within 2 by at most 0.007 Z. A beam line 5 mm from the one the correction assumes, or on the simulated beam's mean track 10–18 mm from it, moves it by at most 0.015 Z. For vertices within 100 mm of the window, these five variations add fits off by 3 or more: 10 pp for the step-by-step drift, 11 pp for the charge spread over the beam radius, 7 pp for the beam line moved 5 mm in y, and 2 pp for the beam line moved 5 mm in x or onto the simulated beam's mean track. These fits move the mean dZ of all fits of the whole sample by −0.04 to −0.18 Z. For vertices beyond 100 mm, the mean dZ of all fits changes by at most 0.13 Z in any 100 mm band of vertex depth, and mostly by 0.02 Z or less.

### Kinematics of the reaction (probes C2, C3, C5)

The fit assumes the total kinetic energy (TKE) of Viola systematics and the mass number A = round(204 Z / 85) for each fragment. It takes the fragment directions and the vertex from the Y-pattern and infers the beam velocity from the folding angle. Each variation changes one property of the reaction in the pseudo-data only: the TKE (probe C2), the fragments' mass numbers (C3), or the beam's spot and direction (C5). The fit keeps its assumptions. The changes against the baseline:

| variation of the pseudo-data | Δ correct (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) | Δ folding angle (deg) |
|---|---|---|---|---|---|
| TKE × 0.98 | +0.5 ± 0.5 | −0.023 ± 0.006 | +0.025 ± 0.026 | −0.3 ± 0.2 | −0.300 ± 0.002 |
| TKE × 1.02 | −0.8 ± 0.5 | +0.021 ± 0.006 | +0.040 ± 0.025 | −0.3 ± 0.2 | +0.300 ± 0.002 |
| TKE spread, σ = 8% (compared as independent samples) | −0.3 ± 1.2 | +0.007 ± 0.024 | +0.024 ± 0.047 | +0.2 ± 0.3 | +0.01 ± 0.11 |
| light fragment's A − 1, heavy A + 1 | −0.1 ± 0.5 | −0.007 ± 0.006 | +0.015 ± 0.023 | −0.1 ± 0.2 | +0.040 ± 0.002 |
| light fragment's A + 1, heavy A − 1 | −0.8 ± 0.5 | +0.016 ± 0.006 | +0.079 ± 0.028 | −0.2 ± 0.2 | −0.037 ± 0.002 |
| beam spot spread × 2 | −0.9 ± 0.5 | −0.002 ± 0.007 | +0.024 ± 0.038 | +1.2 ± 0.3 | −0.036 ± 0.017 |
| beam angular spread × 3 (2.2 to 6.6 mrad) | −0.1 ± 0.5 | +0.009 ± 0.007 | +0.036 ± 0.029 | +0.1 ± 0.2 | −0.001 ± 0.003 |
| beam's mean direction tilted 10 mrad in x | −0.7 ± 0.5 | −0.005 ± 0.007 | +0.007 ± 0.027 | +0.3 ± 0.2 | −0.006 ± 0.005 |

- **A TKE 2% above or below Viola's shifts the mean dZ within 2 by 0.011 ± 0.002 Z per percent of TKE.** A 2% higher TKE gives the fragments larger velocities in the center-of-mass frame. They leave the beam at larger angles, and the folding angle grows by 0.30°. The fit sets each fragment's lab momentum to the transverse momentum of a 90° decay at Viola's TKE, divided by the sine of the fragment's angle to the beam direction (`AtMCFission::SetMomMagnitude`; the directions come from the Y-pattern). A TKE 2% above Viola's therefore makes the fit's fragment momenta 1% and energies 2% too low, whatever the folding angle. The shift of the mean dZ within 2 is about the same at every decay angle: in 2° bands of θ_CM − 90° it is −0.05 to +0.01 Z for TKE × 0.98 and −0.02 to +0.07 Z for TKE × 1.02. For each true Z, within 4° of 90°, it is −0.11 to +0.09 Z for TKE × 0.98 and −0.03 to +0.08 Z for TKE × 1.02, with errors of 0.02–0.06 Z.
- **A spread in the TKE with σ = 8% widens the distribution of dZ a little and does not shift it.** Against the baseline, 90.5% of the fits are within one Z instead of 92.8%, and the spread of dZ within 2 is 0.95 Z instead of 0.91 Z. The mean dZ within 2 changes by +0.007 ± 0.024 Z.
- **Mass numbers one unit away from the fit's assumption shift the mean dZ within 2 by at most 0.016 Z.** In each 2° band of θ_CM − 90°, and for each true Z within 4° of 90°, the shift is at most 0.09 Z.
- **The beam spot and direction shift the mean dZ within 2 by at most 0.009 Z.** The fit takes the fragment directions and the vertex from each event's Y-pattern, not from an assumed beam. Doubling the beam spot adds 1.2 ± 0.3 pp of fits off by 3 or more.

### Dead pads, pad gains and electronics noise (probes D1, D3, D4)

The pads are read out by AGET chips of 64 pads each, four to an AsAd board (256 pads), and the AsAds by CoBo boards. Each variation digitizes the baseline's simulation again, with the baseline's random seed and one change to the pads or the electronics. The fit does not know of the change: it compares charge only on pads with a hit in the data, and it assumes that every pad has the same gain and no noise. Each variation is paired event by event with the baseline. Several of these changes widen the distribution of dZ, which pulls the mean dZ within 2 toward zero in every angle band where the baseline's mean is off zero (see [What is measured](#what-is-measured)). The table therefore also gives the mean dZ of all fits, and the change in the fraction within one Z.

| variation of the digitization | Δ correct (pp) | Δ within 1 (pp) | Δ off ≥ 3 (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) |
|---|---|---|---|---|---|
| dead AsAd (CoBo 2, AsAd 3) | −1.1 ± 0.5 | −3.8 | +3.1 ± 0.4 | −0.016 ± 0.007 | −0.173 ± 0.045 |
| dead AsAd + 2% of the pads, scattered | −1.7 ± 0.5 | −3.9 | +3.1 ± 0.4 | −0.012 ± 0.008 | −0.170 ± 0.041 |
| dead AsAd + 10% of the pads, scattered | −2.0 ± 0.5 | −4.3 | +3.4 ± 0.4 | −0.018 ± 0.007 | −0.196 ± 0.041 |
| dead AsAd + 10% of the pads, in patches of 16 | −1.8 ± 0.6 | −4.8 | +3.6 ± 0.4 | −0.023 ± 0.008 | −0.187 ± 0.044 |
| gain per pad, σ = 5% | +0.2 ± 0.6 | −1.3 | +0.2 ± 0.2 | +0.012 ± 0.007 | +0.025 ± 0.024 |
| gain per pad, σ = 10% | +1.1 ± 0.7 | −4.4 | −0.0 ± 0.2 | +0.010 ± 0.010 | +0.004 ± 0.027 |
| gain per AGET, σ = 5% | +1.0 ± 0.8 | −6.0 | −0.0 ± 0.2 | −0.016 ± 0.011 | +0.010 ± 0.030 |
| gain per AGET, σ = 10% | −2.1 ± 1.1 | −15.0 | +2.6 ± 0.3 | −0.015 ± 0.016 | −0.015 ± 0.032 |
| gain per AsAd, σ = 5% | −1.2 ± 1.0 | −13.4 | +1.8 ± 0.3 | −0.011 ± 0.015 | +0.003 ± 0.027 |
| gain per AsAd, σ = 10% | −8.2 ± 1.1 | −30.7 | +13.7 ± 0.6 | −0.041 ± 0.024 | −0.050 ± 0.036 |
| noise, σ = 5 ADC | +0.2 ± 0.7 | −3.9 | −0.2 ± 0.2 | +0.198 ± 0.008 | +0.250 ± 0.030 |
| noise, σ = 10 ADC | −1.2 ± 1.0 | −13.7 | +2.2 ± 0.4 | +0.418 ± 0.012 | +0.463 ± 0.042 |

Δ within 1 is the difference of the two samples' fractions within one Z (errors about 0.7 pp). The further dead pads are a fraction of the pads the fit uses. The three samples with further dead pads not shown (5% scattered, and 2% and 5% in patches of 16) lie between the rows shown: Δ off ≥ 3 of +3.3 to +3.5 pp and Δ mean dZ within 2 of −0.016 to −0.014 Z.

- **Dead pads change the fit only in events that lose many of their pads, as the dead AsAd does to 11% of the events.** The dead AsAd's 256 pads form one contiguous region. The events are sorted by the fraction of their fragments' pads (in the baseline's hits) that are dead:

  | dead pads | fraction of the fragments' pads dead | events | off ≥ 3, all pads working | off ≥ 3, with the dead pads | within 1, with the dead pads |
  |---|---|---|---|---|---|
  | the AsAd | none | 2407 | 1.8% | 1.8% | 93.0% |
  | the AsAd | below 10% | 270 | 1.9% | 1.9% | 93.7% |
  | the AsAd | 10% or more | 323 | 1.2% | 31.6% | 53.4% |

  With the dead AsAd alone, only the 323 events with 10% or more of their fragments' pads on it change: 31.6% of them are fitted 3 or more Z off, against 1.2% with every pad working. Further dead pads, scattered or in patches, put a dead pad in almost every event (98% of the events with 2% scattered, 96% with 10% in patches) without hurting the fit. Events with less than 10% of their pads dead change by at most 0.02 Z in the mean dZ within 2 and by at most 0.3 pp in the fraction off by 3 or more. The events with 10% or more of their pads dead, which include all the events with 10% or more of their pads on the dead AsAd, carry the whole effect.
- **The fit fails when a large dead region removes a long part of a fragment's charge curve.** The fit compares the simulated charge only on pads with a data hit, so a dead pad drops out of the comparison. A large dead region, however, removes a long part of one fragment's charge curve, and the fit of such an event often ends 3 or more Z off (the 31.6% above). These events lower the mean dZ of all fits of the whole sample by 0.17 Z.
- **Gain factors common to an AsAd or an AGET widen the distribution of dZ; the mean dZ of all fits moves by at most 0.05 Z.** A gain error common to an AsAd (256 pads) or an AGET (64 pads) makes a step in a fragment's charge curve where its track crosses into another AsAd or AGET, as a change of stopping power would. Independent factors per pad partly average out over the many pads in each time bucket: σ = 10% per pad costs 4.4 pp of fits within one Z, against 15 pp for σ = 10% per AGET. Each sample draws its gain factors once (about 40 AsAds), so the sizes in the table hold for that one draw.
- **Noise raises dZ, most for asymmetric splits.** The fit's simulation has no noise. With noise on the pseudo-data, the fit gives the light fragment a lower Z. At σ = 5 ADC, the mean dZ of all fits shifts by +0.06 to +0.34 Z in the 2° bands of θ_CM − 90°. Within 4° of 90°, the mean dZ within 2 shifts by +0.26 to +0.35 Z for a light-fragment Z of 31–35 and by +0.05 to +0.08 Z for Z = 40–42. The shift roughly doubles from 5 to 10 ADC. Its cause is not traced here. The noise is added only to traces with charge: pads without a simulated signal have no trace, and so no noise.

#### The measured fragment-Z distribution

The distribution of fission fragments in Z is measured from the split each fit gives: the lighter of the two fitted Z, whichever track it is on. A fit that exchanges the two fragments' Z therefore does not change it. `split_distribution.py` compares that distribution between two fits of the same events. The true light Z is uniform over 31–42 (mean 36.44, standard deviation 3.49); with every pad working, the fitted light Z has mean 36.37 and standard deviation 3.58.

| variation | mean fitted light Z (change, event by event) | standard deviation of the fitted light Z | fitted light Z below 31 |
|---|---|---|---|
| every pad working | 36.37 | 3.58 | 3.6% |
| dead AsAd (CoBo 2, AsAd 3) | −0.031 ± 0.026 | 3.63 | 4.1% |
| dead AsAd, events with 10% or more of their pads on it (323) | −0.31 ± 0.21 | 4.03 (3.58 with every pad working) | 10.8% (5.9%) |
| dead AsAd + 10% of the pads the fit uses, in patches of 16 | −0.014 ± 0.029 | 3.66 | 4.3% |
| gain per AsAd, σ = 10% | −0.035 ± 0.031 | 3.67 | 5.6% |
| noise, σ = 5 ADC | −0.203 ± 0.017 | 3.70 | 5.6% |

- **Dead pads broaden the measured distribution; they do not shift it measurably.** Over the whole sample, the dead AsAd changes the mean fitted light Z by −0.031 ± 0.026 and widens the distribution from 3.58 to 3.63, which a Gaussian smearing of 0.6 Z would also do. For the events with 10% or more of their pads on the AsAd, the widening corresponds to a smearing of 1.9 Z, and the mean moves by −0.31 ± 0.21, toward more asymmetric splits, not significantly.
- **The broadening shows up on the asymmetric side.** The measured split is folded at the symmetric split (42.5): a fit that overshoots symmetry comes back as a lighter fragment. Fitted light Z of 26–29, where no true event is, hold 35 events with the dead AsAd against 13 with every pad working. In a measured yield that falls toward asymmetric splits, such a tail adds false yield where the true yield is smallest.
- **Noise, unlike dead pads and gain offsets, shifts the measured distribution.** At 5 ADC the mean fitted light Z falls by 0.203 ± 0.017: the fragments look more asymmetric. Gain offsets of 10% per AsAd broaden it about as much as the dead AsAd and do not shift it.

#### Asymmetric and symmetric fission

The pseudo-data have a flat distribution of the light fragment's Z (about 250 events at each Z from 31 to 42), so the fit's response to each true Z is measured. Weighting the events by a chosen true distribution gives the distribution the fit would measure for it, with every pad working and with each variation, on the same events (`shape_response.py`; errors from 200 bootstrap resamples of the events). Two shapes stand for asymmetric fission, with two Gaussian peaks placed symmetrically about the symmetric split; one stands for symmetric fission, with one Gaussian at Z = 42.5. The measured light Z is the lighter fitted Z, so the full fragment distribution is it mirrored about 42.5. All decay angles within 10° of 90° are used.

| true distribution | variation | change of the mean measured light Z (peak position) | change of its standard deviation (peak width) | change of the height at Z = 42 over the peak height (valley) |
|---|---|---|---|---|
| peaks at Z 36 and 49, σ = 2 (measured width 2.27 with every pad working) | dead AsAd | +0.002 ± 0.030 | +0.149 ± 0.041 | +0.014 ± 0.014 |
| | dead AsAd + 10% of pads in patches | +0.051 ± 0.037 | +0.203 ± 0.039 | +0.035 ± 0.015 |
| | gain per AsAd, σ = 10% | +0.043 ± 0.043 | +0.388 ± 0.035 | +0.075 ± 0.018 |
| | noise, σ = 5 ADC | −0.210 ± 0.024 | +0.043 ± 0.023 | −0.010 ± 0.010 |
| peaks at Z 34 and 51, σ = 1.5 (measured width 1.85) | dead AsAd | +0.074 ± 0.041 | +0.238 ± 0.058 | +0.008 ± 0.011 |
| | noise, σ = 5 ADC | −0.276 ± 0.025 | +0.042 ± 0.043 | −0.006 ± 0.009 |
| one peak at Z 42.5, σ = 3 (measured width of the light half 1.92) | dead AsAd | −0.172 ± 0.042 | +0.349 ± 0.083 | — |
| | noise, σ = 5 ADC | −0.065 ± 0.017 | +0.045 ± 0.020 | — |

- **For asymmetric fission, dead pads widen the peaks and leave them in place.** With the dead AsAd, each peak of the 36/49 distribution stays within 0.03 Z of where it is with every pad working and widens from 2.27 to 2.42, as an extra Gaussian smearing of about 0.8 Z would. The valley at the symmetric split fills by 1.4 ± 1.4% of the peak height. Further dead pads up to 10% in patches widen the peaks a little more (to 2.47) and fill the valley by 3.5 ± 1.5%. The distribution does not look more or less asymmetric in the position of its peaks; its peaks are broader and its valley slightly shallower.
- **For symmetric fission, dead pads widen the one peak.** The measured light half of the peak widens from 1.92 to 2.27 and its mean falls by 0.17 ± 0.04 Z: in the full distribution, mirrored about 42.5, the symmetric peak is wider, not split.
- **Noise moves asymmetric peaks apart.** At 5 ADC, the light peak moves by −0.21 ± 0.02 Z (36/49) and −0.28 ± 0.03 Z (34/51) and the heavy peak by the same amount the other way; the widths hardly change. Measured with noise the fission looks more asymmetric than it is.
- **Gain offsets per AsAd widen the peaks most and fill the valley.** With σ = 10%, the 36/49 peaks widen to 2.66 and the valley fills by 7.5 ± 1.8% of the peak height.

### Drift velocity and diffusion of the gas (probes A5, A6)

Each variation digitizes the baseline's simulation again with the electrons drifting or diffusing differently from the parameter file. The hits (pulse-shape analysis) and the fit keep the parameter file's drift velocity (0.815 cm/µs) and diffusion coefficients (transverse 0.0038 cm²/µs, longitudinal 0.0055 cm²/µs), as the analysis of the experimental data does. Charge from the window stays at the time bucket the experimental data fix for it (`TBEntrance`, 457), so a different drift velocity moves the pad plane in time. The changes are paired event by event with the baseline.

| variation of the digitization | Δ correct (pp) | Δ mean dZ within 2 (Z) | Δ mean dZ of all fits (Z) | Δ off ≥ 3 (pp) | Δ folding angle (deg) |
|---|---|---|---|---|---|
| drift velocity × 0.98 | −0.8 ± 0.5 | −0.030 ± 0.006 | −0.001 ± 0.025 | +0.0 ± 0.2 | −0.600 ± 0.002 |
| drift velocity × 0.99 | −0.8 ± 0.5 | −0.020 ± 0.007 | −0.007 ± 0.023 | +0.1 ± 0.2 | −0.304 ± 0.002 |
| drift velocity × 1.01 | +0.4 ± 0.5 | +0.024 ± 0.006 | +0.075 ± 0.028 | −0.1 ± 0.2 | +0.325 ± 0.002 |
| drift velocity × 1.02 | +0.9 ± 0.5 | +0.045 ± 0.006 | +0.097 ± 0.026 | −0.2 ± 0.2 | +0.620 ± 0.003 |
| transverse diffusion × 0.8 | +1.1 ± 0.6 | −0.114 ± 0.007 | −0.135 ± 0.027 | −0.5 ± 0.2 | −0.005 ± 0.002 |
| transverse diffusion × 1.2 | −1.1 ± 0.6 | +0.106 ± 0.007 | +0.093 ± 0.028 | +0.6 ± 0.2 | +0.007 ± 0.002 |
| longitudinal diffusion × 0.8 | +0.1 ± 0.4 | −0.036 ± 0.006 | +0.026 ± 0.027 | −0.4 ± 0.2 | +0.003 ± 0.002 |
| longitudinal diffusion × 1.2 | −0.1 ± 0.4 | +0.019 ± 0.005 | +0.035 ± 0.024 | +0.1 ± 0.2 | +0.001 ± 0.002 |

- **A drift velocity higher than assumed compresses the tracks along the beam and widens the folding angle.** Reconstructed with the parameter file's drift velocity and anchored at the window, a point a distance d from the window appears at d times the ratio of the assumed to the true drift velocity. A 1% faster drift therefore makes the tracks 1% shorter along the beam, their angles to it larger, and the folding angle larger by 0.31–0.33°. The mean dZ within 2 moves by 0.020 ± 0.002 Z per percent, with the sign of the folding-angle change: 0.07 Z per degree, half the 0.14–0.15 Z per degree of a space-charge error. In 2° bands of θ_CM − 90°, a 2% change shifts the mean dZ within 2 by −0.07 to +0.01 Z (× 0.98) and −0.01 to +0.11 Z (× 1.02). The mean dZ of all fits shifts by −0.001 ± 0.025 Z (× 0.98) and +0.097 ± 0.026 Z (× 1.02) over the whole sample, the latter mostly from vertices within 100 mm of the window, where × 1.02 moves it by +0.39 Z.
- **Transverse diffusion different from the fit's shifts Z in proportion to the difference, most where the drift is long.** The fit simulates the parameter file's transverse diffusion, so wider charge clouds in the pseudo-data spread each fragment's charge over more pads than the fit expects. The mean dZ within 2 moves by +0.0055 ± 0.0003 Z per percent of the coefficient (−0.114 Z for × 0.8, +0.106 Z for × 1.2). The diffusion grows with the drift time, and so does the shift: for × 1.2, +0.13 to +0.16 Z for vertices within 400 mm of the window and +0.03 Z at 600–700 mm, the vertices closest to the pad plane. Within 4° of 90°, × 1.2 shifts light Z 31–36 by +0.12 to +0.20 Z and Z 40–42 by +0.04 to +0.07 Z. With × 1.2, 89.7% of the fits are within one Z, against 92.8% in the baseline.
- **Longitudinal diffusion 20% off shifts the mean dZ within 2 by at most 0.04 Z.** It spreads the charge along the drift direction, in time, and ± 20% moves the mean dZ within 2 by −0.036 ± 0.006 and +0.019 ± 0.005 Z.

## Code and tools (branch `fit-syst`)

- **Settings used by this study.**
  - `sim.decayAngleMin` and `sim.decayAngleMax` draw θ_CM uniformly between two angles.
  - `sim.massUnit` sets the simulation's MeV per mass unit (default 931.5, the fit's value).
  - `sim.elossScale` multiplies every fragment's dE/dx (LISE tables only).
  - Space charge: `sim.spaceCharge` (`line`, `lineNumeric`, `uniform`, `constant`), `sim.lambda`, `sim.scBeam`; the correction `digi.scCorrection`, `digi.lambda`, `digi.scBeam`; `fit.spaceCharge`.
  - Stopping powers: `sim.eloss` and `fit.eloss` take `LISE`, `ZIEGLER` (the LISE files' Ziegler column) or `SRIM`.
  - Kinematics: `sim.tkeScale`, `sim.tkeSigma`, `sim.massShift`, `sim.beamSpotScale`, `sim.beamDivScale`, `sim.beamTiltX`, `sim.beamTiltY`.
  - Digitization: `digi.deadAsAd`, `digi.deadPads`, `digi.deadPatch`, `digi.deadSeed`; `digi.padGainSigma`, `digi.agetGainSigma`, `digi.asadGainSigma`, `digi.gainSeed`; `digi.noise`; `digi.driftVelocityScale`, `digi.coefTScale`, `digi.coefLScale`; `digi.saveTraces` (0 leaves the traces out of the file).

  See the [settings reference](../MANUAL.md#settings-reference).
- **Space charge in the simulation and the fit.** `AtSimpleSimulation::AddHit` places distorted points at their own depth; `AtLineChargeModel` puts charge that the distortion carries past the beam line on the line, and its constant-in-z mode gives the distortion in the same units as its linear mode; `AtMCFission` sets the space charge once per event, before its threads start. Unit tests `AtSimpleSimulationTest.SpaceChargeKeepsZ`, `LineChargePullsTowardBeam`, `AtLineChargeModelTest.DistortionUnits`.
- **`AtPulse::ApplyNoise`** adds Gaussian noise to each sample. Unit test `AtPulseTest.NoiseIsAdditive`.
- **`AtDigiPar`** has setters for the drift velocity and the diffusion coefficients, which the digitization uses to drift electrons in a gas other than the one the reconstruction assumes.
- **`AtELossTable::SetDensity`** works on tables without range variance, which includes every LISE table. Unit test `SetDensityWithoutRangeVariance`.
- **`dump_fit.C`** writes, besides the fit results, the true vertex depth, the folding angle and vertex depth of the Y-pattern, and the fitted beam energy. Events without a fit are written with their true angle and depth.
- **`syst_report.py`** computes the measures above, grouped by true Z, angle band, side of 90°, depth band, true Z near 90° (`z-near`) or the fraction of dead pads (`dead`), and the changes against a chosen base fit, written to a CSV with `--changes-csv`.
- **`dead_cross.C`** gives, for each event, how many of its fragments' pads (in the baseline's hits) are in a set of dead pads made as the digitization makes it.
- **`syst_study/make_tables.sh`** reruns every comparison of the systematic shifts into `syst_study/changes.csv`; **`syst_study/summary.py`** prints the report's tables from it. **`syst_study/make_figures.py`** makes the figures. **`truth_*.csv`** hold `sim_truth.C`'s output for every simulation, and **`dead/*.csv`** `dead_cross.C`'s for every dead-pad sample.
- **The local shared inputs** (`TPC_SHARED_INFO`) agree with the cluster's, including the beam's LISE table `83_200.txt` (²⁰⁰Bi, prerequisite P6 of the plan).

## Appendix: machines and paths

- Cluster checkouts `/home/faculty/aanthony/fission/adam/syst/code`, `code2`, `code3`, `code4` (remotes `hpc-syst` to `hpc-syst4` in `remotes.ini`): the baseline and the stopping-power scale and SRIM samples on the first, space charge on the second, kinematics, pads and gas on the third, Ziegler on the fourth. Data directory `/home/faculty/aanthony/fission/adam/syst/data`. `throttle.sh` and its log in `/home/faculty/aanthony/fission/adam/syst/`; truth tables in `.../syst/truth/`.
- Fetched results in `fissionStudies/data/remote/hpc-syst*/`.
- The tables of the systematic shifts come from `syst_study/make_tables.sh` and `syst_study/summary.py`, the figures from `./make_figures.py DUMPS figures --changes changes.csv`, run in `syst_study/` (`DUMPS` is `make_tables.sh`'s dump directory). The baseline's tables come from:

  ```
  ./syst_report.py hpc-syst:syst_base:rec@syst_study/truth_syst_base-base.csv \
     hpc-syst:syst_base:rec2@syst_study/truth_syst_base-base.csv \
     hpc-syst:syst_base:ref@syst_study/truth_syst_base-base.csv \
     --base rec --by all --by side --by depth --by z
  ./syst_report.py hpc-syst:syst_base:wide_lo@syst_study/truth_syst_base-wide_lo.csv \
     hpc-syst:syst_base:wide_hi@syst_study/truth_syst_base-wide_hi.csv \
     --by side --angle-edges 10 15 20 25 30
  ./syst_report.py hpc-syst:syst_c4:mu931@syst_study/truth_syst_c4-mu931.csv \
     hpc-syst:syst_c4:mu939@syst_study/truth_syst_c4-mu939.csv \
     --base mu931 --paired --by all --by angle
  ```
