#!/usr/bin/env python3
"""Run the fission-fit validation chain (sim -> digi -> fit) for the runs of a study file.

A study file (studies/*.ini) holds shared [defaults] and one [run <name>] section per run. Keys are
prefixed by the stage that uses them (sim., digi., fit.). ions.*, events and seed are used by every
stage. chunks splits a run's events into independent pieces that can run in parallel. A run with
`upstream = <other run>` reuses that run's sim and digi output and only refits. The upstream may be in
another study (`upstream = <study>:<run>`) or be existing digi files (`upstream = <path or glob>`,
relative to FISSION_DATA). Those are never run or removed from here: they must already be up to date.

Each stage writes <data>/<study>-<run>.<stage>[.cNN].root and, when it finishes, a .cfg file with the
settings it used. A stage is rerun if its output is missing, its settings (or any upstream settings)
changed, or its input is newer than its output.

    ./fission.py studies/smoke.ini --dry-run       # show what would run and why
    ./fission.py studies/smoke.ini --show base     # show a run's resolved settings and stage inputs
    ./fission.py studies/smoke.ini --status        # show which stages of each run are done
    ./fission.py studies/smoke.ini -j 4            # run everything that is stale, 4 jobs at a time
    ./fission.py studies/smoke.ini 90deg --stage digi
    ./fission.py studies/smoke.ini --force fit     # rerun fit even if up to date
    ./fission.py studies/smoke.ini --slurm         # submit the jobs with sbatch instead

Environment: FISSION_DATA (default ./data next to this script), TPC_SHARED_INFO (tables, response).
"""
import argparse
import configparser
import glob
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
import zlib
from collections import Counter, namedtuple
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait

HERE = os.path.dirname(os.path.abspath(__file__))
STAGES = ["sim", "digi", "fit"]
MACROS = {"sim": "run_simp_fiss.C", "digi": "run_digi_fiss.C", "fit": "run_fit.C"}
GLOBAL_KEYS = ("events", "seed")  # Plus everything starting with "ions."
FIT_THREADS_DEFAULT = "4"  # run_fit.C's default for fit.threads, so SLURM requests what the fit uses
# SLURM time limit and memory per stage, overridable with slurm.<stage>.time / slurm.<stage>.mem. Rough
# upper bounds for a 500-event chunk, not measurements: lower them once bench.ini has timings.
SLURM_DEFAULTS = {
    "sim": {"time": "0:10:00", "mem": "4G"},
    "digi": {"time": "8:00:00", "mem": "8G"},
    "fit": {"time": "3-00:00:00", "mem": "8G"},
}
DATA = os.path.abspath(os.environ.get("FISSION_DATA", os.path.join(HERE, "data")))
RUN_NAME = r"[A-Za-z0-9_-]+"
STUDY_NAME = r"[A-Za-z0-9_]+"  # No - in study names, so <study>-<run> splits only one way
# Stage outputs are <study>-<run>.<stage>[.cNN] plus one of OUTPUT_EXTS (also the SLURM job name)
JOB_NAME = re.compile(rf"(?P<study_run>{STUDY_NAME}-{RUN_NAME})\.(?:{'|'.join(STAGES)})(?:\.c\d\d)?")
OUTPUT_EXTS = (".root", ".cfg", ".cfg.in", ".log")


def used_by(key, stage):
    """True if a setting affects the output of stage (its own keys, upstream keys, and globals)."""
    if key in GLOBAL_KEYS or key.startswith("ions."):
        return True
    prefix = key.split(".", 1)[0]
    return prefix in STAGES and STAGES.index(prefix) <= STAGES.index(stage)


def is_slurm_key(key):
    """True for slurm.<stage>.time and slurm.<stage>.mem, which only set sbatch options."""
    parts = key.split(".")
    return len(parts) == 3 and parts[0] == "slurm" and parts[1] in STAGES and parts[2] in ("time", "mem")


