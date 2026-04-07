# Macro Cookbook

Example macro starting points referenced by these docs. 

## Notes

- `macro/examples/` contains adaptation starting points.
- `macro/tests/` contains repeatable workflow and regression-style examples.
- Many macros are experiment-specific or user-specific and are intentionally omitted.
- Files under `macro/` are ROOT / Cling scripts, not normal compiled translation units.
- Run repo macros interpreted with `root -l -q ...` or `root -l -b -q ...`; do not compile them with ACLiC (`.C+`, `.C++`) or treat them like build targets.
- ROOT/core headers such as `TClonesArray.h` or `TTreeReader.h`, and STL headers a macro genuinely uses, are normal.
- Do not treat a macro like standalone C++ or try to fix missing symbols by adding project headers first.

## Macro Do / Don't

- Do: copy an existing pattern from `macro/examples/` or `macro/tests/`.
- Do: run macros interpreted and keep them in the ROOT/Cling style already used in this tree.
- Don't: start by adding `#include "At*.h"` to compensate for missing dictionaries or libraries, or rewrite a macro as if it were a compiled source file.
- Don't: compile repo macros with ACLiC or any other macro-compilation path.

## Starting Points

| Path | Use | Caveat |
|------|-----|--------|
| `macro/examples/run_sim.C` | minimal simulation structure | simple example, not a full workflow template |
| `macro/examples/rundigi_sim.C` | simulation plus downstream task ordering | parts of the task usage are older and should be checked against current APIs |
| `macro/tests/AT-TPC/run_unpack_attpc.C` | unpacking plus PSA/cleaning flow | experiment-facing example, not a framework contract |
| `macro/tests/AT-TPC/run_sim_attpc.C` | fuller simulation test flow | oriented toward test workflow rather than minimal onboarding |
| `macro/tests/AT-TPC/run_pra_sim_integration.C` | small simulation-based PRA integration setup for `16C(p,p)` | generates MC files under `macro/tests/AT-TPC/data/pra-sim-integration/` |
| `macro/tests/AT-TPC/run_pra_sim_reco.C` | runs PRA + UKF on the generated `16C(p,p)` simulation sample | expects output from `run_pra_sim_integration.C` |
| `macro/tests/AT-TPC/AssertPRASimIntegration.C` | asserts nontrivial `AtPatternEvent` and `AtTrackingEvent` output from the simulation regression | intended to be run after `run_pra_sim_integration.C` |
