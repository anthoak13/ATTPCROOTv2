#!/usr/bin/env python3
"""Run the fission-fit validation chain (sim -> digi -> fit) for the runs of a study file.

A study file (studies/*.ini) holds shared [defaults] and one [run <name>] section per run. Keys are
prefixed by the stage that uses them (sim., digi., fit.). ions.*, events and seed are used by every
stage. chunks splits a run's events into independent pieces that can run in parallel. A run with
`upstream = <other run>` reuses that run's sim and digi output and only refits.

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


def chunk_suffix(chunk, nchunks):
    """File name suffix of one chunk: none for an unchunked run, else .cNN."""
    return f".c{chunk:02d}" if nchunks > 1 else ""


def read_cfg(path):
    """Parse a key = value file (the format RunConfig.h reads and writes)."""
    values = {}
    with open(path) as f:
        for line in f:
            if "=" in line and not line.startswith("#"):
                key, val = line.split("=", 1)
                values[key.strip()] = val.strip()
    return values


class Study:
    def __init__(self, path):
        self.name = os.path.splitext(os.path.basename(path))[0]
        parser = configparser.ConfigParser(inline_comment_prefixes=(";", "#"))
        parser.optionxform = str  # Keep key case (sim.massDev)
        parser.read(path)
        self.defaults = dict(parser["defaults"]) if parser.has_section("defaults") else {}
        self.sections = {s.split(None, 1)[1]: dict(parser[s]) for s in parser.sections() if s.startswith("run ")}
        if not self.sections:
            sys.exit(f"{path}: no [run <name>] sections")
        # Names end up in file names (<study>-<run>.<stage>), SLURM job names and ROOT command lines.
        # No - in study names, so <study>-<run> splits only one way.
        if not re.fullmatch(r"[A-Za-z0-9_]+", self.name):
            sys.exit(f"{path}: study name {self.name!r} may only use letters, digits and _")
        for run in self.sections:
            if not re.fullmatch(r"[A-Za-z0-9_-]+", run):
                sys.exit(f"{path}: run name {run!r} may only use letters, digits, _ and -")
        for section, values in [("defaults", self.defaults)] + [(f"run {r}", v) for r, v in self.sections.items()]:
            for key in values:
                if not (used_by(key, STAGES[-1]) or key == "chunks" or is_slurm_key(key)
                        or (key == "upstream" and section != "defaults")):
                    sys.exit(f"{path}: [{section}] has unknown setting {key} (use a {', '.join(STAGES)} or ions. prefix,"
                             f" or slurm.<stage>.time/mem)")

    def settings(self, run):
        """Resolved settings for a run, and the run that owns its sim and digi output."""
        if run not in self.sections:
            sys.exit(f"{self.name}: no run named {run}")
        section = dict(self.sections[run])
        upstream = section.pop("upstream", None)
        settings = {**self.defaults, **section}
        owner = run
        if upstream:
            up_settings, owner = self.settings(upstream)
            settings = {**up_settings, **section}
            for key in settings:
                if not (used_by(key, "digi") or key == "chunks"):
                    continue
                if settings[key] != up_settings.get(key):
                    sys.exit(f"{self.name}: run {run} changes {key} but reuses sim/digi of {upstream}")
        settings.setdefault("seed", str(zlib.crc32(f"{self.name}-{owner}".encode()) & 0x7FFFFFFF or 1))
        settings.setdefault("events", "500")
        return settings, owner

    def origin(self, run, key):
        """Where a run's resolved setting comes from."""
        section = self.sections[run]
        if key in section:
            return "run"
        if "upstream" in section:
            return f"{section['upstream']}: {self.origin(section['upstream'], key)}"
        if key in self.defaults:
            return "defaults"
        return "from study and run name" if key == "seed" else "driver default"


