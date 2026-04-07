> **Status (2026-04-06): POINT-IN-TIME SNAPSHOT** — Written at a specific point in the branch's development. Several "architectural gaps" noted here have since been resolved: the PRA monolith was refactored into a composable `AtPatternFinder`/`AtPatternTransform` pipeline, and UKF fitting is now validated at ~98% convergence. Body text is preserved as-is.

# OpenKF Branch Governing Summary

## Purpose

This branch grew into something much larger than a fitter feature branch. It
started as an effort to make OpenKF work inside ATTPCROOT, but once that was in
place the center of gravity shifted. The main gains no longer came from the UKF
equations themselves. They came from improving the track presented to the
fitter: clustering, ordering, fragment merging, momentum seeding, vertex
handling, and the final interpretation of the fitted state.

That is the governing lesson of the branch. OpenKF was made usable, and in the
process the real bottlenecks in the reconstruction chain became much easier to
see.

This document summarizes what changed, why it changed, what the branch has
already proved, and what still needs to be cleaned up before this should be
treated as a robust default analysis pipeline.

## High-Level Outcome

Relative to `origin/develop`, this branch delivers five major changes:

1. A full OpenKF/UKF fitting stack for AT-TPC tracks, including propagator,
   process model, smoothing, tests, and FairRoot integration.
2. A reworked fitter/data interface so the UKF can run as a normal
   reconstruction option rather than a standalone experiment.
3. A substantial validation and diagnostics suite built around synthetic tracks,
   digitized 16C(p,p) data, compiled tests, and interactive displays.
4. A broad redesign of clustering and pattern-recognition behavior, because
   cluster quality and ordering turned out to dominate fit quality.
5. A later pass on vertex reconstruction and kinematics so fitted tracks could
   be used more coherently at the reaction level.

This is a successful research branch. It is also still a research branch. The
core ideas are strong, but the code at HEAD still carries some split ownership,
some competing clustering paths, and some heuristics that have not yet been
distilled into one clean supported workflow.

## 1. OpenKF Became A Real AT-TPC Fitter

The first phase of the branch was about making OpenKF genuinely usable in this
codebase, not just compiling in isolation.

That work included:

- building a detector-aware propagator (`AtPropagator`) with common stepper and
  surface interfaces
- implementing UKF prediction, correction, and smoothing for AT-TPC track
  states
- stabilizing matrix handling, decomposition, covariance conditioning, and gain
  calculations
- adding process noise and energy-loss handling suitable for charged-particle
  transport in gas
- creating a physics-facing `TrackFitterUKF` layer and a FairRoot-facing
  `AtFitterUKF` wrapper

Why this mattered:

- OpenKF needed a detector-specific state model and a practical bridge into the
  existing reconstruction framework
- a fitter that only works in toy tests is not enough for this repository

In hindsight, this phase succeeded. OpenKF is no longer the speculative part of
the story. The remaining problems are mostly about how the fitter is fed and how
its output is interpreted.

## 2. The Fitter Interface Was Refactored Along The Way

UKF integration exposed weaknesses in the older fitter abstractions, so the
branch also turned into a broader refactor of the fitting layer.

That included:

- changes to `AtFitter`, `AtFittedTrack`, fit metadata, and tracking-event data
  structures
- compatibility shims where older interfaces needed to be preserved
- cleanup in the EventFit layer
- support in `AtFitterTask` and related code for selecting the UKF fitter inside
  the normal task pipeline

Why this mattered:

- the older fitter abstractions were too case-specific
- UKF integration forced the contract between pattern recognition, fitting, and
  output data products to become more explicit

The refactor helped, but it did not finish the job. Some key fitter settings
still live at the macro layer, and some of the most important controls are still
clearer in low-level or diagnostic code than in the main wrapper API.

## 3. Robustness Work Was Extensive, Because The First Failures Were Numerical

Once the fitter existed end-to-end, the next problem was stability.

Key changes:

- positive-definite covariance checks and conditioning
- reduced dependence on unstable matrix inverses
- fixes for smoother inconsistency and covariance replacement behavior
- protection against stopped-particle pathologies in the propagator
- guard rails for NaN geometry, segfault-prone states, runaway iteration, and
  infinite clip-to-surface loops
