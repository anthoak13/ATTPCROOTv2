/pa  # Plan: Add Covariance-Method Comparison Without Changing Current Behavior

  ## Summary

  The work should be split into two tracks that stay deliberately separate:

  1. a behavior-preserving refactor of the current covariance path
  2. a new alternate covariance path for comparison

  The baseline must remain byte-for-byte equivalent in behavior as far as practical. No bug fixes, no parameter cleanup, no inheritance cleanup, no “while we are here” improvements. If the current code has
  inconsistencies, they should remain in place during this work and be documented, not corrected. Otherwise the comparison stops being a comparison to the current code.

  The comparison should cover exactly three modes:

  - fixed_sigma: fitter ignores cluster covariance
  - transformer_direct: current behavior, preserved exactly
  - hit_cluster_online: new path where per-hit variance is set from the same detector model and clusters are accumulated through AtHitCluster::AddHit(...)

  ## Implementation Changes

  ### 1. Freeze the current behavior behind characterization tests

  Before introducing any new covariance mode, add tests that lock down the present behavior of the current clustering path.

  These tests should characterize the current implementation, not the intended implementation. They should assert what the code does today, including awkward details.

  Add characterization coverage for:

  - ClusterizeSmooth3D cluster positions and cluster count on deterministic input
  - ClusterizeSmooth3D cluster covariance values on deterministic input
  - ClusterizeByGroup cluster positions and covariance values on deterministic input
  - fitter behavior with SetUsePerClusterCov(false) versus true on a fixed cluster set
  - adaptive re-clustering path using the current local AtTrackTransformer defaults, without “repairing” parameter propagation

  The purpose is to make sure the refactor leaves the current mode unchanged.

  ### 2. Refactor the current transformer code without changing its outputs

  Refactor AtTrackTransformer only enough to isolate the current covariance calculation into a reusable helper.

  Rules for this refactor:

  - preserve the current formulas exactly
  - preserve the current diagonal-only covariance
  - preserve current use of z /= nHits while x,y are charge-weighted
  - preserve current smoothing and midpoint behavior
  - preserve current defaults and fallback parameter values
  - preserve current handling of unset diffusion parameters
  - do not change ordering, filtering, or cluster membership logic

  The helper should be treated as a frozen baseline implementation. Its job is to make the old path testable and reusable, not better.

  ### 3. Add a second covariance-construction mode alongside the baseline

  After the baseline is frozen by tests, add an explicit covariance-construction selector with two clustering-side modes:

  - TransformerDirect
  - HitClusterOnline

  TransformerDirect must call the preserved baseline helper and remain the default.

  HitClusterOnline should:

  - use the same cluster membership produced by the existing clustering logic
  - derive per-hit position variance from the same detector inputs used by the baseline method
  - create an AtHitCluster and feed member hits through AddHit(...)
  - use the resulting AtHitCluster::GetCovMatrix() as the cluster covariance

  Important constraint:

  - do not alter centroid or membership rules in this phase unless required to make AtHitCluster usable
  - if AtHitCluster naturally produces a different internal position estimate, keep that difference confined to the new mode only
  - the baseline path must not be rerouted through AtHitCluster

  ### 4. Keep the detector model identical between baseline and new mode

  Both covariance methods must consume the same detector-model inputs:

  - transverse diffusion coefficient
  - longitudinal diffusion coefficient
  - drift velocity
  - time bucket duration
  - pad transverse resolution
  - whatever longitudinal pad-resolution assumption the current transformer already uses

  Do not “improve” this model for the new method. The new method should differ only in how hit uncertainties are aggregated into a cluster covariance.

  ### 5. Extend the existing scan infrastructure to compare modes

  Use the existing scan-style tests/utilities to compare:

  - fixed_sigma
  - transformer_direct
  - hit_cluster_online

  The comparison should be run on:

  - synthetic truth-driven tracks
  - digitized 16C(p,p) data

  Keep all non-covariance settings fixed within a comparison block:

  - same clustering parameters
  - same fitter settings
  - same straggling choice
  - same momentum seed policy
  - same event selection

  The goal is to isolate covariance-method effects rather than re-open the entire reconstruction tuning problem.

  ## Test Plan

  ### A. Baseline characterization tests

  These are the most important addition.

  Add deterministic tests that lock down the current transformer_direct behavior:

  - exact cluster count for fixed toy inputs
  - exact or tight-tolerance cluster positions
  - exact or tight-tolerance covariance diagonals
  - zero off-diagonal terms
  - unchanged behavior for both ClusterizeSmooth3D and ClusterizeByGroup

  These tests should fail if the refactor accidentally changes present behavior.

  ### B. Baseline fitter-regression tests

  Add tests that confirm the refactor does not change current fitter behavior when using the baseline mode:

  - same convergence status
  - same number of fitted clusters
  - same fitted momentum/kinematics within tight tolerance
  - same chi2 or fit metadata where applicable

  This should be done for:


  Add targeted tests for hit_cluster_online:

  - cluster covariance is produced through AtHitCluster
  - covariance is symmetric and non-negative on the diagonal
  - covariance changes as expected when hit variance or drift distance changes
  - the new mode differs from the baseline only in covariance construction, not because baseline behavior was altered


  Report at minimum:

  - convergence fraction
  - cluster count

  ## Public Interfaces / Config

  Add one covariance-construction selector on the clustering side with two values:

  - TransformerDirect as the default
  - HitClusterOnline as the alternate mode

  Do not add AtHitClusterFull estimators in this phase.

  The fitter-side comparison still uses:

  - SetUsePerClusterCov(false) for fixed_sigma
  - SetUsePerClusterCov(true) for both cluster-covariance modes

  ## Assumptions And Guardrails

  - No bug fixes or cleanup to existing covariance behavior are allowed in this phase.
  - Existing inconsistencies must remain unchanged and, where relevant, be documented in test names or comments.
  - The baseline mode is the current code path, preserved exactly.
  - The new mode exists only as an alternate comparison path behind configuration.
  - Any future cleanup of the baseline should happen only after comparison results are collected and explicitly separated from this work.