class Job:
    """One stage of one chunk of one run."""

    def __init__(self, study, run, stage, chunk, settings, input_run):
        self.study, self.run, self.stage, self.chunk = study, run, stage, chunk
        self.nchunks = nchunks = int(settings.get("chunks", 1))
        suffix = chunk_suffix(chunk, nchunks)
        self.base = os.path.join(DATA, f"{study}-{run}.{stage}{suffix}")
        self.output = self.base + ".root"
        self.deps = []
        self.slurm = {opt: settings.get(f"slurm.{stage}.{opt}", default) for opt, default in SLURM_DEFAULTS[stage].items()}

        cfg = {k: v for k, v in settings.items() if used_by(k, stage)}
        events = int(settings["events"])
        cfg["events"] = str(events // nchunks + (chunk < events % nchunks))
        cfg["seed"] = str(int(settings["seed"]) + chunk)
        cfg["output"] = self.output
        if stage != "sim":
            prev = STAGES[STAGES.index(stage) - 1]
            cfg["input"] = os.path.join(DATA, f"{study}-{input_run}.{prev}{suffix}.root")
        self.cfg = cfg

    @property
    def key(self):
        return (self.run, self.stage, self.chunk)

    def __str__(self):
        return os.path.basename(self.base)

    def stale_reason(self):
        sidecar = self.base + ".cfg"
        if not (os.path.exists(self.output) and os.path.exists(sidecar)):
            return "missing"
        done = read_cfg(sidecar)
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
        sidecar = self.base + ".cfg"
        if os.path.exists(sidecar):
            os.remove(sidecar)
        start = time.time()
        with open(self.base + ".log", "w") as log:
            subprocess.run(self.command(), cwd=HERE, stdout=log, stderr=subprocess.STDOUT)
        ok = os.path.exists(sidecar)
        print(f"{'done' if ok else 'FAILED':6} {self} ({time.time() - start:.0f} s)"
              + ("" if ok else f", see {self.base}.log"), flush=True)
        return ok


def plan(study, runs, last_stage, force):
    """The jobs needed to bring every run up to last_stage, in dependency order."""
    return [j for j in plan_all(study, runs, last_stage, force).values() if j.reason]


def plan_all(study, runs, last_stage, force):
    """Every job of the runs up to last_stage, keyed (run, stage, chunk) so shared upstream appears once.
    job.reason says why a job must run, or is None if it is up to date."""
    jobs = {}
    for run in runs:
        settings, owner = study.settings(run)
        for chunk in range(int(settings.get("chunks", 1))):
            prev = None
            for stage in STAGES[: STAGES.index(last_stage) + 1]:
                job_run = run if stage == "fit" else owner
                key = (job_run, stage, chunk)
                if key in jobs:  # sim/digi shared with an upstream run, already planned
                    prev = key
                    continue
                job = jobs[key] = Job(study.name, job_run, stage, chunk, settings, owner)
                reason = job.stale_reason()
                if force and STAGES.index(stage) >= STAGES.index(force):
                    reason = "forced"
                if prev is not None and jobs[prev].reason:
                    reason = reason or "upstream rerun"
                    job.deps.append(jobs[prev])
                job.reason = reason
                prev = job.key
    return jobs


def status(study, runs):
    """Print a table of each run's stages: done, or how many chunks are missing, stale or in SLURM."""
    jobs = plan_all(study, runs, STAGES[-1], None)
    queued = slurm_jobs()
    rows = []
    for run in runs:
        settings, owner = study.settings(run)
        cells = []
        for stage in STAGES:
            job_run = run if stage == "fit" else owner
            stage_jobs = [jobs[(job_run, stage, c)] for c in range(int(settings.get("chunks", 1)))]
            counts = {}
            for job in stage_jobs:
                state = "queued" if str(job) in queued else (job.reason or "done").split(":")[0]
                counts[state] = counts.get(state, 0) + 1
            if counts == {"done": len(stage_jobs)}:
                cells.append("done")
            elif len(stage_jobs) == 1:
                cells.append(next(iter(counts)))
            else:
                cells.append(", ".join(f"{n} {state}" for state, n in sorted(counts.items())))
        rows.append([run + (f" (from {owner})" if owner != run else "")] + cells)
    header = ["run"] + STAGES
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(f"{cell:{w}}" for cell, w in zip(row, widths)).rstrip())
    done = sum(all(c == "done" for c in row[1:]) for row in rows)
    print(f"\n{done} of {len(rows)} runs finished." + (" --dry-run shows why the rest would run." if done < len(rows) else ""))