- reduced logging noise so the actual failures became diagnosable

Why this mattered:

- early UKF failures were dominated by decomposition failures, pathological
  sigma-point propagation, and invalid states
- later physics studies would have been misleading without clearing those out

This work was worth doing because it removed the obvious numerical blockers.
That changed the character of the branch. After this point the dominant
questions were no longer "can the filter run?" but "what exactly are we asking
it to fit?"

## 4. Validation Moved To Digitized Detector Data

A major shift in the branch was the move away from idealized test inputs and
toward the full detector chain.

That included:

- many-track and single-track UKF tests
- dedicated diagnostics for smoother behavior, statistics, energy loss, alpha
  scans, and straggling studies
- end-to-end validation on digitized 16C(p,p) AT-TPC events
- macros and studies that compare reconstruction output with truth

Why this mattered:

- perfect MC points hide the real reconstruction problem
- the UKF had to be judged on pad/discretization/diffusion/PSA-affected inputs,
  not only on ideal trajectories

This is one of the strongest parts of the branch. It turned the work from a
numerical project into a detector-reconstruction study and made the real
bottlenecks visible.

## 5. The Real Bottleneck Was The Input Track

This is the main scientific result of the branch.

Once the UKF itself became reasonably stable, the dominant source of poor
performance was not the filter equations. It was the quality of the ordered
cluster sequence and the momentum and vertex information supplied to the fitter.

That drove a large second phase of work:

- clustering scans and parameter studies
- configurable minimum cluster spacing
- per-cluster covariance support
- corrected cluster-covariance handling with diffusion information from
  `AtDigiPar`
- raw-hit versus clustered-hit visualization
- comparisons between clustering methods such as `ClusterizeSmooth3D` and
  `ClusterizeByGroup`
- a progression of clustering defaults from `r10 d20` to overlapping `r20 d15`
  and then to adaptive clustering

Why this mattered:

- cluster placement, spacing, and ordering define the measurement sequence seen
  by the UKF
- Brho seeds and residuals are only as good as the clustered track

The branch found the right problem, but the current code still reflects several
generations of the search. There is no single settled clustering policy yet.
Different paths use different defaults, and fitter-local adaptive re-clustering
can still override what came from upstream.

## 6. Pattern Recognition Was Pulled Closer To The Fitter

The branch pushed several operations upstream so that tracks were in better
shape before fitting.

Key changes:

- nearest-neighbor cluster ordering moved into PRA
- `AtTrackFinderTC` was corrected to use the intended ordering behavior
- circle-based fragment merging was added
- later fragment merging shifted toward vertex-based selection and merging
- beam-track rejection was added for shallow-angle lab tracks

Why this mattered:

- ordering and fragment selection should not be a fragile fitter-only cleanup
- better pre-fit topology gives better Brho estimation and better convergence

This area is improved but not settled. Track conditioning is still split between
PRA and `AtFitterUKF`, the TC path is more aligned with the later branch logic
than the HC path, and the fitter still performs nontrivial filtering, trimming,
rejection, and possible re-clustering instead of simply consuming a settled
track object.

## 7. Momentum Seeding And Vertex Recovery Were Reworked Repeatedly

Once clustering improved, seed quality and vertex momentum recovery became the
next bottlenecks.

This produced several changes:

- Brho estimation shifted to circle fits on vertex-end hits
- adaptive selection of the hits used for that circle fit
- seed overrides and MC-truth seed options for diagnostics
- back-extrapolation from the reconstructed track to the beam axis to estimate
  vertex momentum
- a later replacement of propagator-based back-extrapolation with capped linear
  back-extrapolation

Why this mattered:

- fit quality depends strongly on the initial momentum hypothesis
- whole-track circle fits were too sensitive to distorted or Bragg-dominated
  sections of the track

The branch clearly improved seed quality, but the current vertex-momentum
recovery should still be read as heuristic. It is probably the right practical
choice for now, but it does not yet look like the final physics model for a
production analysis pipeline.

