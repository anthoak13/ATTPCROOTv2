# PRA Structure Plan

## Summary

This note proposes a cleanup of PRA that keeps the current ATTPCROOT branch flow and runtime objects intact while separating the responsibilities that are currently tangled across `AtPRAtask`, `AtPRA`, `AtTrackFinderTC`, `AtTrackFinderHC`, and `AtTrackTransformer`.

The design goal is not to introduce new persisted data classes or parallel pipeline abstractions. The goal is to make the existing PRA pieces easier to reason about and easier to swap independently:

- candidate finding
- candidate refinement
- track building
- cluster covariance construction
- geometric seed estimation

The current macro-facing task contracts should remain unchanged during this cleanup:

- `AtPRAtask`: `AtEvent -> AtPatternEvent`
- `AtSampleConsensusTask`: `AtEvent -> AtPatternEvent`
- `AtPatternModificationTask`: `AtPatternEvent -> AtPatternEvent`

## Guiding Constraints

- Reuse the existing ATTPCROOT data model: `AtEvent`, `AtPatternEvent`, `AtTrack`, `AtHitCluster`.
- Do not add new branch-level event classes.
- Do not introduce a parallel naming scheme that conflicts with existing `At*` conventions.
- Keep compatibility with current macros while improving the internal separation.
- Prefer moving responsibilities to existing task seams before adding new task types.

## Proposed Separation

### 1. Pre-pattern event cleanup remains outside PRA

`AtEvent -> AtEvent` preprocessing already exists in the reconstruction chain and should remain the home for event-level cleanup before pattern recognition.

This stage should own:

- hit cleaning
- beam/noise suppression
- event-level outlier rejection
- any future preprocessing that should apply before any candidate finder runs

Implication for PRA:

- logic that is really pre-pattern cleanup should not live in `AtPRA`
- kNN-style pruning should be reviewed and either moved into an `AtEvent` cleaning task or reclassified as track-level refinement

### 2. Candidate finding should only group hits into candidates

`AtTrackFinderTC` and `AtTrackFinderHC` should be narrowed so that they only do the algorithm-specific work of assigning hits to track candidates.

They should continue to:

- consume `AtEvent`
- create `AtTrack` candidates inside an `AtPatternEvent`
- fill `fHitArray` with candidate member hits
- fill `fNoise` with unassigned hits

They should stop owning:

- cluster ordering
- fragment merging
- vertex-based candidate selection
- geometric seed estimation
- covariance-policy decisions

That keeps TC, HC, and sample-consensus as different candidate-finding front ends that can share the same downstream preparation logic.

### 3. Candidate refinement should be a distinct `AtPatternEvent` stage

Operations that modify, reorder, merge, or reject already-found candidates are not part of candidate finding itself. They should operate on `AtPatternEvent` after the finder has produced initial candidates.

This stage should own:

- fragment merging
- primary-track selection near the vertex
- beam-track rejection
- ordering of hits or clusters along the track
- future candidate splitting when a finder over-merges

The preferred home is the existing `AtPatternModificationTask` mechanism, using focused `AtPatternModification` implementations instead of embedding these policies into each finder.

This directly applies to logic currently living in `AtPRA`, especially:

- `SelectAndMergeTracks`
- `OrderClustersAlongTrack`
- any other policy that edits track candidates after they have already been found

### 4. Track building should be separate from geometric seeding

The current PRA path mixes two different concerns:

1. building the `AtTrack` representation used by downstream fitters
2. estimating the geometric quantities used as fit seeds

These should be separated internally without changing the public runtime objects.

Track building should own:

- filling `fHitClusterArray`
- choosing clusterization behavior
- choosing centroid construction behavior
- choosing covariance construction behavior

Geometric seeding should own:

- filling `fPattern`
- filling `fGeoRadius`
- filling `fGeoCenter`
- filling `fGeoTheta`
- filling `fGeoPhi`

Concretely:

- `AtTrackTransformer` should be responsible for track-building concerns
- `AtPRA::SetTrackInitialParameters` should move out of the generic PRA base into a dedicated seeding component under `AtPatternRecognition`
- candidate finders should call that seeding component rather than inheriting seeding behavior from `AtPRA`

This gives a cleaner swap boundary:

- same candidates, different cluster or covariance builder
- same built track, different seeding strategy

### 5. Covariance construction should be a first-class strategy inside `AtTrackTransformer`

The recent covariance work already shows that covariance handling is not just a minor detail of clusterization. It is an independent analysis axis.

`AtTrackTransformer` should therefore be split internally into separate concerns:

- hit-to-cluster grouping
- cluster centroid construction
- cluster covariance construction

This allows the framework to compare or swap:

- legacy and alternate covariance modes
- different centroid rules with the same candidate membership
- different covariance builders without rewriting the track finder itself

The public goal is not to add many new public interfaces immediately. The goal is to stop encoding covariance behavior as a hidden side effect of one monolithic clustering path.

### 6. Reduce `AtPRA` to shared orchestration

`AtPRA` currently acts as both:

- a base class for candidate finders
- a container for shared post-processing and fitter-facing logic

That makes it too broad.

After cleanup, `AtPRA` should keep only what is truly common infrastructure for PRA-derived finders:

- shared configuration plumbing
- access to shared helpers used by multiple finders
- minimal common orchestration

It should stop directly owning:

- pre-pattern event cleanup
- candidate refinement policy
- geometric seeding implementation
- fitter-oriented track preparation details that belong in dedicated helpers

## Suggested Migration Order

1. Extract geometric seeding from `AtPRA::SetTrackInitialParameters` into a dedicated helper or class in `AtPatternRecognition`.
2. Extract candidate refinement logic from `AtPRA` and the finder classes into focused `AtPatternModification` implementations or an equivalent `AtPatternEvent` refinement layer.
3. Split `AtTrackTransformer` internally into cluster-building concerns and covariance-building concerns.
4. Narrow `AtTrackFinderTC` and `AtTrackFinderHC` so they are candidate finders first, not full candidate-to-fit preparation pipelines.
5. Update `AtPRAtask` so it assembles the existing pieces in sequence rather than depending on one oversized finder implementation.
6. Where useful, let `AtSampleConsensusTask` reuse the same downstream seeding or refinement stages.

## Tests

Before each extraction, add characterization coverage so the cleanup does not silently change current behavior.

Recommended test coverage:

- `AtPRAtask` output characterization on representative events:
  - number of candidates
  - number of noise hits
  - cluster count
  - seed-field behavior
- candidate-refinement tests on synthetic `AtPatternEvent` inputs:
  - merge behavior
  - vertex-based selection
  - ordering stability
- `AtTrackTransformer` tests:
  - same cluster membership and centroiding in legacy mode
  - covariance modes compared independently
- geometric seeding tests:
  - same `fGeo*` and `fPattern` outputs on fixed `AtTrack` inputs
- integration tests:
  - `AtPRAtask` still produces fitter-usable `AtPatternEvent`
  - finder choice can vary independently from covariance and seeding choice

## Expected End State

After the cleanup:

- pre-pattern event cleanup remains an `AtEvent -> AtEvent` concern
- candidate finding remains an `AtEvent -> AtPatternEvent` concern
- candidate refinement becomes an explicit `AtPatternEvent` concern
- track building and covariance policy live in `AtTrackTransformer`
- geometric seed estimation is its own concern under `AtPatternRecognition`
- `AtPRAtask` remains macro-compatible, but becomes easier to understand and extend

This keeps the current ATTPCROOT structure recognizable while making PRA less monolithic and more scientifically testable.