def fit_default_key(key):
    """True for the [defaults] settings that reach a run fitting another study's or a file's digi."""
    return key.startswith("fit.") or is_slurm_key(key)


def is_upstream_file(upstream):
    """True if upstream names digi files (a path or glob) rather than a run."""
    return "/" in upstream or upstream.endswith(".root")


def derived_seed(name):
    return str(zlib.crc32(name.encode()) & 0x7FFFFFFF or 1)


def short_path(path):
    """path relative to the current directory if it is below it, for messages to copy and paste."""
    rel = os.path.relpath(path)
    return path if rel.startswith("..") else rel if os.sep in rel else os.path.join(".", rel)


def stem(study, run, stage, chunk, nchunks):
    """Path of a stage output without extension. Unchunked runs have no .cNN."""
    return os.path.join(DATA, f"{study}-{run}.{stage}" + (f".c{chunk:02d}" if nchunks > 1 else ""))


def sidecar(output):
    """The .cfg a stage writes next to its .root output when it finishes."""
    return output[: -len(".root")] + ".cfg"


def read_cfg(path):
    """Parse a key = value file (the format RunConfig.h reads and writes)."""
    values = {}
    with open(path) as f:
        for line in f:
            if "=" in line and not line.startswith("#"):
                key, val = line.split("=", 1)
                values[key.strip()] = val.strip()
    return values


Resolved = namedtuple("Resolved", "settings origins owner")
_cache = {}


def load_study(path):
    """The Study of an .ini file, parsed once, so runs of one study compare as the same owner."""
    path = os.path.abspath(path)
    if path not in _cache:
        _cache[path] = Study(path)
    return _cache[path]


def load_digi_files(pattern):
    if ("files", pattern) not in _cache:
        _cache[("files", pattern)] = DigiFiles(pattern)
    return _cache[("files", pattern)]