## Execution Report

### Dataset and commands used

This comparison was executed on 2026-04-05 in the local ATTPCROOT workspace.

Digitized 16C(p,p) input was generated with:

- `root -l -b -q 'geometry/ATTPC_H300torr.C'`
- `root -l -b -q 'macro/Simulation/ATTPC/16C_pp/C16_pp_sim.C(50)'`
- `root -l -b -q 'macro/Simulation/ATTPC/16C_pp/run_digi_attpc.C(50)'`

The generated files were:

- `macro/Simulation/ATTPC/16C_pp/data/attpcsim.root`
- `macro/Simulation/ATTPC/16C_pp/data/attpcpar.root`
- `macro/Simulation/ATTPC/16C_pp/data/output_digi.root`

The digitized sample contained 50 simulated events. Pattern recognition produced:

- 23 events with one selected track candidate
- 27 events with zero selected tracks

So the covariance comparison on digitized data was run on the same 23 proton-like PRA tracks for all three covariance modes.

### Physics under comparison

The comparison isolates only the measurement-covariance model seen by the UKF. Cluster membership and centroiding stay in the existing `ClusterizeSmooth3D` path.

The detector uncertainty model shared by `transformer_direct` and `hit_cluster_online` is:

- drift time: `t_drift = z_mm / (10 * v_drift)`
- transverse single-hit variance: `sigma_x^2 = sigma_y^2 = padResXY^2 + 100 * coefT * 2 * t_drift`
- longitudinal single-hit variance: `sigma_z^2 = padResZ^2 + sigma_TB^2 + 100 * coefL * 2 * t_drift`
- time-bucket term: `sigma_TB^2 = (v_drift * tbTime * 10)^2 / 12`

The three fitter modes mean:

- `fixed_sigma`: ignore cluster covariance and use the fitter’s fixed isotropic measurement sigma of 2 mm
- `transformer_direct`: use the existing cluster covariance calculation in `AtTrackTransformer`
- `hit_cluster_online`: assign the same per-hit variances as above, then accumulate covariance through `AtHitCluster::AddHit(...)`