## 8. Kinematics Interpretation Was Corrected

Late in the branch, the focus widened from "can we fit the track?" to "are we
interpreting the fitted track in the right physics frame?"

Key changes:

- fixes to theta/phi conversion from the Z-flipped detector frame back to the
  lab frame
- expanded kinematics analysis and visualization tools
- vertex XY and low-energy diagnostic views in `show_kinematics.C`

Why this mattered:

- ATTPCROOT uses frame conversions that can silently produce physically wrong
  angles while still looking numerically reasonable

This is a useful reminder of what the branch uncovered more generally: fit
quality and physics interpretation are coupled. A numerically healthy fit can
still produce misleading observables if the frame conventions or state
definitions are off.

## 9. Diagnostics Became Part Of The Method

This branch added a large amount of tooling around the fitter, not as a side
feature but as part of the actual development method.

That included:

- `AtUKFDisplay` as a standalone interactive fitter display
- live refit controls, raw-hit overlays, clustering controls, seed controls, and
  fit summaries
- kinematics visualization macros for failure-mode inspection
- compiled clustering scans and digitized-data scan utilities

Why this mattered:

- the hard failures were no longer obvious from scalar outputs alone
- developers needed to see where a track was ordered badly, merged badly,
  over-clustered, or seeded with the wrong momentum

The tooling also exposed an unresolved mismatch: some important controls are
better surfaced in exploratory tools than in the production API. Energy-loss
scaling is the clearest example.

## 10. The Branch Converged Scientifically Faster Than Architecturally

Several notes in `docs/development` capture intermediate plans. They are still
useful, but they should not be mistaken for the final state of the branch.

In broad terms:

- the early goal was to make OpenKF work and integrate it
- the middle goal was to validate and stabilize it on digitized data
- the late goal was to improve the reconstruction products around it so the
  fitter received better track objects and produced better reaction-level
  observables

That progression makes sense. The important caveat is that the final code still
contains traces of more than one intermediate strategy. The branch seems to have
converged scientifically before it fully converged architecturally.

## What This Branch Says About The Problem

The branch makes a strong claim about OpenKF fitting in this codebase:

The limiting factor is not just the UKF algorithm. The limiting factor is the
full reconstruction chain that defines the measurement sequence, the seed, and
the meaning of the fitted state.

That is why this branch touches:

- fitting internals
- propagator physics
- cluster covariance
- clustering defaults
- PRA ordering
- fragment merging
- Brho estimation
- vertex extrapolation
- frame conversion and kinematics

These are not side issues. Once the filter works, they are the reconstruction
problem.

The branch also suggests something more practical: most of the right questions
have already been found. The next gains will come less from another round of
free-form tuning and more from reducing duplication, settling ownership, and
enforcing one well-parameterized pipeline.

## Current Status

The fairest reading of the current branch is straightforward. The UKF is
integrated and useful. The branch succeeded in exposing the real
detector-reconstruction bottlenecks. The code still behaves like an active
research branch in a few important places.

The main unfinished pieces are:

- clustering is not unified across HC, TC, and fitter-local adaptive
  re-clustering
- pre-fit conditioning is still split between PRA and `AtFitterUKF`
- adaptive re-clustering does not cleanly inherit the same detector-parameter
  configuration as PRA
- some physics knobs matter in practice but are not exposed cleanly through the
  main wrapper interface
- fitter configuration still depends too much on macros
- fit metadata does not yet reflect every choice made by the actual filter

That does not diminish the branch. It simply means the exploration phase is
ahead of the consolidation phase.

## Suggested Next Steps

The best next step is a consolidation pass that turns the strongest ideas in
this branch into one coherent analysis path.

### 1. Settle Ownership Of The Pre-Fit Track

This is the highest-value architectural decision left in the branch.

Pick one model and commit to it:

- PRA owns the final ordered and clustered measurement sequence, and the fitter
  only fits
- the fitter owns an explicit pre-fit conditioning stage, but that stage becomes
  a named, testable, parameterized component rather than hidden mutation inside
  `AtFitterUKF`

My view is that the first option is better for analysis production. It makes
data lineage clearer, makes fitter comparisons easier, and reduces hidden side
effects.