class Study:
    def __init__(self, path):
        self.path = path
        self.name = os.path.splitext(os.path.basename(path))[0]
        parser = configparser.ConfigParser(inline_comment_prefixes=(";", "#"))
        parser.optionxform = str  # Keep key case (sim.massDev)
        parser.read(path)
        self.defaults = dict(parser["defaults"]) if parser.has_section("defaults") else {}
        self.sections = {s.split(None, 1)[1]: dict(parser[s]) for s in parser.sections() if s.startswith("run ")}
        if not self.sections:
            sys.exit(f"{path}: no [run <name>] sections")
        # Names end up in file names (<study>-<run>.<stage>), SLURM job names and ROOT command lines.
        if not re.fullmatch(STUDY_NAME, self.name):
            sys.exit(f"{path}: study name {self.name!r} may only use letters, digits and _")
        for run in self.sections:
            if not re.fullmatch(RUN_NAME, run):
                sys.exit(f"{path}: run name {run!r} may only use letters, digits, _ and -")
        for section, values in [("defaults", self.defaults)] + [(f"run {r}", v) for r, v in self.sections.items()]:
            for key in values:
                if not (used_by(key, STAGES[-1]) or key == "chunks" or is_slurm_key(key)
                        or (key == "upstream" and section != "defaults")):
                    sys.exit(f"{path}: [{section}] has unknown setting {key} (use a {', '.join(STAGES)} or ions. prefix,"
                             f" or slurm.<stage>.time/mem)")
            upstream = values.get("upstream")
            if upstream is not None and not (re.fullmatch(RUN_NAME, upstream) or is_upstream_file(upstream)
                                             or re.fullmatch(f"{STUDY_NAME}:{RUN_NAME}", upstream)):
                sys.exit(f"{path}: [{section}] upstream = {upstream} is not a run, <study>:<run>, or digi file path")

    def owner_of(self, upstream):
        """The owner of the sim and digi output an upstream setting names: (Study, run) for <run> or
        <study>:<run>, or DigiFiles for a path or glob."""
        if is_upstream_file(upstream):
            return load_digi_files(upstream)
        if ":" not in upstream:
            return self, upstream
        name, run = upstream.split(":")
        if name == self.name:
            sys.exit(f"{self.path}: upstream = {upstream} is in this study, write upstream = {run}")
        path = os.path.join(os.path.dirname(self.path), name + ".ini")
        if not os.path.exists(path):
            sys.exit(f"{self.path}: upstream = {upstream}, but there is no {path}")
        study = load_study(path)
        if run not in study.sections:
            sys.exit(f"{self.path}: upstream = {upstream}, but {path} has no [run {run}]")
        return study, run

    def resolve(self, run):
        """A run's settings, where each comes from (for --show), and the owner of its sim and digi
        output: (Study, run), which may be another study, or DigiFiles for a run that fits files."""
        if run not in self.sections:
            sys.exit(f"{self.name}: no run named {run}")
        section = dict(self.sections[run])
        upstream = section.pop("upstream", None)
        owner = self.owner_of(upstream) if upstream else (self, run)
        files = isinstance(owner, DigiFiles)
        settings, origins = {}, {}

        def add(values, origin):
            """Add values over the settings so far. origin labels them all, or is a label per key."""
            settings.update(values)
            origins.update(origin if isinstance(origin, dict) else dict.fromkeys(values, origin))

        if not files:  # Files without an event count: each chunk takes its file's
            add({"events": "500"}, "driver default")
        add({"seed": derived_seed(f"{self.name}-{run}")}, "from study and run name")
        if not upstream:
            add(self.defaults, "defaults")
        else:
            if files:
                up_settings, up_origins = owner.settings, "digi files"
            else:
                up = owner[0].resolve(owner[1])
                up_settings, up_origins = up.settings, {k: f"{upstream}: {o}" for k, o in up.origins.items()}
            add(up_settings, up_origins)
            for key, val in section.items():
                if (used_by(key, "digi") or key == "chunks") and val != up_settings.get(key):
                    sys.exit(f"{self.name}: run {run} changes {key} but reuses sim/digi of {upstream}")
            if files or owner[0] is not self:
                # Another study's or a file's run starts from its own fit settings, then this study's fit defaults
                add({k: v for k, v in self.defaults.items() if fit_default_key(k)}, "defaults")
        add(section, "run")
        return Resolved(settings, origins, owner)

    def owner_name(self, owner):
        """How this study refers to a sim/digi owner: <run>, <study>:<run>, or the file pattern."""
        if isinstance(owner, DigiFiles):
            return owner.pattern
        study, run = owner
        return run if study is self else f"{study.name}:{run}"


class DigiFiles:
    """Existing digi files a run fits (upstream = <path or glob>, relative to FISSION_DATA), one per
    chunk, with the settings recorded in their .cfg. The driver never reruns or removes them."""

    def __init__(self, pattern):
        self.pattern = pattern
        self.paths = sorted(glob.glob(os.path.join(DATA, pattern)))  # An absolute pattern ignores DATA
        if not self.paths:
            sys.exit(f"upstream = {pattern}: no files match in {DATA}")
        self.chunk_settings = []
        for path in self.paths:
            if not (path.endswith(".root") and os.path.exists(sidecar(path))):
                sys.exit(f"upstream = {pattern}: {path} has no .cfg next to it (unfinished, or not a stage output)")
            cfg = read_cfg(sidecar(path))
            self.chunk_settings.append({k: v for k, v in cfg.items() if k not in ("input", "output")})
        # Chunks differ only in their events and seed
        common = [{k: v for k, v in s.items() if k not in ("events", "seed")} for s in self.chunk_settings]
        for path, values in zip(self.paths[1:], common[1:]):
            if values != common[0]:
                sys.exit(f"upstream = {pattern}: {os.path.basename(path)} was made with different settings"
                         f" than {os.path.basename(self.paths[0])}")
        self.settings = dict(common[0], chunks=str(len(self.paths)))
        if all("events" in s for s in self.chunk_settings):
            self.settings["events"] = str(sum(int(s["events"]) for s in self.chunk_settings))
        if "seed" in self.chunk_settings[0]:
            self.settings["seed"] = self.chunk_settings[0]["seed"]