Important baseline details preserved intentionally:

- `x` and `y` centroids are charge weighted
- `z` is averaged by hit count rather than charge
- `transformer_direct` covariance is diagonal only
- adaptive and PRA-side clustering defaults were not “cleaned up” during this comparison

The UKF settings used in the digitized comparison were:

- proton mass 938.272 MeV/c^2 in H2 gas with CATIMA energy loss
- `Bz = 2.85 T`
- UKF parameters `(alpha, beta, kappa) = (1e-3, 2.0, 0.0)`
- measurement sigma for `fixed_sigma`: 2 mm
- momentum seed width: 30%
- minimum clusters: 5
- `ZPadPlane = 1000 mm`
- energy straggling disabled in the digitized covariance-mode comparison, matching all three modes

### Synthetic comparison

Synthetic tracks use the same true propagated proton trajectory for all three modes inside each trial, then smear the hit coordinates before clustering. The output below therefore compares covariance construction on matched synthetic realizations rather than on different tracks.

Results from `ClusteringScanTest.CovarianceMethodComparison` over 20 trials:

| Mode | Tried | Converged | Mean momentum error [%] | RMS momentum error [%] | Mean residual [mm] | RMS residual [mm] | Avg clusters | Mean delta momentum vs fixed [%] | Mean delta residual vs fixed [mm] |
|------|------:|----------:|------------------------:|-----------------------:|-------------------:|------------------:|-------------:|---------------------------------:|----------------------------------:|
| fixed_sigma | 20 | 20 | -2.221 | 0.168 | 0.410 | 0.067 | 25.6 | 0.000 | 0.000 |
| transformer_direct | 20 | 20 | -2.380 | 0.190 | 0.328 | 0.044 | 25.6 | -0.160 | -0.082 |
| hit_cluster_online | 20 | 20 | -2.340 | 0.169 | 0.346 | 0.055 | 25.6 | -0.119 | -0.064 |

Interpretation:

- all three modes converged on all 20 synthetic trials
- both covariance-aware modes reduced the mean spatial residual relative to `fixed_sigma`
- `transformer_direct` had the lowest residuals in this synthetic study
- `hit_cluster_online` improved residuals relative to `fixed_sigma`, but not as much as `transformer_direct`
- momentum bias changed only at the few-tenths-of-a-percent level between the covariance-aware modes

This is a useful controlled sanity check, but still not enough by itself to declare a winner.

### Digitized 16C(p,p) comparison

Results from `ClusteringDigiScanTest.CompareCovarianceModes` on the 23 PRA-selected proton-like tracks from the generated digitized sample:

| Mode | Tried | Converged | Good fits | Good fraction [%] | Avg clusters | Avg fitted KE [MeV] | Avg fitted theta [deg] | Mean KE bias [MeV] | RMS KE bias [MeV] | Mean theta bias [deg] | RMS theta bias [deg] |
|------|------:|----------:|----------:|------------------:|-------------:|--------------------:|-----------------------:|-------------------:|------------------:|----------------------:|---------------------:|
| fixed_sigma | 23 | 23 | 3 | 13.0 | 54 | 8.81 | 62.61 | 0.15 | 0.31 | 0.28 | 0.53 |
| transformer_direct | 23 | 23 | 3 | 13.0 | 54 | 8.71 | 62.57 | 0.05 | 0.22 | 0.24 | 0.46 |
| hit_cluster_online | 23 | 23 | 3 | 13.0 | 54 | 8.43 | 21.06 | -0.23 | 0.44 | -41.27 | 28.43 |

Interpretation:

- all three modes converged numerically on every selected event
- the count of kinematically plausible fits stayed low and identical at 3/23
- `fixed_sigma` and `transformer_direct` reconstruct essentially the same recoil-angle physics on this sample
- `transformer_direct` is slightly closer to MC truth than `fixed_sigma` in both KE and theta RMS
- `hit_cluster_online` is the outlier: although it converges, many events collapse into a beam-like or wrong-branch angular solution, pulling the mean fitted theta from about 63 deg down to about 21 deg