### 2. Collapse To One Canonical Clustering Policy

The code should define:

- one canonical clustering path for production
- one optional adaptive path for controlled studies
- one explicit configuration source for clustering parameters

If HC is still supposed to matter for OpenKF, it should be brought up to the
same track-selection and merging standard as TC. Otherwise it should be treated
explicitly as a legacy or exploratory path.

### 3. Stop Letting The Fitter Silently Rewrite The Input Track

`AtFitterUKF` currently acts partly as fitter and partly as track editor. That
was useful during development and is awkward for analysis.

At minimum, the code should do one of these:

- fit a copy of the input track and preserve the original object
- produce a distinct conditioned-track object
- record explicit metadata describing every conditioning step that happened
  before the fit

### 4. Make Runtime Parameters The Single Source Of Truth

Detector and fitter configuration should flow from task and runtime parameters
rather than being re-specified in macros. That includes:

- geometry-derived quantities such as `ZPadPlane`
- measurement-noise configuration
- momentum-seed uncertainty
- diffusion parameters
- adaptive clustering thresholds
- energy-loss and straggling controls

Macros should be launchers, not configuration owners.

### 5. Expose The Physics Knobs That Already Matter

If a control matters for physics conclusions, it should be present in the public
wrapper API, recorded in output metadata, and documented as part of the
supported pipeline.

The clearest examples are:

- energy-loss scale factor
- per-cluster covariance usage
- energy straggling choice
- adaptive clustering enable/disable and thresholds
- beam-track rejection and Bragg-end trimming policy

### 6. Make Fit Metrics Match The Noise Model

If the fitter uses per-cluster covariance, the reported fit quality should use
the same model. Otherwise the code teaches users to trust diagnostics that do
not actually describe the fit they ran.

### 7. Separate Exploration Mode From Production Mode

Right now the branch mixes both.

That was useful for discovery, but it is time to formalize the split:

- production mode should be narrow, documented, parameter-driven, and stable
- exploration mode should keep the extra GUI controls, alternate clustering
  strategies, seed overrides, and heuristic experiments

### 8. Revisit Vertex Momentum Recovery As A Physics Problem

The current capped linear correction is probably the right pragmatic endpoint
for this branch, but it should not harden into dogma without a dedicated study.

That study should ask:

- where the heuristic works well
- where it biases low-energy or short-track events
- whether a more physical transport-based correction can be made robust
- whether the fitted state should explicitly distinguish first-cluster momentum
  from estimated vertex momentum

### 9. Build A Small Analysis-Grade Reference Configuration

The branch would benefit from one explicit blessed configuration and dataset
pair used for regression.

That reference should define:

- pattern-recognition mode
- clustering parameters
- fitter parameters
- runtime detector parameters
- output fit-quality metrics
- expected truth-level or benchmark distributions

Without that, the code will remain vulnerable to drift even if the architecture
gets cleaner.

## Questions Worth Settling Explicitly

These are the questions that seem most worth answering next:

1. Is HC still intended to be a serious OpenKF path, or is TC the only one that
   should be maintained?
2. Should adaptive re-clustering live inside the fitter at all?
3. What is the canonical definition of the track object the fitter is supposed
   to consume?
4. Which fitter parameters belong in the runtime DB or task configuration, and
   which are genuinely study-specific?
5. What fit-quality metric should analysis users trust when per-cluster
   covariance is enabled?
6. What is the supported definition of vertex momentum in the output physics
   objects?
7. Which current defaults are real defaults, and which are just historical
   leftovers spread across docs, headers, macros, and displays?

## Final Summary

This branch turned OpenKF fitting from an isolated experiment into a serious
reconstruction workflow for AT-TPC data. Its most important contribution is not
simply that a UKF was added. It is that the branch clarified a larger point:
accurate fitting depends on consistent treatment of track formation, ordering,
seeding, transport, vertex recovery, and kinematics interpretation.

In that sense, the branch already looks scientifically successful. The next step
is narrower and more architectural: take the best ideas it discovered and turn
them into a smaller, clearer, and more reproducible default pipeline.