class Job:
    """One stage of one chunk of one run.

    input is the previous stage's output (None for sim). chunk_cfg, for a fit of existing digi files,
    is that chunk's file's settings: its events and seed are used instead of a share of the run's."""

    def __init__(self, study, run, stage, chunk, settings, input=None, chunk_cfg=None):
        self.study, self.run, self.stage, self.chunk = study, run, stage, chunk
        self.nchunks = nchunks = int(settings.get("chunks", 1))
        self.base = stem(study, run, stage, chunk, nchunks)
        self.output = self.base + ".root"
        self.sidecar = sidecar(self.output)
        self.log = self.base + ".log"
        self.deps = []
        self.external = None  # The Study that owns this job, if it isn't the one being run
        self.slurm = {opt: settings.get(f"slurm.{stage}.{opt}", default) for opt, default in SLURM_DEFAULTS[stage].items()}

        cfg = {k: v for k, v in settings.items() if used_by(k, stage)}
        if chunk_cfg is None:
            events = int(settings["events"])
            cfg["events"] = str(events // nchunks + (chunk < events % nchunks))
            cfg["seed"] = str(int(settings["seed"]) + chunk)
        else:
            cfg.pop("events", None)
            cfg.update({k: chunk_cfg[k] for k in ("events", "seed") if k in chunk_cfg})
            cfg.setdefault("seed", str(int(settings["seed"]) + chunk))
        cfg["output"] = self.output
        if input:
            cfg["input"] = input
        self.cfg = cfg

    @property
    def key(self):
        return (self.study, self.run, self.stage, self.chunk)

    def __str__(self):
        return os.path.basename(self.base)

    def stale_reason(self):
        if not (os.path.exists(self.output) and os.path.exists(self.sidecar)):
            return "missing"
        done = read_cfg(self.sidecar)
        changed = [f"{k} {done.get(k, '-')}->{self.cfg.get(k, '-')}"
                   for k in sorted(set(done) | set(self.cfg)) if done.get(k) != self.cfg.get(k)]
        if changed:
            return "changed: " + ", ".join(changed)
        if "input" in self.cfg and os.path.exists(self.cfg["input"]) and \
                os.path.getmtime(self.cfg["input"]) > os.path.getmtime(self.output):
            return "input is newer"
        return None

    def write_cfg(self):
        with open(self.base + ".cfg.in", "w") as f:
            f.write("".join(f"{k} = {v}\n" for k, v in self.cfg.items()))
        return self.base + ".cfg.in"

    def command(self):
        return ["root", "-l", "-b", "-q", f'{MACROS[self.stage]}("{self.write_cfg()}")']

    def run_local(self):
        """Run the macro. Success means the macro wrote its .cfg (RunConfig::Finish)."""
        if os.path.exists(self.sidecar):
            os.remove(self.sidecar)
        start = time.time()
        with open(self.log, "w") as log:
            subprocess.run(self.command(), cwd=HERE, stdout=log, stderr=subprocess.STDOUT)
        ok = os.path.exists(self.sidecar)
        print(f"{'done' if ok else 'FAILED':6} {self} ({time.time() - start:.0f} s)"
              + ("" if ok else f", see {self.log}"), flush=True)
        return ok


def plan(study, runs, last_stage, force):
    """The jobs needed to bring every run up to last_stage, in dependency order, and the jobs of other
    studies that are out of date and must be run there first."""
    jobs = [j for j in plan_all(study, runs, last_stage, force).values() if j.reason]
    return [j for j in jobs if not j.external], [j for j in jobs if j.external]


def plan_all(study, runs, last_stage, force):
    """Every job of the runs up to last_stage, keyed (study, run, stage, chunk) so shared upstream appears
    once. job.reason says why a job must run, or is None if it is up to date. Sim and digi jobs of
    another study are included with job.external set. Digi files have no jobs: the fit reads them."""
    jobs = {}
    for run in runs:
        settings, _, owner = study.resolve(run)
        files = owner if isinstance(owner, DigiFiles) else None
        for chunk in range(int(settings.get("chunks", 1))):
            prev = None
            for stage in STAGES[: STAGES.index(last_stage) + 1]:
                if stage == "fit":
                    job_study, job_run = study, run
                elif files:
                    continue
                else:
                    job_study, job_run = owner
                key = (job_study.name, job_run, stage, chunk)
                if key in jobs:  # sim/digi shared with an upstream run, already planned
                    prev = key
                    continue
                input = jobs[prev].output if prev else files.paths[chunk] if files and stage == "fit" else None
                job = jobs[key] = Job(job_study.name, job_run, stage, chunk, settings, input,
                                      files.chunk_settings[chunk] if files else None)
                if job_study is not study:
                    job.external = job_study
                reason = job.stale_reason()
                if force and not job.external and STAGES.index(stage) >= STAGES.index(force):
                    reason = "forced"
                if prev is not None and jobs[prev].reason:
                    reason = reason or "upstream rerun"
                    job.deps.append(jobs[prev])
                job.reason = reason
                prev = key
    return jobs


def status(study, runs):
    """Print a table of each run's stages: done, or how many chunks are missing, stale or in SLURM."""
    queued = slurm_jobs()
    rows = []
    for run in runs:
        owner = study.resolve(run).owner
        stage_jobs = {stage: [] for stage in STAGES}  # Sim and digi stay empty for a fit of digi files
        for job in plan_all(study, [run], STAGES[-1], None).values():
            stage_jobs[job.stage].append(job)
        cells = []
        for jobs in stage_jobs.values():
            counts = Counter("queued" if str(job) in queued else (job.reason or "done").split(":")[0] for job in jobs)
            if not jobs:
                cells.append("-")
            elif counts == {"done": len(jobs)}:
                cells.append("done")
            elif len(jobs) == 1:
                cells.append(next(iter(counts)))
            else:
                cells.append(", ".join(f"{n} {state}" for state, n in sorted(counts.items())))
        rows.append([run + (f" (from {study.owner_name(owner)})" if owner != (study, run) else "")] + cells)
    header = ["run"] + STAGES
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(f"{cell:{w}}" for cell, w in zip(row, widths)).rstrip())
    done = sum(all(c in ("done", "-") for c in row[1:]) for row in rows)
    print(f"\n{done} of {len(rows)} runs finished." + (" --dry-run shows why the rest would run." if done < len(rows) else ""))