def show(study, runs, last_stage):
    """Print each run's resolved settings, with where each comes from, and what each stage is passed."""
    for run in runs:
        settings, owner = study.settings(run)
        print(f"[run {run}]" + (f"  (sim and digi from {owner})" if owner != run else ""))
        width = max(len(f"{k} = {v}") for k, v in settings.items())
        for key in sorted(settings):
            print(f"  {f'{key} = {settings[key]}':{width}}  ; {study.origin(run, key)}")
        nchunks = int(settings.get("chunks", 1))
        for stage in STAGES[: STAGES.index(last_stage) + 1]:
            job = Job(study.name, run if stage == "fit" else owner, stage, 0, settings, owner)
            print(f"  {os.path.basename(job.base)}.cfg.in" + (f" (chunk 0 of {nchunks})" if nchunks > 1 else ""))
            for key, val in job.cfg.items():
                print(f"    {key} = {val}")
            print(f"    (--slurm: --time={job.slurm['time']} --mem={job.slurm['mem']})")
        print()
    print("Settings not passed to a stage use the macro's built-in default.")


def affected_runs(study, todo):
    """Runs whose files the jobs in todo replace: the runs owning the sim/digi output being rerun, and
    every run that refits it."""
    owners = {study.settings(job.run)[1] for job in todo}
    return sorted(run for run in study.sections if study.settings(run)[1] in owners)


def old_chunking(study, runs):
    """Outputs (with their .cfg, .cfg.in and .log) of runs left over from a different chunk count.

    Every stage is checked, not only the planned ones: after `--stage sim` with a new chunk count, the
    old digi and fit chunks would otherwise stay behind and be picked up by the plots."""
    old = []
    for run in runs:
        nchunks = int(study.settings(run)[0].get("chunks", 1))
        for stage in STAGES:
            prefix = os.path.join(DATA, f"{study.name}-{run}.{stage}")
            current = {prefix + chunk_suffix(c, nchunks) + ".root" for c in range(nchunks)}
            for path in sorted(glob.glob(prefix + ".root") + glob.glob(prefix + ".c[0-9][0-9].root")):
                if path not in current:
                    old += [path[:-5] + ext for ext in (".root", ".cfg", ".cfg.in", ".log")
                            if os.path.exists(path[:-5] + ext)]
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
    base, _, last = name.rpartition(".")
    if re.fullmatch(r"c\d\d", last):
        base, _, last = base.rpartition(".")
    return base if base and last in STAGES else None


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
        cmd = ["sbatch", "--parsable", f"--job-name={job}", f"--output={job.base}.log",
               f"--cpus-per-task={threads}", f"--time={job.slurm['time']}", f"--mem={job.slurm['mem']}",
               f"--chdir={HERE}"]
        if job.deps:
            cmd.append("--dependency=afterok:" + ":".join(ids[d.key] for d in job.deps))
            cmd.append("--kill-on-invalid-dep=yes")
        if os.path.exists(job.base + ".cfg"):
            os.remove(job.base + ".cfg")
        sidecar = shlex.quote(job.base + ".cfg")
        cmd.append(f"--wrap=rm -f {sidecar}; {shlex.join(job.command())}; test -f {sidecar}")
        ids[job.key] = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.strip().split(";")[0]
        print(f"submitted {job} as {ids[job.key]}")


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

    study = Study(args.study)
    if args.show:
        show(study, args.runs or list(study.sections), args.stage)
        return
    if args.status:
        status(study, args.runs or list(study.sections))
        return
    todo = plan(study, args.runs or list(study.sections), args.stage, args.force)
    if not todo:
        print("Everything is up to date.")
        return
    # Jobs still in SLURM own their files: don't run them again here or delete their outputs. This
    # includes jobs from any chunk count, whose files may not exist yet.
    queued = slurm_jobs()
    affected = affected_runs(study, todo)
    old = old_chunking(study, affected)  # Removed so plots don't pick them up
    planned = {f"{study.name}-{run}" for run in affected}
    busy = sorted(name for name in queued if job_run_name(name) in planned)
    for job in todo:
        print(f"{str(job):40} {job.reason}" + (" (queued in SLURM)" if str(job) in queued else ""))
    for path in old:
        print(f"{'would remove' if args.dry_run or busy else 'remove'} {os.path.basename(path)} (old chunk count)")
    if busy:
        message = "Still queued or running in SLURM (wait, or scancel them): " + ", ".join(busy)
        if not args.dry_run:
            sys.exit(message)
        print(message)
    if args.dry_run:
        return

    os.makedirs(DATA, exist_ok=True)
    for path in old:
        os.remove(path)
    if args.slurm:
        run_slurm(todo)
    elif not run_local(todo, args.j):
        sys.exit(1)


if __name__ == "__main__":
    main()
