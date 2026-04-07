# PRA Remaining Seams Plan

## Summary

The first PRA cleanup pass extracted seeding and refinement behavior out of `AtPRA` into dedicated helpers while preserving current behavior. The remaining work is now about separating ownership at the next useful seams:

- candidate finding versus post-finder refinement
- cluster construction versus covariance policy inside `AtTrackTransformer`

The goal is still the same as in [PRAStructurePlan.md](/home/adam/ATTPCROOTv2/docs/development/PRAStructurePlan.md): preserve current ATTPCROOT task boundaries and runtime objects while making the reconstruction path easier to reason about and easier to swap safely.

## Seam 1: Track Finding Versus Refinement

### Problem

`AtTrackFinderTC` still owns too much of the full PRA pipeline. In one method it currently:

- converts clustering output into `AtTrack` candidates
- clusterizes hits into `AtHitCluster`
- orders clusters along the track
- applies refinement and selection policy
- computes geometric seed parameters
- emits the final `AtPatternEvent`

That means the finder still implicitly owns post-finder policy even though the behavior has already been extracted into `AtTrackRefiner` and `AtTrackSeeder`.

### Target split

The finder should own only:

- mapping algorithm output to raw `AtTrack` candidates
- candidate-local cluster construction that is needed before refinement
- noise bookkeeping for unused hits

Refinement should own:

- `SelectAndMergeTracks`
- `OrderClustersAlongTrack`
- any future primary selection, beam rejection, and fragment-merging policy

Seeding should own:

- geometric parameter estimation for the final selected tracks

### Execution steps

1. Characterize raw TC candidate output before refinement.
2. Split `AtTrackFinderTC` into:
   - raw candidate building
   - refinement and seeding finalization
3. Keep `FindTracks()` behavior identical by calling those phases in sequence.
4. Add tests that show the raw candidate count differs from the refined output on a synthetic fragment layout.
5. Once stable, move the refinement call site upward toward task-level ownership.

### Definition of done for this seam

- raw candidate construction is a distinct phase in `AtTrackFinderTC`
- refinement is applied in one obvious post-finder step
- seeding is applied after refinement, not mixed into raw candidate construction
- `AtPRAtask` output remains unchanged

## Seam 2: `AtTrackTransformer` Does Too Many Things

### Problem

`AtTrackTransformer` still mixes:

- hit grouping into clusters
- cluster centroid construction
- covariance construction

The covariance work already showed that this is a real scientific strategy axis, not just an implementation detail.

### Target split

Keep `AtTrackTransformer` as the public compatibility façade, but separate its internal responsibilities into:

- cluster-building logic
- centroid construction logic
- covariance construction logic

### Execution steps

1. Add or extend characterization tests for current cluster count, centroid positions, and covariance contents.
2. Extract covariance construction first while keeping public `AtTrackTransformer` APIs unchanged.
3. If centroid calculation remains tangled with covariance logic, extract centroid construction next.
4. Only after the behavior is locked down should cluster-building policy itself become independently swappable.

### Definition of done for this seam

- covariance construction is no longer hidden inside one monolithic clustering implementation
- centroid and covariance logic are independently testable
- current transformer-direct behavior remains unchanged

## Current Status

### Completed so far

- Seam 1 has been split at the code-organization level:
  - geometric seeding now lives in `AtTrackSeeder`
  - candidate refinement now lives in `AtTrackRefiner`
  - `AtTrackFinderTC` exposes a raw-candidate build phase and a later refinement/seeding finalization phase
- characterization coverage was added around:
  - track ordering and fragment merging behavior
  - geometric seeding behavior
  - the seam between raw TC candidate construction and later refinement/seeding
- Seam 2 has now started inside `AtTrackTransformer`:
  - covariance-aware cluster construction was extracted into an internal helper
  - `AtTrackTransformer` still presents the same public API, but no longer owns the covariance-building details inline
  - direct transformer covariance, online covariance, and diagonal-only online covariance are now testable independently from the clustering loop

### Still to do

- move refinement ownership farther upward so `AtTrackFinderTC` is no longer the place that decides event-level post-finder policy
- decide whether the next seam-2 extraction should split centroid construction from covariance construction
- if that split is useful, add characterization coverage that makes centroid policy independently swappable
- only after centroid and covariance behavior are locked down should cluster-building policy itself become more modular

## Recommended order

1. Separate raw TC candidate building from refinement/seeding.
2. Move refinement ownership upward from finder internals toward task/application flow.
3. Characterize `AtTrackTransformer` behavior in more detail.
4. Extract covariance construction from `AtTrackTransformer`.
5. Extract centroid construction if needed.

## Acceptance criteria

- current PRA behavior remains covered by unit tests and the interpreted simulation regression
- `AtTrackFinderTC` can be understood as candidate discovery plus local cluster building, not full event interpretation
- refinement policy has a single obvious home
- covariance policy can be changed without rewriting track-finding logic