def show(study, runs, last_stage):
    """Print each run's resolved settings, with where each comes from, and what each stage is passed."""
    for run in runs:
        settings, origins, owner = study.resolve(run)
        if isinstance(owner, DigiFiles):
            print(f"[run {run}]  (fits {len(owner.paths)} digi files matching {owner.pattern})")
        else:
            print(f"[run {run}]" + (f"  (sim and digi from {study.owner_name(owner)})" if owner != (study, run) else ""))
        width = max(len(f"{k} = {v}") for k, v in settings.items())
        for key in sorted(settings):
            print(f"  {f'{key} = {settings[key]}':{width}}  ; {origins[key]}")
        nchunks = int(settings.get("chunks", 1))
        for job in plan_all(study, [run], last_stage, None).values():
            if job.chunk != 0:
                continue
            print(f"  {os.path.basename(job.base)}.cfg.in" + (f" (chunk 0 of {nchunks})" if nchunks > 1 else "")
                  + (f" (owned by {job.external.name}, not run from here)" if job.external else ""))
            for key, val in job.cfg.items():
                print(f"    {key} = {val}")
            if not job.external:
                print(f"    (--slurm: --time={job.slurm['time']} --mem={job.slurm['mem']})")
        print()
    print("Settings not passed to a stage use the macro's built-in default.")