That makes the current conclusion straightforward for this branch state:

- `transformer_direct` remains the safest covariance mode
- `hit_cluster_online` is not ready to replace it
- if `hit_cluster_online` is investigated further, it should remain behind the explicit selector and be studied as a research branch, not as a baseline replacement

### Reproducibility notes

The compiled comparison tests now resolve the digitized files through `VMCWORKDIR`, so they run from the normal `build/tests` location instead of silently missing the generated dataset because of a relative-path mistake.

## Failure Analysis and Hypotheses

### Why the modes can differ so much

The observed `hit_cluster_online` failure mode is not best described as “adding a small amount of extra information.” In this branch state it changes the statistical model seen by the UKF in two important ways:

- `transformer_direct` supplies a diagonal covariance tied to the legacy cluster estimator
- `hit_cluster_online` supplies a full covariance matrix from `AtHitCluster::AddHit(...)`, including off-diagonal correlations and a different weighting law

This matters because the UKF does not just read the covariance magnitude. It uses the full measurement covariance in the innovation matrix and Kalman gain. Changing the covariance can therefore rotate the correction direction, not just tighten or loosen it.

There is also a likely consistency problem in the current implementation:

- in `hit_cluster_online`, the cluster position still comes from the legacy transformer centroid
- only the covariance matrix is replaced by the `AtHitCluster` result

So the measurement mean and measurement covariance are not coming from the same estimator in that mode. That mismatch can be harmless for weakly correlated covariances, but it can become important once the off-diagonal terms are appreciable.

### Concrete clues from the current run

The current digitized comparison suggests a branch-selection failure rather than a simple loss of precision:

- `fixed_sigma` and `transformer_direct` both reconstruct the recoil branch near `63 deg`
- `hit_cluster_online` converges numerically on the same events, but the average fitted theta shifts to `21 deg`
- the fitted kinetic energy does not collapse in the same dramatic way, so the failure is not simply “everything diverged”

That pattern is consistent with the filter accepting a different geometric solution while still finding an internally stable state estimate.

The current logs also show repeated covariance regularization diagnostics from the OpenKF layer. Those do not by themselves prove that `hit_cluster_online` is the sole cause, but they are consistent with the hypothesis that the online covariance matrices are often harder for the filter to use robustly.

### Ranked hypotheses

#### H1. Off-diagonal correlations from `AtHitCluster` are destabilizing the measurement update

This is the leading hypothesis.

`transformer_direct` is diagonal-only. `hit_cluster_online` introduces covariance coupling between measured axes. If those correlations are physically inconsistent with the cluster centroid being used, or if they are simply too strong/noisy for this fitter setup, they can rotate the innovation weighting enough to pull the filter onto the wrong angular branch.

Prediction:

- if the online covariance is forced to diagonal-only while keeping its diagonal entries, much of the wrong-theta behavior should disappear

#### H2. The online covariance scale is overconfident in some directions

`AtHitCluster` uses reliability weighting based on inverse hit variance. That can produce variances much smaller than the legacy direct formula in some cluster shapes. If the filter is over-trusting those measurements, it can commit too strongly to an early wrong correction.

Prediction:

- multiplying the online covariance by a global factor larger than 1 should reduce the branch-flip rate if overconfidence is the main problem

#### H3. The measurement mean/covariance pair is internally inconsistent

This is also a strong candidate.

The current `hit_cluster_online` mode keeps the legacy cluster position but swaps in the online covariance. If the `AtHitCluster` covariance describes the uncertainty around a different effective cluster estimator than the legacy centroid, the UKF is being given a statistically inconsistent measurement packet.

Prediction:

- a mode that uses both the online centroid and the online covariance should behave more coherently than the current hybrid mode

#### H4. Real digi clusters expose pathologies that the current synthetic test does not

The synthetic comparison is still relatively clean:

- same propagated truth track in each trial
- simple Gaussian position smearing
- no PRA ambiguities
- no fragment merging mistakes
- no beam/recoil competition in the selected candidate

