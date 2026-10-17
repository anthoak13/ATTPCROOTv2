# Fission Fit Validation: User Manual

How to run simulate → digitize → fit studies for e12014 fission and look at the results. The macros started as copies of Skyler Gangestad's validation work in [`../skyler/`](../skyler/); see her [NOTES.md](../skyler/NOTES.md) for the physics, results so far and known issues.

## Contents

1. [Setup](#setup)
2. [Quick start](#quick-start)
3. [Writing a study](#writing-a-study)
4. [Running a study](#running-a-study)
5. [Looking at results](#looking-at-results)
6. [Running on a cluster](#running-on-a-cluster)
7. [Running macros by hand](#running-macros-by-hand)

Reference:

8. [Settings reference](#settings-reference)
9. [Command-line options](#command-line-options)
10. [What reruns](#what-reruns)
11. [Files produced](#files-produced)
12. [Chunks and seeds](#chunks-and-seeds)
13. [SLURM details](#slurm-details)
14. [Plots](#plots)
15. [Troubleshooting](#troubleshooting)
16. [Adding a setting](#adding-a-setting)
17. [Appendix: known paths by machine](#appendix-known-paths-by-machine)

## Setup

You need:

- A built ATTPCROOT with its environment loaded (`source build/config.sh`). See [docs/tooling/installation.md](../../../docs/tooling/installation.md).
- Python 3.8 or newer (standard library only). If `python3` is older, as on some clusters, run `python3.11 fission.py` (or whichever newer one there is) instead of `./fission.py`.
- The shared inputs, in one directory pointed to by `TPC_SHARED_INFO`:

```
$TPC_SHARED_INFO/
├── eLoss/
│   ├── LISE/<Z>_<A>.txt     # one per fragment in the ion list, plus the beam: 83_200.txt
│   └── SRIM/<Z>_<A>.txt     # same, for SRIM
├── respAvg.root             # averaged pad response
└── e12014_zap.csv           # low-gain pads: digi turns them down, the fit skips them
```

Set the environment:

```bash
export TPC_SHARED_INFO=/path/to/shared/info
export FISSION_DATA=/path/to/data   # optional, default: data/ next to these macros
```

`FISSION_DATA` is where every output file goes. A relative path is relative to the directory you start `fission.py` or ROOT in, so start both from the same place, or use an absolute path. Put it on a disk with plenty of room. Digi files store raw traces, and fit files store the 10 best simulated events per event, so each costs roughly 20–25 MB per event. A 500-event sim with one digi and one fit is about 20–25 GB.

## Quick start

From this directory:

```bash
./fission.py studies/smoke.ini --dry-run   # list what would run
./fission.py studies/smoke.ini -j 2        # run using two jobs using 2 threads per job (a few minutes)
```

Output:

```
smoke-base.sim                           missing
smoke-base.digi                          missing
smoke-base.fit                           missing
smoke-srimfit.fit                        missing
start  smoke-base.sim
done   smoke-base.sim (5 s)
...
```

Run it again and it prints `Everything is up to date.` `--status` shows what is finished:

```
section      input         state
sim base                   done
digi base    <- sim base   done
fit base     <- digi base  done
fit srimfit  <- digi base  done

4 of 4 sections finished.
```

Plot one fit:

```bash
root -l
.L plot_fit.C
plot_fit("smoke", "base")
```

## Writing a study

A study is one `.ini` file in `studies/`. The file name (without `.ini`) is the study name. It holds the outputs to make and their settings. It is the record of what was done, so commit it.

The studies already in `studies/`:

- `smoke.ini`: 5 events through sim, digi and fit, plus a second fit of the same digi with SRIM tables. 
- `smoke_refit.ini`: one more fit (20 iterations) of `smoke.ini`'s digi. An example of a study that reads another study's output (see [What a section reads](#what-a-section-reads)).
- `digi_scan.ini`: digitize `smoke.ini`'s sim with three different sets of parameters.
- `fit_scan.ini`: one fit of each `digi_scan.ini` digitization. 
- `bench.ini`: one sim and digi, fit with 1, 2, 4 and 8 threads, to time the fit.
- `decay_angle.ini` (decay angle 90° vs. 120°), `mass_dev.ini` (`massDev` 0 vs. 2) and `eloss_mismatch.ini` Guessed implementations of skyler's previous results

Each section makes exactly one output called `<name>`. By default, the stage looks for a proper input of the same name, but that can be overwritten using `input`.

There are three stages: a simulation `[sim <name>]`, a digitization `[digi <name>]` and a fit `[fit <name>]`.

```ini
[defaults]                    ; <stage>.<key>, applies to every section of that stage
sim.events = 500
sim.zToSim = 50
sim.eloss = LISE
fit.iter = 100

[sim 90deg]                   ; keys in a section have no prefix: decayAngle is sim.decayAngle
decayAngle = 90

[digi 90deg]                  ; digitizes [sim 90deg]

[fit 90deg]                   ; fits [digi 90deg]

[fit 90deg-srim]              ; fits [digi 90deg], with SRIM tables
input = 90deg
eloss = SRIM
```

Rules:

- A key `k` in a `[<stage> <name>]` section is the setting `<stage>.k`. In `[defaults]`, write the full `<stage>.k`. This holds for every setting, including `sim.events`, `sim.chunks`, `<stage>.seed`, `<stage>.input` and `<stage>.slurm.*` (see the [reference](#settings-reference)).
- A section's settings are the `[defaults]` keys of its *stage*, overridden by the section.
- Anything not set uses the macro's built-in default.
- Comments start with `;` or `#`.
- Section names may use letters, digits, `_` and `-`, and are unique per stage (a sim and a fit may share a name). Study names (the file name) may use letters, digits and `_`, so `<study>-<name>` splits only one way.
- Any section header other than `[defaults]` and `[sim|digi|fit <name>]` will error, as will a key with a stage prefix inside a stage's section.

### What a section reads

A digitization's input is a simulation output, and a fit's input is a digitization output. The input is chosen in this order:

1. `input` key listed in the section in the section.
2. `<stage>.input` in `[defaults]`, e.g. `fit.input = t1` to fit one digitization several ways.
3. The output of the previous stage with the same name: `[fit 90deg]` fits the output of `[digi 90deg]`. If there is no such section, the driver stops and names the section it looked for.

The `input` key has three forms for chaining different studies. For example, for a fit:

| Form | Input |
|---|---|
| `input = 90deg` | the output of `[digi 90deg]` from this study |
| `input = decay_angle:90deg` | the output of `[digi 90deg]` in the same folder as this study |
| `input = manualDigi.root` | a digitization file made outside the driver by running `run_digi_fiss.C` [by hand](#running-macros-by-hand).|

An `input` containing `/` or ending in `.root` is a file path which is treated as relative to `$FISSION_DATA` unless it's an absolute path. It may be a glob that matches several files.

Running a section also brings its inputs in this study up to date: running a fit first runs its digitization and simulation if they are missing or suspected to be out of date.

## Running a study

```bash
./fission.py STUDY.ini [SECTION ...] [options]
```

With no sections named, the whole study runs. `NAME` selects every section with that name, and `NAME.STAGE` (e.g. `90deg.digi`) one section. The everyday commands:

```bash
./fission.py studies/decay_angle.ini --dry-run              # what would run, and why
./fission.py studies/decay_angle.ini -j 4                   # run, up to 4 jobs at once
./fission.py studies/decay_angle.ini --status               # what is finished
./fission.py studies/decay_angle.ini 90deg --force fit      # refit 90deg even if up to date
```

Every option is in [Command-line options](#command-line-options).

Only out-of-date sections run: a section reruns when its output is missing, when a setting it or its input chain depends on changed in the study file, or when its input reran. `--dry-run` gives the reason for each. See [What reruns](#what-reruns).

Outputs are overwritten in place. To keep an old result, give the new version a new name.

### Parallel running

There are two independent levers:

- **`-j N`** runs separate jobs at once, such as different sections, or a fit while another sim runs.
- **`sim.chunks = N`** splits a sim into N pieces, each with its own seeds. Each piece goes through digi and fit on its own. Plots combine the pieces automatically. See [Chunks and seeds](#chunks-and-seeds).

Every fit job also uses `fit.threads` threads. Keep `-j × fit.threads` near your core count.

Prefer more chunks over more threads. The fitter's threads wait on a shared lock, so speed does not grow with thread count, and they draw from one shared random-number generator. Separate processes avoid both. `studies/bench.ini` measures how fit time scales with threads on a given machine.

No fit is reproducible, even with one thread: the fitter seeds its parameter sampling randomly and `fit.seed` does not reach it. See bug 2 in [mc-fitter-threading.md](../../../docs/development/mc-fitter-threading.md).

## Looking at results

Start ROOT from this directory, with `FISSION_DATA` set as it was when the study ran.

**One fit:**

```cpp
.L plot_fit.C
plot_fit("decay_angle", "90deg")     // [fit 90deg] of decay_angle.ini: fills and draws the Z histogram
zHistDiff->Draw()                    // true Z minus fitted Z
```

Events the fitter skipped (no good Y-pattern) have no fit results and are left out of every plot. Only finished fit files are read: a fit that failed, is running or is queued is skipped with a warning.

To plot specific files, give a list. Wildcards are allowed, and the files are read as they are, finished or not:

```cpp
plot_fit(std::vector<TString>{"data/decay_angle-90deg.fit*.root", "data/decay_angle-120deg.fit*.root"});
```

**Compare fits by a setting:**

```cpp
.L group_fit_plots.C
group_fit_plots("decay_angle", "sim.decayAngle")   // writes ./.groups/decay_angle/<plot>.root
show_groups("Z", "./.groups/decay_angle")          // side by side
show_groups("Z", "./.groups/decay_angle", true)    // overlaid
```

Fits with the same value of the setting are combined. The values come from the settings recorded in each fit file, which include its whole chain's settings and the fit macro's defaults. Only the `[fit <name>]` sections in `studies/<study>.ini` are used, so leftover files from renamed or removed fits are ignored. Start ROOT from this directory so it can find the study file.

The histograms and plot names are listed in [Plots](#plots).

## Running on a cluster

From your own machine, with the cluster `hpc` defined in `remotes.ini` (below):

```bash
./fission.py --remote hpc --sync                            # bring the cluster's checkout to this commit, built
./fission.py studies/decay_angle.ini --remote hpc --slurm    # submit the study on the cluster
./fission.py studies/decay_angle.ini --remote hpc --status   # check on it
./fission.py studies/decay_angle.ini --remote hpc --fetch    # copy the finished fits' results here
root -l -e '.L plot_fit.C' -e 'plot_fit("hpc:decay_angle", "90deg")'   # plot them
./fission.py studies/decay_angle.ini --remote hpc 90deg.fit --fetch-logs   # copy logs, if needed
```

`--remote hpc` first copies the study file, and every study it reads from (`input = <study>:<name>`), to the cluster's checkout, overwriting the cluster's copies. Then it runs the same command there, in the cluster's environment, so `--slurm`, `--status`, `--dry-run`, `--show`, `--force` and section selections all act on the cluster. Jobs don't run on the cluster's login node: give `--slurm` to run anything.

Prerequisites:

- `ssh <host>` works without a prompt (a key, or an agent). Put any port, user or key for the host in `~/.ssh/config`.
- `remotes.ini` next to `fission.py` (not committed). Copy `remotes.ini.example` and fill it in:

| Key | Meaning |
|---|---|
| `host` | ssh host or alias (from `~/.ssh/config`). |
| `repo` | The ATTPCROOT checkout on the cluster, used only by this driver (`--sync` creates it if missing). The driver runs from the same directory in it as here. |
| `env.<VAR>` | Exported on the cluster before running, configuring and building, e.g. `env.TPC_SHARED_INFO` and `env.FISSION_DATA` (paths the compute nodes can see), and `env.SIMPATH` and `env.FAIRROOTPATH` for a new checkout. Used as written: no `~` or `$VAR` expansion. |
| `cmake.<VAR>` | Passed as `-D<VAR>=<value>` when `--sync` configures a new checkout, e.g. `cmake.CMAKE_PREFIX_PATH` (the HDF5 prefix, plus any other prefix the machine needs, `;`-separated). An existing build keeps its cache, so these matter only for a first configure. |
| `[<stage>.]slurm.<key>` | Values for `slurm_job.sh` for every study run on this cluster (see [SLURM details](#slurm-details)). The study's settings override them. |

**The cluster's checkout.** The code a study runs on the cluster (`fission.py`, the macros, the libraries) is the checkout at `repo`, not what is on your machine. It belongs to the driver, not to your daily work on the cluster: `./fission.py --remote hpc --sync` brings it to your current branch and commit and builds it. For a new cluster, set `env.SIMPATH`, `env.FAIRROOTPATH` and `cmake.CMAKE_PREFIX_PATH` (the values of the [first configure](../../../docs/tooling/installation.md), the HDF5 prefix included), pick a `repo` that doesn't exist yet, and run `--sync`. It:

1. Sends the commits the cluster doesn't have, as a git bundle over ssh: the whole branch the first time (about 80 MB), later only the new commits. The cluster needs no GitHub access.
2. Clones the checkout from it, or fast-forwards the existing one to your commit and checks out your branch. Changes to `studies/` there are uploaded copies, so they're discarded (the next command uploads them again).
3. Configures the build if `build/config.sh` doesn't exist, with the `env.*` and `cmake.*` values.
4. Builds it (`cmake --build`, half the login node's cores) unless the last `--sync` build is of this commit, and records the commit in `build/.fission_built`. Output goes to `<repo>/build/fission-sync.log`. It runs in the foreground: if the connection drops, run `--sync` again (the build is incremental).

`--sync` runs alone (no study, no other action) and refuses when:

- `repo` exists but isn't a git checkout;
- the checkout has changes to tracked files outside `studies/` (someone edited it by hand: commit or discard them there);
- any of your SLURM jobs there is a fission job (its name is `<study>-<name>.<stage>`), since queued jobs read the checkout when they start;
- the cluster's copy of your branch has commits yours doesn't (it only fast-forwards);
- a checkout without `build/config.sh` would be configured, but `env.SIMPATH`, `env.FAIRROOTPATH` or `cmake.CMAKE_PREFIX_PATH` isn't set. This is checked before anything is sent.

Uncommitted changes on your machine are not sent: `--sync` and every `--remote` command warn `not on hpc: uncommitted changes in …` when tracked files outside `studies/` differ from your commit.

**The sync check.** Every other `--remote` command checks that the cluster's checkout is at your commit, has no changes outside `studies/`, and was built by `--sync` at that commit. If not, it lists the differences (`hpc is at cc09f002, you are at 44fcc59d`, `hpc's build is from …`, `hpc has local changes: …`). `--slurm` then stops before submitting anything; add `--no-sync-check` to submit anyway. `--status`, `--show`, `--dry-run`, `--fetch` and `--fetch-logs` print the differences as warnings and go on.

**Several versions on one cluster.** Each checkout is its own `remotes.ini` entry: the same `host`, a different `repo` (each a full clone with its own build), and usually the same `env.FISSION_DATA`. `--sync` each entry from the branch it should run. Fetched results stay apart, in `$FISSION_DATA/remote/<entry>/`, and `plot_fit("<entry>:<study>", …)` reads each entry's.

**Results files.** Fits run through `--remote` also write `<fit>.results.root` (and its `.results.cfg`) in the same job, with `slim_fit.C`. It holds what `plot_fit.C` reads: per event, the fit results (`AtMCResult`), the truth (`SimInfo`), the best simulated event (`SimEvent`, 1 instead of 10) and the digi's hits (`AtEvent`) and Y-pattern (`AtFissionEvent`), with the same branch names, but no raw traces. That is about 70 kB per event, against about 35 MB for the digi and fit files. If writing it fails, the fit job fails.

`--fetch` copies the results files of the selected fits whose job finished and isn't queued, to `$FISSION_DATA/remote/<remote>/`, with the same names as on the cluster. It overwrites that remote's copies and never deletes anything. `--fetch-logs` copies the `.log` of every job of the selected sections the same way; it can be large (about 1.25 MB per fit event). Both list the files and total size first, and with `--dry-run` only list them. A study argument `"<remote>:<study>"` to `plot_fit` and `group_fit_plots` reads these copies (see [Plots](#plots)).

### Running on the cluster directly

Logged in on the cluster, with the ATTPCROOT environment loaded and `TPC_SHARED_INFO` and `FISSION_DATA` set to paths the compute nodes can see:

```bash
./fission.py studies/decay_angle.ini --slurm --slurm-set slurm.time=1-00:00:00 --slurm-set slurm.mem=8G
```

This is what `--remote` runs (with the newest `python3.N` there), plus `--results` (write results files) and the remote's `slurm` values as `--slurm-set`. It submits one `sbatch` job per section and chunk. Each job waits for the job that makes its input (`--dependency=afterok`). As with local runs, a job succeeds if the macro wrote its `.cfg`, whatever ROOT's exit code. Fit jobs request `fit.threads` CPUs, and other jobs request one. Logs go to the usual `.log` files. Jobs inherit the environment of the shell you submit from.

Check status with `squeue`. If a job fails, SLURM cancels the jobs waiting on it. To check progress, use `--status`: sections with jobs still in SLURM show as `queued`. When the jobs finish, every section shows `done`. If a job failed, the next run reruns only that section and what reads it.

While jobs whose files a run would write or remove are still queued or running, the driver refuses to run. See [SLURM details](#slurm-details).

## Running macros by hand

Every stage is an ordinary ROOT macro and still runs on its own, which is the easy way to try out a change.

**With built-in defaults:**

```bash
root -l -b -q run_simp_fiss.C    # writes $FISSION_DATA/manual.sim.root
root -l -b -q run_digi_fiss.C    # reads manual.sim.root,  writes manual.digi.root
root -l -b -q run_fit.C          # reads manual.digi.root, writes manual.fit.root
```

Each stage writes `manual.<stage>.cfg`, listing every setting and the seed it used. To try something, edit the default in the macro.

**With a settings file:** write `key = value` lines, with the full `<stage>.` prefix, and pass the file:

```bash
cat > try.cfg <<EOF
sim.events = 20
sim.decayAngle = 120
output = data/try.sim.root
EOF
root -l -b -q 'run_simp_fiss.C("try.cfg")'
```

`input` and `output` set the file paths. Without them, the `manual.*` names are used.

Interactive viewers (`run_eve_*.C`) take file names directly and are not part of the driver.

## Settings reference

Defaults are the macros' built-in values, except where marked as the driver's.

**Driver settings, every stage.** These are used by the driver and never passed to a macro, except for the seed.

| Key | Default | Meaning |
|---|---|---|
| `<stage>.seed` | from study, name and stage (driver) | Random seed of the stage. Fixed for a given section, so sim and digi reruns reproduce. The fit does not (see [parallel running](#parallel-running)). |
| `<stage>.input` | the section of the previous stage with the same name (driver) | The section's input. See [What a section reads](#what-a-section-reads). Not allowed for a sim. Another study's sections and input files are never run, forced or removed from here. Input files must each have the `.cfg` their stage wrote next to them, which supplies their settings, and must agree on every setting except `*.events` and `*.seed`. |
| `<stage>.slurm.<key>` | none (driver) | The value of `@<key>@` in `slurm_job.sh`, e.g. `fit.slurm.time`. See [SLURM details](#slurm-details). |

**Simulation (`sim.`)**

| Key | Default | Meaning |
|---|---|---|
| `sim.events` | 500 (driver) | Events to simulate. Digi and fit process every event they receive. |
| `sim.chunks` | 1 (driver) | Split the sim, and everything that reads it, into this many independent pieces, at most 100. See [chunks and seeds](#chunks-and-seeds). |
| `sim.ions.zmin`, `sim.ions.zmax` | 26, 59 | Fragment Z range that can be simulated and fit. Every Z needs tables. Both fragments of a split must be in range, so `zmin + zmax` must equal `zcn`, or every stage stops with an error. |
| `sim.ions.zcn`, `sim.ions.acn` | 85, 204 | Compound nucleus (²⁰⁴At). The simulation fissions it, the fit assumes it, and `plot_fit` scales its ranges by it. A fragment's A is `round(Z / zcn * acn)`. The fit takes these from its sim, so it can only assume the sim's nucleus. |
| `sim.zToSim` | 50 | Sets the mean fragment mass, as the fraction `zToSim / zcn` of the compound nucleus. That mass must belong to a Z in `sim.ions.zmin`–`sim.ions.zmax`, or the sim stops with an error. |
| `sim.massDev` | 0 | Width of the fragment mass distribution (amu). 0 gives a single split. |
| `sim.decayAngle` | 90 | CoM decay angle in degrees. 0 samples the measured angular distribution. |
| `sim.eloss` | LISE | Energy-loss tables used in simulation: `LISE` or `SRIM`. |
| `sim.beamE`, `sim.beamEsig` | 2700.13, 128.122 | Beam energy and spread at the window (MeV). |
| `sim.zCutoff` | 300 | Vertices are simulated from the window up to this distance from the pad plane (mm). |

**Digitization (`digi.`)**

| Key | Default | Meaning |
|---|---|---|
| `digi.lowGain` | 0.19 | Gain factor for the zapped low-gain pads. |
| `digi.respScale` | 0.0075 | Scaling applied to the measured pad response. |

**Fit (`fit.`)**

| Key | Default | Meaning |
|---|---|---|
| `fit.eloss` | same as `sim.eloss` | Energy-loss tables used in the fit. Set it only to study a table mismatch on purpose. |
| `fit.iter` | 100 | Monte Carlo iterations per round. |
| `fit.rounds` | 2 | Rounds. Each round narrows the parameter distributions around the best result. |
| `fit.threads` | 4 | Threads per fit job. |
| `fit.psaThreshold` | 25 | PSA threshold for the fit's simulated events. |
| `fit.timeEvent` | 0 | 1 logs the time per round, for benchmarking. |

Misspelled settings are errors. The driver rejects misplaced keys (no stage prefix in `[defaults]`, a prefix inside a section, `input` in a sim, `events` or `chunks` outside a sim, a `slurm.` key that isn't `slurm.<letters, digits, _>`). Each macro calls `cfg.CheckUnused(...)` before running and fails on any setting it was passed but never read, except for the earlier stages' prefixes it lists.

## Command-line options

```bash
./fission.py STUDY.ini [SECTION ...] [options]
```

| Option | Effect |
|---|---|
| `SECTION ...` | Only these sections (default: all). `NAME` selects every section with that name, `NAME.STAGE` (e.g. `90deg.digi`, spelled like its files) one section. Their inputs in this study come up to date first. |
| `--stage sim\|digi\|fit` | Only sections of this stage (repeatable). A filter on the selection: `--stage sim` runs only the sims, and `--stage digi` the digis and the sims they read. |
| `--dry-run` | Show what would run and why, then stop. |
| `--status` | Show one row per section: its input and its state, `done` or how many chunks are `missing`, `changed`, `input is newer`, `upstream rerun` or `queued` in SLURM, then stop. |
| `--show` | Print each section's settings, with where each comes from (`section`, `defaults`, or a driver default), what it reads (and what that reads), what its macro would be passed (chunk 0's `.cfg.in`), and its `slurm_job.sh` values with where each comes from, then stop. |
| `-j N` | Run up to N jobs at once (default 1). |
| `--force sim\|digi\|fit` | Rerun this stage and the later ones, even if up to date. Never reaches another study's files. |
| `--slurm` | Submit to SLURM instead of running here. See [Running on a cluster](#running-on-a-cluster). |
| `--slurm-set [STAGE.]slurm.KEY=VALUE` | A value for `slurm_job.sh` (repeatable; without `STAGE`, for every stage). Lowest priority: the study overrides it. |
| `--results` | Fits also write a results file (`slim_fit.C`). `--remote` passes it. |
| `--remote NAME` | Copy the study files to the cluster `NAME` of `remotes.ini` and run the command there, after checking its checkout is at this commit. See [Running on a cluster](#running-on-a-cluster). |
| `--sync` | With `--remote NAME`, alone (no study): bring the cluster's checkout to this branch and commit, creating it if missing, and build it. |
| `--no-sync-check` | With `--remote NAME --slurm`: submit even if the cluster's checkout isn't at this commit, built. |
| `--fetch` | With `--remote`: copy the selected fits' finished results files to `$FISSION_DATA/remote/NAME/`. |
| `--fetch-logs` | With `--remote`: copy the selected sections' logs to `$FISSION_DATA/remote/NAME/`. |

## What reruns

The driver only runs jobs that are out of date and gives the reason for each:

| Reason | Meaning |
|---|---|
| `missing` | No output, or the last attempt did not finish. |
| `changed: sim.massDev 0->2` | A setting this section or its input chain depends on changed in the study file. |
| `input is newer` | The input was rerun since. |
| `upstream rerun` | The input is about to rerun. |
| `forced` | `--force` was given. |

A section depends on its own settings and on its input's, all the way back to the sim. Changing a `[fit]` setting refits only that fit. Changing a `[digi]` setting redoes the digi and every fit that reads it. Changing a `[sim]` setting redoes everything downstream of it.

Each stage's macro is passed its own settings plus every setting of its input, and of that input's input. A fit is therefore passed every `sim.*`, `digi.*` and `fit.*` setting of its chain, which is how a change anywhere in the chain reaches the fits that depend on it. `--show` prints what each section's macro is passed.

This is also how a fit learns about the simulation it is fitting: `fit.eloss` falls back to `sim.eloss`, and the fit uses the sim's `sim.ions.*`.

The driver compares the `.cfg` with what it would pass now to decide what to rerun. It does not see macro defaults, so changing a default in a macro reruns nothing (use `--force`).

## Files produced

All files go in `$FISSION_DATA`:

```
<study>-<name>.sim.root    simulated energy deposits + truth
<study>-<name>.digi.root   traces, hits, Y-pattern fits + truth
<study>-<name>.fit.root    fit results (AtMCResult), best simulated events + truth
<study>-<name>.<stage>.cfg     copy of the .cfg.in, written only on success
<study>-<name>.<stage>.cfg.in  settings passed to the macro
<study>-<name>.<stage>.log     full ROOT output
<study>-<name>.fit.results.root  small copy of the fit for plotting (--results, see Running on a cluster)
<study>-<name>.fit.results.cfg   copy of the fit's .cfg, written when the results file is complete
remote/<remote>/               results files (and logs) fetched from a cluster, same names
```

With `sim.chunks`, each name gets `.cNN` before the extension (`.fit.c03.results.root`). A fit always removes its old results file when it starts, so a results file never outlives its fit.

Each ROOT file also records every setting the stage ran with, including the macro defaults it used, as a `TNamed` called `RunConfig`:

```cpp
TFile f("data/smoke-base.fit.root"); cout << f.Get<TNamed>("RunConfig")->GetTitle();
```

## Chunks and seeds

`sim.chunks = N` splits a sim's events into N independent pieces (`.c00`, `.c01`, … up to `.c99`) that can run in parallel. Chunk k gets its share of `sim.events`. The digis and fits that read a chunked sim have one job per chunk too. A digi or fit has as many chunks as its input: one per chunk of the section it reads, or one per matched file, so an `input` may match at most 100 files.

Every stage has its own seed: `sim.seed`, `digi.seed` and `fit.seed`. Each defaults to a value derived from `<study>-<name>.<stage>`, so it is fixed for a given section, and chunk k uses seed + k.

Changing `sim.chunks` renames the files, so the sim is redone and its old files are removed. So are the old files of every section in the study that reads it, directly or through a digi, even with `--stage sim`, so no plot can mix chunk counts. `--dry-run` lists the files it would remove.

## SLURM details

Each job is `slurm_job.sh` with every `@name@` replaced, passed to `sbatch` on stdin (`scontrol write batch_script <id>` shows a submitted one). Edit the template freely. The driver fills in:

| Placeholder | Value |
|---|---|
| `@job@` | job name, `<study>-<name>.<stage>[.cNN]` |
| `@log@` | the job's `.log` |
| `@dir@` | this directory, where the job runs |
| `@cpus@` | `fit.threads` for a fit (default 4), 1 otherwise |
| `@command@` | the commands that run the stage (keep it) |

Every other `@key@` is the job's `slurm.<key>` value. There are no defaults: each must be set by you, from lowest to highest priority:

1. the remote's `[<stage>.]slurm.<key>` in `remotes.ini`, or `--slurm-set` on the cluster,
2. the study's `[defaults]`, as `<stage>.slurm.<key>`,
3. the section, as `slurm.<key>`.

So a study can give one section more time. The values only change the job script and are never passed to a macro, so changing them never reruns anything. `--show` prints each section's values and where each comes from. Before the first `sbatch`, the driver checks every job: it stops if the template uses a value a section doesn't set, or a section has a `slurm.<key>` value the template never uses (usually a typo). Nothing is submitted until all jobs pass.

To add an `sbatch` option, add a line to the template, e.g. `#SBATCH --partition=@partition@`, and set `slurm.partition` in `remotes.ini` or the study. Comment lines (starting with `#` but not `#SBATCH`, so also `##SBATCH`) are left as they are, so a placeholder there needs no value. Jobs that wait for another job get `--dependency=afterok:<id> --kill-on-invalid-dep=yes` on the `sbatch` command line.

The studies in `studies/` set no `slurm` values: those belong to you and the cluster, in `remotes.ini`.

While any job of a section whose files it would write or remove is queued or running, the driver refuses to run, with or without `--slurm`. Those sections are the ones it would rerun, plus every section of the study that reads them. Jobs are matched by name (`<study>-<name>.<stage>`, with or without `.cNN`), so this covers jobs from an old chunk count too, even ones that haven't started. Their files are never removed or written over while they are still pending. Wait for them, or `scancel` them first. On a machine without `squeue` the check is skipped. Sections in other studies that read them are not checked: don't rerun a section while another study's jobs that read it are queued or running.

## Plots

The histograms are globals in `plot_fit.C`: `zHist`, `aHist`, `hMR`, `hAmp`, `hObj`, `hObjPos`, `hObjQ`, `hBeam`, `hZvsObj`, `hZvsAmp`, `hAmpvsPosObj`, `hAmpvsObj`, `hAmpvsLoc`, `zHistSim`, `hZvsObjSim`, `zHistDiff`. `FillPlots(...)` refills them with different cuts.

A study argument `"<remote>:<study>"`, e.g. `plot_fit("hpc:decay_angle", "90deg")`, reads the results files fetched from that remote (`$FISSION_DATA/remote/<remote>/<study>-<name>.fit[.cNN].results.root`) instead of the fit files. The fits are still taken from `studies/<study>.ini` here, and `group_fit_plots` writes to `./.groups/<remote>/<study>/`. The histograms are the same, since results files have the fit file's branch names.

`group_fit_plots` output is temporary and rewritten on every call, so it goes to the hidden `./.groups/` (not committed).

The plot names for `show_groups` are `Z`, `A`, `Amp`, `Obj`, `ObjPos`, `ObjQ`, `MR`, `Beam`, `ZvsObj`, `ZvsAmp`, `AmpvsPosObj`, `AmpvsObj`, `AmpvsLoc`, `ZvsObjSim`, `ZSim` and `ZDiff`.

## Troubleshooting

**`FAILED <job>, see <log>`.** Open the log; the error is usually near the end. Common causes:

- A missing table, for example `.../eLoss/LISE/83_200.txt`. Check `TPC_SHARED_INFO` and the [layout](#setup).
- `TPC_SHARED_INFO is not set`. Set it.
- An input file that can't be opened, because the stage before it failed. Rerunning the study retries from the first failed job.

**`Settings never read by this stage (typo?)`.** A misspelled key in the study file. Fix the spelling.

**`unknown stage`.** A misspelled stage prefix in `[defaults]`, such as `simm.events`. Fix the spelling.

**`unknown section`, `needs a stage prefix`, `have no stage prefix`.** The study file is in the wrong shape, often an old one with `[run <name>]` sections, unprefixed `events`/`seed`, or `upstream`. Rewrite it as `[sim|digi|fit <name>]` sections (see [Writing a study](#writing-a-study)).

**`[fit x] reads [digi x] by default, but there is none`.** A digi or fit has no `input` and no same-named section of the previous stage. Add that section, or set `input`.

**`has no .cfg next to it`.** An `input` file is unfinished or was not made by a stage macro. Each file needs the `.cfg` its stage writes when it finishes.

**`was made with different settings than`.** The files an `input` glob matches must agree on every setting except `*.events` and `*.seed`. Narrow the glob.

**`was made before study files had stage sections`.** An `input` file's `.cfg` uses the old unprefixed keys. Rerun the file's study, or make the file again.

**`Still queued or running in SLURM`.** Jobs from an earlier `--slurm` submission are still in `squeue`. Wait for them, or `scancel` them.

**`Bring other studies' sections up to date first`.** A section reads another study's section (`input = <study>:<name>`), and that section or something it reads is missing, queued, or out of date with its study file. Run the printed commands, wait for them to finish, then run this study again.

**`No remotes.ini` / `remotes.ini has no [NAME]`.** Copy `remotes.ini.example` to `remotes.ini` and fill in a section for the cluster.

**`slurm_job.sh uses @time@, but slurm.time is not set`.** No value for a placeholder of `slurm_job.sh`. Set it in `remotes.ini`, with `--slurm-set`, or in the study (see [SLURM details](#slurm-details)).

**`slurm_job.sh never uses @foo@`.** A `slurm.foo` value with no `@foo@` in the template. Fix the spelling, or add a line using it to `slurm_job.sh`.

**`hpc is at …, you are at …` / `hpc's build is …` / `--slurm` stops with `Run ./fission.py --remote hpc --sync`.** The cluster's checkout isn't at your commit, or wasn't built there by `--sync`, so the study would run other code than yours. Run `./fission.py --remote hpc --sync`, or add `--no-sync-check` to submit anyway. `hpc's build is not from a finished --sync` also follows a failed `--sync` build.

**`hpc has no checkout at …`.** `repo` doesn't exist yet. Create it with `./fission.py --remote hpc --sync`.

**`hpc has local changes`.** A tracked file outside `studies/` was edited in the cluster's checkout. Commit it there (and bring it into your branch) or discard it (`git -C <repo> checkout -- <file>`), then `--sync`.

**`hpc has fission jobs in SLURM`.** `--sync` won't change the code under queued or running jobs. Wait for them, or `scancel` them.

**`hpc's <branch> is at …, which has commits yours doesn't`.** Someone committed on the cluster's branch. Bring those commits into your branch (or reset the cluster's branch to one of yours), then `--sync`.

**`has no configured build (build/config.sh), and … doesn't set`.** A new (or never configured) checkout needs `env.SIMPATH`, `env.FAIRROOTPATH` and `cmake.CMAKE_PREFIX_PATH` in `remotes.ini` (see [Running on a cluster](#running-on-a-cluster)).

**`hpc: configure failed` / `hpc: build failed, see <repo>/build/fission-sync.log`.** The end of the log is printed. Fix the cause (often a `cmake.*` or `env.*` value) and run `--sync` again: it configures again only if `build/config.sh` is still missing, and the build picks up where it stopped. To configure from scratch, remove `<repo>/build` on the cluster first.

**`ssh: connect to host … Connection timed out` / `Permission denied`.** The driver runs ssh with `BatchMode=yes` and a 30 s connection timeout, so it fails instead of prompting or hanging. Check that `ssh <host>` works without a prompt, and that the VPN is up if the cluster needs one.

**Everything reruns when I didn't expect it.** Run `--dry-run` and read the reason. `--show NAME` shows where each of the section's settings comes from. Changing `[defaults]` affects every section of that stage, and a sim change redoes everything that reads it.

**Nothing reruns after I edited a macro.** The driver tracks settings, not code. Use `--force <stage>`.

**The plot has no files.** The error names the directory `plot_fit` searched. A relative `FISSION_DATA` is relative to where ROOT started, so start ROOT where you ran `fission.py`, with the same `FISSION_DATA`, or use an absolute path.

**Log errors that are normal.** These appear in every run and are not failures:

- `FairBaseParSet not initialized` / `Error occured during initialization`
- `Position of particle … is not in active volume`
- `Hits that are points (sig_z = 0) are not supported yet!`

**Old files (`output_fitNN.root`).** Files made before this driver don't plot with the current `plot_fit.C`. Use the version from git history.

## Adding a setting

To expose a new setting, read it in the macro with `cfg.Get("stage.name", default)`, `cfg.GetInt` or `cfg.GetStr`, and give it the prefix of the stage that reads it. A later stage that needs it (as the fit needs `sim.ions.*`) reads the same key, which it is passed through its input.

## Appendix: Known Paths by Machine

Paths seen in this code's history or found by searching, as of 2026-09. Each is specific to one machine and may be stale. Entries marked "from history" were never checked.
**Adam's laptop (WSL)** (checked 2026-10)
- Shared info: `/home/adam/fair_install/tpcSharedInfo/`. Complete, including `eLoss/LISE/83_200.txt`. The LISE and SRIM sets list different fragments, and SRIM has no `83_200.txt`.

**Faculty lab machine** (from history; the `physics` and `skyler` users are the same machine)
- Shared info: `/home/skyler/fission/tpcSharedInfo/`. Includes `83_200.txt`.
- Data: `/mnt/tpc-data/`

**Faculty cluster, `aanthony`** (from history; SLURM, `cs` partition)
- Shared info: `/home/faculty/aanthony/fission/data/e12014/tpcSharedInfo/`
- Unpacked e12014 data: `/home/faculty/aanthony/fission/data/e12014/unpacked/`
- Repo checkout: `/home/faculty/aanthony/fission/adam/ATTPCROOTv2/`

**FRIB analysis machines** (from history; experiment data, not shared info)
- `/mnt/analysis/e12014/TPC/<pressure>Torr/`
- `/mnt/simulations/attpcroot/adam/`