def affected_runs(study, todo):
    """Runs whose files the jobs in todo replace: the runs owning the sim/digi output being rerun, and
    every run that refits it."""
    owners = {study.resolve(job.run).owner for job in todo}  # todo holds only this study's jobs
    return sorted(run for run in study.sections if study.resolve(run).owner in owners)


def old_chunking(study, runs):
    """Outputs (with their .cfg, .cfg.in and .log) of runs left over from a different chunk count.

    Every stage is checked, not only the planned ones: after `--stage sim` with a new chunk count, the
    old digi and fit chunks would otherwise stay behind and be picked up by the plots."""
    old = []
    for run in runs:
        nchunks = int(study.resolve(run).settings.get("chunks", 1))
        for stage in STAGES:
            current = {stem(study.name, run, stage, c, nchunks) for c in range(nchunks)}
            for path in sorted(glob.glob(stem(study.name, run, stage, 0, 1) + "*.root")):
                base = path[: -len(".root")]
                if base not in current and JOB_NAME.fullmatch(os.path.basename(base)):
                    old += [base + ext for ext in OUTPUT_EXTS if os.path.exists(base + ext)]
    return old


def run_local(todo, n_jobs):
    """Run jobs in dependency order, up to n_jobs at a time. Dependents of failed jobs are skipped."""
    pending, running, finished, failed = list(todo), {}, set(), set()
    with ThreadPoolExecutor(n_jobs) as pool:
        while pending or running:
            for job in list(pending):
                if any(d in failed for d in job.deps):
                    pending.remove(job)
                    failed.add(job)
                    print(f"skip   {job} (upstream failed)")
                elif all(d in finished for d in job.deps) and len(running) < n_jobs:
                    pending.remove(job)
                    print(f"start  {job}", flush=True)
                    running[pool.submit(job.run_local)] = job
            done, _ = wait(running, return_when=FIRST_COMPLETED)
            for future in done:
                job = running.pop(future)
                (finished if future.result() else failed).add(job)
    return not failed


def job_run_name(name):
    """<study>-<run> of a job name <study>-<run>.<stage>[.cNN], or None if it isn't one."""
    match = JOB_NAME.fullmatch(name)
    return match and match["study_run"]


def slurm_jobs():
    """Names of your queued or running SLURM jobs (empty without SLURM)."""
    if shutil.which("squeue") is None:
        return set()
    result = subprocess.run(["squeue", "-h", "--me", "-o", "%j"], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"warning: squeue failed, not checking for queued jobs: {result.stderr.strip()}", file=sys.stderr)
        return set()
    return set(result.stdout.split())


def run_slurm(todo):
    """Submit each job with sbatch, chained with afterok dependencies.

    As in run_local, success means the macro wrote its .cfg, not ROOT's exit code: the wrapper removes
    the old .cfg first and the job's exit status is whether a new one exists. The .cfg is also removed
    at submission, so a queued job is never reported up to date. Jobs whose dependency failed are
    cancelled by SLURM instead of staying pending.
    """
    if shutil.which("sbatch") is None:
        sys.exit("sbatch not found")
    ids = {}
    for job in todo:  # plan() returns jobs in dependency order
        threads = job.cfg.get("fit.threads", FIT_THREADS_DEFAULT) if job.stage == "fit" else "1"
        cmd = ["sbatch", "--parsable", f"--job-name={job}", f"--output={job.log}",
               f"--cpus-per-task={threads}", f"--time={job.slurm['time']}", f"--mem={job.slurm['mem']}",
               f"--chdir={HERE}"]
        if job.deps:
            cmd.append("--dependency=afterok:" + ":".join(ids[d.key] for d in job.deps))
            cmd.append("--kill-on-invalid-dep=yes")
        if os.path.exists(job.sidecar):
            os.remove(job.sidecar)
        quoted = shlex.quote(job.sidecar)
        cmd.append(f"--wrap=rm -f {quoted}; {shlex.join(job.command())}; test -f {quoted}")
        ids[job.key] = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.strip().split(";")[0]
        print(f"submitted {job} as {ids[job.key]}")


