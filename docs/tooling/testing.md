# Testing

ATTPCROOT has two categories of tests: unit tests (compiled, fast) and integration tests (interpreted ROOT macros).

## Unit Tests

Unit tests use [Google Test](https://github.com/google/googletest). They live alongside the source files they test — for example, `AtSimulationData/AtVertexPropagatorTest.cxx` tests `AtVertexPropagator`.

### Writing a Test

Create a `*Test.cxx` file in the module directory and register it in the module's `CMakeLists.txt`:

```cmake
set(TEST_SRCS MyClassTest.cxx)
attpcroot_generate_tests(${LIBRARY_NAME}Tests SRCS ${TEST_SRCS} DEPS ${LIBRARY_NAME})
```

`attpcroot_generate_tests` links against Google Test and registers the binary with CTest. The test binary is placed in `build/tests/`.

Typical binary names follow the module, for example:

- `AtDataTests`
- `AtGeneratorsTests`
- `AtToolsTests`
- `OpenKFTests` for Eigen/OpenKF-specific reconstruction tests

### Rules

- **Tests must not access external files or network resources.** Hardcode small data values inline or use files that ship with the repo under `$VMCWORKDIR`. Tests that require external state are fragile and break on other machines.
- Tests should be fast. Unit tests that take more than a second are a smell.
- For naming conventions see the [wiki](https://github.com/ATTPC/ATTPCROOTv2/wiki/Coding-conventions#test-names).

### Running Unit Tests

```bash
cd build && ctest -V          # all tests
./build/tests/AtToolsTests    # one binary directly
```

## Integration Tests

More involved tests (those that require data files or test full pipelines) live in `macro/tests/`. They are written as ROOT macros (`.C` files) and run interpreted, not compiled.

Do not use ACLiC (`.C+`, `.C++`) or any other macro-compilation path for repo macros. Run them with `root -l -q ...` or `root -l -b -q ...`.

These are listed and run separately from unit tests. Use them for:

- end-to-end workflow checks
- simulation plus reconstruction flows
- experiment-facing regression scenarios

One focused simulation-based PRA regression now lives in `macro/tests/AT-TPC/`:

- `run_pra_sim_integration.C` generates a small `16C(p,p)` simulation sample
- `run_pra_sim_reco.C` runs the full `AtPRAtask` plus `AtFitterTask` UKF chain on that sample
- `AssertPRASimIntegration.C` opens the output and fails if the run does not produce at least a minimal amount of `AtPatternEvent` and `AtTrackingEvent` content

### Prerequisites

Integration tests require geometry ROOT files that are **not** committed to the repo. Generate them first:

```bash
macro/tests/generateGeometry.sh
```

Some tests also require external data files that are too large for the repo:

- **GADGETII tests**: require the fishtank dataset (available on FRIB/NSCL cluster only)
- **SpecMAT tests**: require `TTreesGETrun_9993.root` placed in `macro/tests/SpecMAT/data/`

### Running All Integration Tests

```bash
macro/tests/runAllTest.sh
```

Results are logged in each test subdirectory as `test.log`.

### Running The Simulation-Based PRA Integration Test

```bash
source build/config.sh
root -l -b -q 'macro/tests/AT-TPC/run_pra_sim_integration.C(8)'
root -l -b -q 'macro/tests/AT-TPC/run_pra_sim_reco.C(8)'
root -l -b -q 'macro/tests/AT-TPC/AssertPRASimIntegration.C()'
```

This test is intended as a PRA-friendly simulation smoke/regression check. It is a better fit for PRA cleanup work than the real fission sample `run_0174.root`, which remains useful for branch-flow smoke testing but is not a representative PRA efficiency benchmark.

See [macro-cookbook.md](macro-cookbook.md) for curated macro starting points.

## Continuous Integration

Every PR runs:
1. `clang-format` check
2. `clang-tidy` and `iwyu` static analysis
3. Full unit test build and run (`ctest`)

Run the tests locally before submitting a PR to catch failures early.
