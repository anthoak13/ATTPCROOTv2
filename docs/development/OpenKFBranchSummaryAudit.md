> **Status (2026-04-06): POINT-IN-TIME SNAPSHOT** — Audits the `OpenKFBranchSummary.md` as of a specific point in development. Several discrepancies identified here have since been addressed by the PRA refactor (commits `89d8f071`, `dba73e87`) and subsequent UKF validation work. Body text preserved as-is.

# OpenKF Branch Summary Audit

## Purpose

This document is a critical audit of
[`OpenKFBranchSummary.md`](./OpenKFBranchSummary.md) against the current code at
HEAD. The summary is broadly correct about the branch direction, but it presents
some trends as if they have already converged into a single coherent pipeline.
The code is not fully there yet.

The main conclusion of this audit is:

- the summary is directionally accurate
- the implementation is still partially split across competing code paths
- several important behaviors are still heuristic, duplicated, or not exposed
  through stable interfaces

## Where The Code Differs From The Summary

## 1. There Is Not One Settled Clustering Default

The summary says the branch moved from `r10 d20` to overlapping `r20 d15`, then
to adaptive clustering, which is true as a branch narrative
([OpenKFBranchSummary.md](/home/adam/ATTPCROOTv2/docs/development/OpenKFBranchSummary.md#L148)).
But the current code does not have one single clustering policy.

Current status:

- `AtPRAtask` defaults to `r20 d15`
  ([AtPRAtask.h](/home/adam/ATTPCROOTv2/AtReconstruction/AtPRAtask.h#L62))
- `AtTrackFinderHC` still hardcodes `r10 d20`
  ([AtTrackFinderHC.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtTrackFinderHC.cxx#L157))
- `AtTrackFinderTC` uses the `AtPRAtask` defaults
  ([AtTrackFinderTC.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtTrackFinderTC.cxx#L147))
- `AtFitterUKF` can then re-cluster again with adaptive settings, usually
  `r20 d15`, and `r25 d15` for low-energy tracks
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L214))

Concern:

- the branch summary reads like the code has one canonical clustering policy
- the actual code still has three active clustering regimes depending on the
  entry point and algorithm

## 2. Ownership Of Ordering And Clustering Is Still Split Between PRA And The Fitter

The summary says pattern recognition was pulled closer to fitting needs and that
ordering/selection moved upstream
([OpenKFBranchSummary.md](/home/adam/ATTPCROOTv2/docs/development/OpenKFBranchSummary.md#L159)).
That is only partially true.

Current status:

- PRA owns nearest-neighbor ordering
  ([AtPRA.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtPRA.cxx#L223))
- TC owns vertex-based selection and merging
  ([AtTrackFinderTC.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtTrackFinderTC.cxx#L159))
- `AtFitterUKF` still filters clusters, trims the Bragg end, rejects beam-like
  tracks, and may fully re-cluster and re-order again
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L177),
  [AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L205),
  [AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L214))

Concern:

- there is still no single owner for the pre-fit track object
- the fitter is not just fitting; it is still changing the measurement sequence
- this makes it harder to know whether a result should be attributed to PRA or
  the fitter

## 3. The HC And TC Pattern-Recognition Paths Do Not Represent The Same Pipeline

The summary discusses PRA-level ordering, merging, and beam rejection as if that
is the current pattern-recognition state. That is only true for the TC path.

Current status:

- `AtTrackFinderTC` calls `SelectAndMergeTracks`
  ([AtTrackFinderTC.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtTrackFinderTC.cxx#L161))
- `AtTrackFinderHC` does not call `SelectAndMergeTracks`
  ([AtTrackFinderHC.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPatternRecognition/AtTrackFinderHC.cxx#L157))

Concern:

- the branch summary is accurate for the favored TC-based workflow
- it overstates how universally that logic applies across the codebase

## 4. Diffusion-Aware Cluster Covariance Is Implemented, But Adaptive Re-Clustering Bypasses It

The summary correctly says diffusion parameters were wired from `AtDigiPar` into
clustering
([OpenKFBranchSummary.md](/home/adam/ATTPCROOTv2/docs/development/OpenKFBranchSummary.md#L143)).
That is true in `AtPRAtask`, but not in fitter-local re-clustering.

Current status:

- `AtPRAtask` forwards detector diffusion parameters to the PRA transformer
  ([AtPRAtask.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtPRAtask.cxx#L129))
- `AtTrackTransformer` has parameterized diffusion support
  ([AtTrackTransformer.h](/home/adam/ATTPCROOTv2/AtTools/AtTrackTransformer.h#L14))
- `AtFitterUKF` creates a fresh local `AtTrackTransformer` during adaptive
  re-clustering without calling `SetDiffusionParams`
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L238))

Concern:

- the branch-level story says cluster covariance was made detector-aware
- the adaptive fitter path silently falls back to transformer defaults instead
  of the run's actual detector parameters
- this matters most in the exact part of the pipeline that the branch treats as
  most performance-sensitive

## 5. The Summary Understates How Much The Fitter Mutates Its Input Track

The summary frames `AtFitterUKF` as a fitting stage. In practice it modifies the
input `AtTrack` in place.

Current status:

- cluster positions are converted to lab coordinates in place
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L172))
- clusters are filtered in place
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L177))
- the cluster array may be fully reset and rebuilt during adaptive clustering
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L238))

Concern:

- this is a hidden side effect
- repeated fitting, alternate fitters, or later debug inspection may not see the
  original PRA-produced track anymore
- the code is closer to "fit and rewrite track representation" than the summary
  suggests

## 6. Energy-Loss Scaling Exists In The Core UKF, But Not In The Main Wrapper API

The development docs present `fELossScaleFactor` as a practical mitigation
([UKF.md](/home/adam/ATTPCROOTv2/docs/development/UKF.md#L171)).
That is only partially true in the current integrated pipeline.

Current status:

- `kf::TrackFitterUKF` has `fELossScaleFactor`
  ([TrackFitterUKF.h](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/OpenKF/kalman_filter/TrackFitterUKF.h#L562))
- standalone validation macro `run_ukf_digi.C` can use it directly
  ([run_ukf_digi.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_ukf_digi.C#L78))
- `EventFit::AtFitterUKF` exposes no public setter for it
  ([AtFitterUKF.h](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.h#L48))

Concern:

- one of the branch's most important documented bias mitigations is not available
  through the main FairTask wrapper
- the summary makes this sound more operationally integrated than it is

## 7. The GUI Exposes An E-loss Scale Control That Is Not Wired To The Fitter

This is not just a documentation mismatch. It is a UI correctness issue.

Current status:

- `AtUKFDisplay` creates an `E-loss scale` field
  ([AtUKFDisplay.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtUKFDisplay.cxx#L241))
- it reads the GUI value into `eLossScale`
  ([AtUKFDisplay.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtUKFDisplay.cxx#L649))
- it never applies that value to `AtFitterUKF`
  ([AtUKFDisplay.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtUKFDisplay.cxx#L654))

Concern:

- this is a false affordance
- it suggests that a known important physics knob can be tuned live, but the
  current wrapper API does not support that path

## 8. AtFitterTask Reads `AtDigiPar`, But The Fitter Still Relies On Macro-Level Manual Configuration

The summary implies a more unified pipeline than the current code really
provides.

Current status:

- `AtFitterTask` reads `AtDigiPar`
  ([AtFitterTask.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitterTask.cxx#L95))
- but it does not pass detector geometry information into `AtFitterUKF`
- macros still manually set important fitter state such as `ZPadPlane`,
  measurement sigma, and momentum sigma fraction
  ([run_ukf_only.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_ukf_only.C#L43),
  [run_reco_ukf.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_reco_ukf.C#L76))

Concern:

- the fitter is integrated into the task system, but it is not yet
  parameter-driven in the same way PRA became parameter-driven
- a canonical production configuration still lives in macros, not in the task or
  runtime database

## 9. Per-Cluster Covariance Affects The Filter, But Not The Reported Chi2

The summary says per-cluster covariance support was added
([OpenKFBranchSummary.md](/home/adam/ATTPCROOTv2/docs/development/OpenKFBranchSummary.md#L142)).
That is true for the forward pass, but not for the output metric.

Current status:

- forward filtering can use each cluster covariance
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L367))
- reported chi2 still divides by the fixed `fMeasSigma_mm`
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L526))

Concern:

- if `fUsePerClusterCov` is enabled, the fit and the metadata are no longer using
  the same noise model
- the branch summary implies a cleaner covariance integration than the current
  implementation actually provides

## 10. Several Defaults In Code, Docs, And Macros Still Disagree

This is less about the summary and more about branch maturity.

Examples:

- `AtFitterUKF.h` says energy straggling default is false, but the member default
  is true
  ([AtFitterUKF.h](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.h#L56),
  [AtFitterUKF.h](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.h#L110))
- `UKF.md` lists measurement sigma default as 1.0 mm
  ([UKF.md](/home/adam/ATTPCROOTv2/docs/development/UKF.md#L60))
- active macros commonly use 2.0 mm
  ([run_ukf_only.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_ukf_only.C#L44),
  [run_reco_ukf.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_reco_ukf.C#L77))
- momentum sigma fraction is 0.5 in `run_ukf_only.C` but 0.3 in
  `run_reco_ukf.C`
  ([run_ukf_only.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_ukf_only.C#L45),
  [run_reco_ukf.C](/home/adam/ATTPCROOTv2/macro/Simulation/ATTPC/16C_pp/run_reco_ukf.C#L78))

Concern:

- the summary is intentionally high-level, but the branch still has unresolved
  ambiguity even at the level of "what are the defaults?"

## 11. Vertex Momentum Recovery Is Still A Heuristic, Not A Settled Physics Model

The summary says the branch ended on capped linear back-extrapolation
([OpenKFBranchSummary.md](/home/adam/ATTPCROOTv2/docs/development/OpenKFBranchSummary.md#L191)).
That is true, but the current implementation should still be read as a heuristic.

Current status:

- vertex correction uses a linear path-length estimate from `rXY/sin(theta)`
- it caps that length at 50 mm
- then it adds back `dE/dx * pathLength`
  ([AtFitterUKF.cxx](/home/adam/ATTPCROOTv2/AtReconstruction/AtFitter/AtFitterUKF.cxx#L460))

Concern:

- this is likely the right pragmatic choice for now
- it is not a proof that vertex momentum is now physically robust
- the summary could be read as more final than the code warrants

## Remaining Questions To Fix

## 1. What Is The Canonical Owner Of Pre-Fit Track Conditioning?

Decide whether clustering, ordering, beam rejection, minimum spacing, Bragg-end
trimming, and fragment merging belong to:

- PRA
- `AtFitterUKF`
- or a dedicated track-preparation stage

Right now the answer is "all of the above."

## 2. What Is The Real Canonical Clustering Policy?

Decide and encode one supported answer for production:

- `r10 d20`
- `r20 d15`
- adaptive clustering
- or algorithm-specific defaults

If the answer is algorithm-specific, document that clearly and stop talking
about a single canonical default.

## 3. Should Adaptive Re-Clustering Be Allowed Inside The Fitter At All?

This is the key architecture question.

If yes:

- pass real detector parameters into that path
- expose the policy through setters
- document that fitting mutates the track representation

If no:

- move adaptive clustering into PRA or a pre-fit preparation task

## 4. Should `AtFitterUKF` Expose More Of Its Active Policy?

Important active behaviors are currently hard-coded, not configured:

- adaptive clustering on/off
- minimum cluster spacing
- minimum lab theta
- maximum fit time
- energy-loss scaling

That makes the wrapper less honest than the underlying implementation.

## 5. Should `AtFitterTask` Auto-Configure `AtFitterUKF` From `AtDigiPar`?

`AtFitterTask` already loads runtime parameters, but fitter setup remains
macro-driven. The obvious missing pieces are:

- `ZPadPlane`
- possibly measurement model defaults
- possibly gas/geometry-dependent fitter options

Without that, "pipeline integration" is still partly manual.

## 6. Is The HC Path Still Intended To Be First-Class For OpenKF Workflows?

If yes, it needs the same track-selection/merging semantics as the TC path.
If no, the docs should stop speaking about "PRA" generically when the active
OpenKF logic is really TC-centric.

## 7. What Is The Intended Status Of Per-Cluster Covariance?

Questions still open:

- should it be a production default or a diagnostic option?
- should chi2 be updated to use the same per-cluster covariance model?
- should adaptive re-clustering preserve detector-derived covariance inputs?

At the moment the answer appears to be "supported, but not fully closed."

## 8. Is Energy-Loss Scaling A Supported Physics Control Or Just A Validation Knob?

The current code gives mixed signals:

- core UKF supports it
- docs recommend it
- one standalone macro uses it
- wrapper and GUI do not really support it end-to-end

This should be decided explicitly.

## 9. Is Theta/Phi Conversion Fully Closed Or Only Closed For The Main Proton Case?

The late branch fix is important, but coordinate conventions in this repository
are historically fragile. The remaining question is whether the fix is:

- fully general
- only valid for the current proton-in-16C(p,p) workflow
- or still dependent on assumptions about ordering and Z-flip direction

That deserves an explicit validation note, not just a code change.

## Concerns Beyond Simple Mismatches

## 1. The Code Still Looks Like A Successful Research Branch More Than A Settled Production Pipeline

That is not criticism of the branch value. It is a statement about maturity.

The branch discovered the right problems and implemented many of the right
fixes, but the implementation still contains evidence of active exploration:

- duplicated logic
- heuristic policy inside the fitter
- UI controls that are not wired
- manual macro configuration for important runtime state

## 2. The Governing Summary Is Slightly More Coherent Than The Code

This is normal for a synthesis document, but it creates risk:

- the summary can make the branch seem more architecturally unified than it is
- future readers may assume there is already one canonical pipeline
- then they will be surprised by HC/TC differences, fitter-local clustering, and
  macro-specific settings

## 3. The Next High-Value Step Is Probably Not More Physics Tuning

The next high-value step is likely architectural cleanup:

- choose ownership boundaries
- expose active policy as configuration
- remove duplicated track conditioning logic
- connect detector/runtime parameters into the fitter wrapper
- make reported metadata consistent with the noise model actually used

Only after that will new tuning results be easy to trust and reproduce.