def blockers(study, external, affected, queued):
    """Why the jobs can't run yet, as messages: runs in the way of SLURM jobs, and other studies' stale jobs."""
    messages = []
    # Jobs still in SLURM own their files: don't run them again here or delete their outputs. This
    # includes jobs from any chunk count, whose files may not exist yet.
    planned = {f"{study.name}-{run}" for run in affected}
    busy = sorted(name for name in queued if job_run_name(name) in planned)
    if busy:
        messages.append("Still queued or running in SLURM (wait, or scancel them): " + ", ".join(busy))
    if external:
        # Other studies' files are theirs to run: running them from here would change the inputs of
        # their own fits behind their back
        owners = sorted({(job.external.path, job.run) for job in external})
        messages.append("Bring the sim and digi of other studies up to date first:\n" + "\n".join(
            f"  {shlex.quote(short_path(os.path.join(HERE, 'fission.py')))} {shlex.quote(short_path(path))} {run}"
            " --stage digi" for path, run in owners))
    return messages


def print_plan(jobs, old, queued, remove):
    """Print why each job runs, and the files of an old chunk count that are (or would be) removed."""
    for job in jobs:
        print(f"{str(job):40} {job.reason}" + (" (queued in SLURM)" if str(job) in queued else "")
              + (f" (owned by {job.external.name})" if job.external else ""))
    for path in old:
        print(f"{'remove' if remove else 'would remove'} {os.path.basename(path)} (old chunk count)")


def update(study, runs, args):
    """Run, or with --dry-run only list, the jobs that bring the runs up to args.stage."""
    todo, external = plan(study, runs, args.stage, args.force)
    if not todo and not external:
        print("Everything is up to date.")
        return
    queued = slurm_jobs()
    affected = affected_runs(study, todo)
    old = old_chunking(study, affected)  # Removed so plots don't pick them up
    messages = blockers(study, external, affected, queued)
    print_plan(external + todo, old, queued, remove=not (args.dry_run or messages))
    if args.dry_run:
        for message in messages:
            print(message)
        return
    if messages:
        sys.exit("\n".join(messages))

    os.makedirs(DATA, exist_ok=True)
    for path in old:
        os.remove(path)
    if args.slurm:
        run_slurm(todo)
    elif not run_local(todo, args.j):
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("study", help="study file (.ini)")
    parser.add_argument("runs", nargs="*", help="runs to process (default: all)")
    parser.add_argument("--stage", choices=STAGES, default="fit", help="last stage to run (default: fit)")
    parser.add_argument("--force", choices=STAGES, help="rerun from this stage down even if up to date")
    parser.add_argument("--dry-run", action="store_true", help="only show what would run")
    parser.add_argument("--show", action="store_true",
                        help="print the resolved settings of each run and what each stage is passed, then exit")
    parser.add_argument("--status", action="store_true",
                        help="print which stages of each run are done, missing, stale or in SLURM, then exit")
    parser.add_argument("-j", type=int, default=1, help="jobs to run at once (default: 1)")
    parser.add_argument("--slurm", action="store_true", help="submit jobs with sbatch")
    args = parser.parse_intermixed_args()

    study = load_study(args.study)
    runs = args.runs or list(study.sections)
    if args.show:
        show(study, runs, args.stage)
    elif args.status:
        status(study, runs)
    else:
        update(study, runs, args)


if __name__ == "__main__":
    main()
