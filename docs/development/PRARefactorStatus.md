# PRA Refactor Status

## Plan In Scope

This status note tracks the PRA cleanup work described in [PRAStructurePlan.md](/home/adam/ATTPCROOTv2/docs/development/PRAStructurePlan.md) and refined in [PRARemainingSeamsPlan.md](/home/adam/ATTPCROOTv2/docs/development/PRARemainingSeamsPlan.md).

The active plan has two main seams:

1. separate track finding from post-finder refinement and seeding
2. separate cluster construction from covariance policy inside `AtTrackTransformer`

The goal is to keep ATTPCROOT runtime objects and macro-facing task contracts stable while making PRA easier to reason about, test, and swap in pieces.

## Accomplished

### Seam 1: Track Finding Versus Refinement

The first seam has been separated at the component level.

- geometric seeding was extracted from `AtPRA` into `AtTrackSeeder`
- candidate refinement was extracted from `AtPRA` into `AtTrackRefiner`
- `AtTrackFinderTC` now has an explicit raw-candidate construction phase and a later finalization phase
- characterization tests were added for:
  - cluster ordering
  - fragment merging and beam-like rejection
  - curved-track seeding
  - the raw-candidate versus finalized-track seam in `AtTrackFinderTC`

This means the code now exposes the seam clearly even though refinement is still invoked from finder-level flow.

### Seam 2: `AtTrackTransformer` Internal Responsibilities

The second seam has started.

- covariance-aware cluster construction was extracted into `AtTrackClusterBuilder`
- `AtTrackTransformer` continues to expose the same public API and behavior
- transformer-direct covariance, online covariance, and diagonal-only online covariance are now covered independently in tests
- focused builder tests now verify that:
  - the current transformer-direct centroid and covariance are preserved
  - online covariance can be swapped in without changing the current centroid/metadata path
  - diagonal-only online mode zeroes off-diagonal terms while keeping the online diagonals

This establishes a real internal boundary between the track clustering loops and covariance construction.

## Remaining Work

### Seam 1

- move refinement ownership upward from `AtTrackFinderTC` toward task/application flow
- make the post-finder refinement stage the single obvious home for candidate selection, fragment merging, and ordering
- keep `AtPRAtask` output unchanged while reducing finder ownership further

### Seam 2

- decide whether centroid construction should be extracted next from `AtTrackClusterBuilder`
- if extracted, add characterization tests that pin centroid behavior independently from covariance mode
- only after centroid and covariance behavior are isolated should cluster-building policy itself become independently swappable

## Current Assessment

The refactor is past pure planning and into stable seam extraction.

- seam 1 is functionally separated but not yet fully re-owned at task level
- seam 2 now has its first concrete internal extraction in place
- the next useful step is to decide whether to move centroid construction into its own strategy boundary or stop once covariance construction is sufficiently isolated for the immediate physics work