That is enough for a controlled covariance study, but it does not reproduce the full structure of digitized clusters. The real digi sample includes irregular cluster shapes, varying hit multiplicity, PRA ordering effects, and ambiguous track geometry. Those are exactly the situations where covariance-model differences matter most.

Prediction:

- a more realistic synthetic stress test with anisotropy, ambiguous curvature, larger cluster multiplicities, and noisier seeds should begin to reproduce the online failure mode

#### H5. Positive-definiteness or conditioning problems are present in the online matrices

The OpenKF layer attempts to regularize non-positive-definite matrices. If `hit_cluster_online` frequently produces poorly conditioned covariances or innovation matrices, that can inject extra numerical distortion even if the fit ultimately “converges.”

Prediction:

- event-by-event covariance spectra should show smaller minimum eigenvalues, worse condition numbers, or more frequent regularization when using the online mode

## Next Exploration Steps

The next tests should discriminate between the hypotheses above rather than add more broad reporting.

### 1. Covariance census on the digitized sample

For each matched cluster in the three modes, record:

- covariance diagonal terms
- correlation coefficients
- determinant
- eigenvalues
- condition number
- whether OpenKF regularization was triggered

This directly tests H1 and H5.

### 2. Add `hit_cluster_online_diag_only` as a temporary diagnostic mode

Use the online covariance construction, then zero the off-diagonal elements before handing the matrix to the UKF.

Interpretation:

- if theta returns to the baseline branch, the off-diagonal structure is the main problem
- if the failure remains, the diagonal scale or centroid mismatch is more likely

This is the cleanest test of H1.

### 3. Add an online-centroid consistency diagnostic mode

Construct the cluster fully through `AtHitCluster`, using both:

- the online position estimate
- the online covariance estimate

This should remain a diagnostic mode, not a production switch, until the comparison is understood.

Interpretation:

- if this behaves much better than the current hybrid online mode, then the main issue is the mean/covariance inconsistency described in H3

### 4. Run an online covariance scale scan

Introduce a temporary scale factor applied only to the online covariance:

- `0.5`
- `1.0`
- `2.0`
- `5.0`

Interpretation:

- recovery at larger scale indicates that the online covariance is too aggressive or overconfident
- no recovery suggests that shape/correlation, not overall scale, is the dominant issue

This tests H2.

### 5. Trace one matched failing event cluster-by-cluster

Pick one digitized event where:

- `transformer_direct` reconstructs the expected recoil branch
- `hit_cluster_online` converges to the wrong-theta branch

For each cluster update, record:

- cluster position
- cluster covariance
- predicted state before correction
- corrected state after correction
- innovation magnitude

This should identify the point at which the filter commits to the wrong branch.

### 6. Measure the centroid mismatch explicitly

For each cluster in the digitized sample, compare:

- legacy transformer centroid
- `AtHitCluster` centroid

If this displacement is comparable to or larger than the quoted online uncertainty, then the current hybrid online mode is statistically inconsistent by construction.

This is the most direct test of H3.

### 7. Build a more adversarial synthetic comparison

The current synthetic test is useful but too gentle. Extend it with:

- larger cluster hit multiplicity
- stronger anisotropic diffusion
- higher curvature ambiguity
- noisier initial momentum seed
- intentional track-ordering perturbations

The goal is to identify whether the online failure is inherently tied to digi irregularities or whether it can be reproduced in a controlled synthetic setting.

## Exploration Results: Controlled Digi Hypothesis Tests

I executed the first round of the hypothesis plan on the digitized `16C(p,p)` sample using a more controlled fitter configuration than the earlier comparison table.

Important methodological change:

- for these diagnostic tests, I disabled the fitter's adaptive re-clustering step

Why that matters:

- the original fitter can re-cluster the track internally using the same covariance mode being studied
- that mixes two effects:
  - the measurement set itself changes
  - the UKF update model changes
- for the purpose of diagnosis, we want to hold the cluster sequence fixed and only change what covariance model is handed to the fitter

So the numbers below are not a replacement for the earlier physics-performance table. They are a controlled failure-analysis experiment.

### Diagnostic modes tested

- `transformer_direct`
  - legacy centroid and legacy direct covariance
  - per-cluster covariance enabled in the UKF
- `online_raw`
  - legacy centroid with online covariance
  - covariance summarized only, not fit
- `online_raw_fixed`
  - same clusters as `online_raw`
  - fitter forced back to fixed isotropic measurement sigma
- `online_diag`
  - legacy centroid with online covariance after zeroing off-diagonals
  - covariance summarized only, not fit
- `online_diag_fixed`
  - same clusters as `online_diag`
  - fitter forced back to fixed isotropic measurement sigma
- `online_consistent_fixed`
  - online centroid and online covariance from the same `AtHitCluster` construction
  - fitter forced back to fixed isotropic measurement sigma
- `online_reg`, `online_consistent_reg`, `online_x0.5_reg`, `online_x2_reg`, `online_x5_reg`
  - online covariance with eigenvalue flooring and optional scale factors
  - used to test whether positive-definiteness or global scale alone explains the failure

### Controlled digitized results

Selected proton-like PRA tracks: `23 / 50` events

| mode | conv | avgTh (deg) | dTh (deg) | dKE (MeV) | varX | varY | varZ | \|rXY\| | \|rXZ\| | \|rYZ\| | minEig | cond | dPos (mm) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `transformer_direct` | 23 | 53.77 | -8.56 | 587.19 | 1.68 | 1.68 | 3.42 | 0.00 | 0.00 | 0.00 | 1.68 | 2.03 | 0.00 |
| `online_raw` | 0 | 0.00 | 0.00 | 0.00 | 12.23 | 12.49 | 5.19 | 0.66 | 0.78 | 0.76 | 0.33 | 34476.77 | 0.00 |
| `online_raw_fixed` | 23 | 51.91 | -10.42 | 401.06 | 12.23 | 12.49 | 5.19 | 0.66 | 0.78 | 0.76 | 0.33 | 34476.77 | 0.00 |
| `online_diag` | 0 | 0.00 | 0.00 | 0.00 | 12.23 | 12.49 | 5.19 | 0.00 | 0.00 | 0.00 | 3.34 | 10.54 | 0.00 |
| `online_diag_fixed` | 23 | 51.91 | -10.42 | 401.06 | 12.23 | 12.49 | 5.19 | 0.00 | 0.00 | 0.00 | 3.34 | 10.54 | 0.00 |
| `online_consistent_fixed` | 23 | 53.55 | -8.78 | 582.80 | 12.43 | 12.60 | 5.25 | 0.66 | 0.78 | 0.76 | 0.33 | 34616.69 | 12.78 |
| `online_reg` | 0 | 0.00 | 0.00 | 0.00 | 12.23 | 12.49 | 5.19 | 0.66 | 0.78 | 0.76 | 0.33 | 34476.77 | 0.00 |
| `online_consistent_reg` | 0 | 0.00 | 0.00 | 0.00 | 12.43 | 12.60 | 5.25 | 0.66 | 0.78 | 0.76 | 0.33 | 34616.69 | 0.00 |
| `online_x0.5_reg` | 0 | 0.00 | 0.00 | 0.00 | 6.12 | 6.25 | 2.60 | 0.66 | 0.78 | 0.76 | 0.16 | 34476.77 | 0.00 |
| `online_x2_reg` | 0 | 0.00 | 0.00 | 0.00 | 24.47 | 24.99 | 10.39 | 0.66 | 0.78 | 0.76 | 0.65 | 34476.77 | 0.00 |
| `online_x5_reg` | 0 | 0.00 | 0.00 | 0.00 | 61.16 | 62.46 | 25.97 | 0.66 | 0.78 | 0.76 | 1.63 | 34476.77 | 0.00 |

### What these numbers show

#### 1. The online covariance field is geometrically extreme

Compared with `transformer_direct`, the online covariance on real digi clusters is:

- roughly `7x` larger in `x`
- roughly `7x` larger in `y`
- still larger in `z`
- strongly correlated, with average absolute correlations around `0.66-0.78`
- extremely ill-conditioned, with average condition number around `3.4e4`

This is not a perturbation of the legacy model. It is a qualitatively different measurement-noise geometry.

#### 2. Off-diagonal terms are not the sole cause

`online_diag` removes the correlations entirely, but still gives `0 / 23` converged fits once the UKF is asked to use those per-cluster uncertainties.

That falsifies the simplest version of the "the off-diagonals alone broke it" hypothesis.

The failure therefore survives even when the matrix is reduced to diagonal variances only.

#### 3. Positive-definite regularization and global scale changes do not rescue the fit

I tested:

- eigenvalue flooring (`online_reg`)
- half-scale covariance (`online_x0.5_reg`)
- double-scale covariance (`online_x2_reg`)
- five-times-scale covariance (`online_x5_reg`)

All remained at `0 / 23` converged fits.

So the problem is not explained by:

- a few slightly negative eigenvalues
- a single global overconfidence factor

#### 4. The cluster centroids themselves are not the main failure in the hybrid online mode

`online_raw_fixed` and `online_diag_fixed` both converge on all `23` events when the fitter ignores the per-cluster covariance and returns to the fixed isotropic sigma model.

Those two modes use:

- the same legacy cluster positions
- different covariance matrices

Yet they produce identical fit behavior once the covariance is not consumed by the UKF.

That means:

- the raw cluster sequence is not itself pathological
- the act of supplying the online covariance to the UKF is what kills convergence

#### 5. Mean/covariance inconsistency is real, but it is probably secondary to the covariance-model failure

`online_consistent_fixed` also converges on all `23` events, but the mean cluster position shifts by about `12.8 mm` relative to the legacy-centroid track.

That is a large effect and confirms that the current hybrid online mode is statistically inconsistent:

- the mean comes from the legacy estimator
- the covariance comes from the online estimator

However, because both `online_raw` and `online_diag` already fail before introducing that centroid shift, the dominant failure seems to be the covariance model itself, not just the mean/covariance mismatch.

### Refined interpretation

After this round, the hypotheses rank as follows:

1. **H2 revised: the online variance model is fundamentally incompatible with the current UKF update tuning**

   The diagonal-only failure shows that the problem is not limited to correlations. The online variances themselves, together with their event-by-event spread, are enough to make the fitter fail.

2. **H1 weakened but not eliminated: correlations are probably harmful, but not necessary for failure**

   The raw online covariance is extremely correlated, so correlations likely worsen the problem. But the diagonal ablation shows they are not the only mechanism.

3. **H3 still relevant: the hybrid mean/covariance construction is statistically inconsistent**

   The `12.8 mm` centroid shift in `online_consistent_fixed` is too large to ignore. It remains a real modeling issue, even if it is not the first-order reason the UKF stops converging.

4. **Simple PD repair is not enough**

   Eigenvalue flooring fixes obvious indefiniteness but does not recover convergence. The instability is structural, not just a numerical floor problem.

### Consequences for the next exploration step

The next synthetic study should no longer focus first on correlations. It should be built to test whether the **magnitude and anisotropy of the diagonal variances alone** can reproduce the digitized failure.

The most useful next synthetic tests are now:

- start from the current synthetic benchmark and replace the legacy covariance with:
  - online diagonal variances only
  - scaled-up legacy diagonal variances with the same order of magnitude as the digi online case
  - strongly anisotropic but diagonal covariance matrices
- hold cluster centroids fixed
- compare convergence rate, fitted theta branch, and innovation norms

That will tell us whether the digitized failure can be reproduced without any centroid mismatch and without any off-diagonal terms.

### Caution on the absolute bias values in this section

The absolute `dKE` and `dTheta` values in this controlled table should not be interpreted as the final physics result.

Reason:

- this experiment deliberately disabled adaptive re-clustering to isolate the covariance effect
- that changes the absolute fitter operating point

So the scientifically relevant outputs from this section are:

- convergence vs non-convergence
- covariance scale/conditioning
- whether fixed-sigma control recovers the fit
- whether diagonalization changes the outcome
- whether centroid shifts are large
