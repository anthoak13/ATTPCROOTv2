#!/usr/bin/env python3
"""Run the e12014 fission fit-validation chain (simulate -> digitize -> fit) for a study.

A study file in studies/ lists the outputs to produce and the settings that
define them. This script runs whatever is missing or out of date while not wasting compute on
up-to-date outputs.

A study file looks like this:

    [defaults]              ; <stage>.<key>, applies to every section of that stage
    sim.events = 500
    fit.iter = 100

    [sim dev2]              ; a simulation. Keys here are unprefixed: massDev is sim.massDev
    massDev = 2

    [digi dev2]             ; digitizes [sim dev2], the sim with the same name

    [fit dev2]              ; fits [digi dev2]

    [fit dev2-srim]         ; fits [digi dev2] again, using SRIM tables
    input = dev2
    eloss = SRIM

input names what a digi or fit reads, always an output of the stage before it, and: MANUAL.md covers 
the input syntax.

Output goes to $FISSION_DATA (default: data/ next to this script) as <study>-<name>.<stage>.root,
with the ROOT output in a matching .log. A section reruns when its output is missing, when a setting
it depends on (its own or one of its input's) changed, or when its input reran.

    ./fission.py studies/smoke.ini --dry-run          # list what would run, and why
    ./fission.py studies/smoke.ini                    # run it
    ./fission.py studies/smoke.ini -j 4               # run it, 4 jobs at a time
    ./fission.py studies/smoke.ini --slurm            # submit it to SLURM instead
    ./fission.py studies/smoke.ini --status           # which sections are done
    ./fission.py studies/smoke.ini --show base.fit    # a section's settings, and where each comes from
    ./fission.py studies/smoke.ini base               # only the sections named base (and their inputs)
    ./fission.py studies/smoke.ini --stage sim        # only the sims
    ./fission.py studies/smoke.ini --force fit        # redo the fits even if up to date

Needs the ATTPCROOT environment (source build/config.sh) and TPC_SHARED_INFO set to the directory
of shared inputs (energy-loss tables, pad response, low-gain pad list). See MANUAL.md.
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
from collections import Counter
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait

HERE = os.path.dirname(os.path.abspath(__file__))
STAGES = ["sim", "digi", "fit"]
MACROS = {"sim": "run_simp_fiss.C", "digi": "run_digi_fiss.C", "fit": "run_fit.C"}
FIT_THREADS_DEFAULT = "4"  # run_fit.C's default for fit.threads, so SLURM requests what the fit uses
# SLURM time limit and memory per stage, overridable with <stage>.slurm.time / <stage>.slurm.mem. Rough
# upper bounds for a 500-event chunk, not measurements: lower them once bench.ini has timings.
SLURM_DEFAULTS = {
    "sim": {"time": "0:10:00", "mem": "4G"},
    "digi": {"time": "8:00:00", "mem": "8G"},
    "fit": {"time": "3-00:00:00", "mem": "8G"},
}
DATA = os.path.abspath(os.environ.get("FISSION_DATA", os.path.join(HERE, "data")))
NAME = r"[A-Za-z0-9_-]+"
STUDY_NAME = r"[A-Za-z0-9_]+"  # No - in study names, so <study>-<name> splits only one way
SECTION = re.compile(rf"({'|'.join(STAGES)})\s+(.*)")
# A stage's files are <study>-<name>.<stage>[.cNN] plus an extension from OUTPUT_EXTS. The name without
# extension is also the stage's SLURM job name.
JOB_NAME = re.compile(rf"(?P<study>{STUDY_NAME})-(?P<name>{NAME})\.(?P<stage>{'|'.join(STAGES)})(?:\.c\d\d)?")
OUTPUT_EXTS = (".root", ".cfg", ".cfg.in", ".log")


def driver_key(key):
    """True for chunks, input and slurm.*: settings the driver uses itself and never passes to a macro."""
    return key in ("chunks", "input") or key.startswith("slurm.")


def is_input_file(spec):
    """True if an input setting names files (a path or glob) rather than a section."""
    return "/" in spec or spec.endswith(".root")


def derived_seed(name):
    """A section's default seed, fixed by its <study>-<name>.<stage> name so a rerun reproduces it."""
    return str(zlib.crc32(name.encode()) & 0x7FFFFFFF or 1)


def short_path(path):
    """path as it would be typed in the current directory (relative if below it), for printed commands."""
    rel = os.path.relpath(path)
    return path if rel.startswith("..") else rel if os.sep in rel else os.path.join(".", rel)


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


_studies = {}


def load_study(path):
    """Parse a study file, once per path. Code compares Study and Section objects and uses them as keys,
    so every reference to one file must get the same object."""
    path = os.path.abspath(path)
    if path not in _studies:
        _studies[path] = Study(path)
    return _studies[path]


class Study:
    """A study file: its [defaults] and [sim|digi|fit <name>] sections, checked for unknown sections,
    misplaced settings and bad names."""

    def __init__(self, path):
        self.path = path
        self.name = os.path.splitext(os.path.basename(path))[0]
        if not os.path.isfile(path):
            sys.exit(f"{short_path(path)}: no such study file")
        # Names end up in file names (<study>-<name>.<stage>), SLURM job names and ROOT command lines
        if not re.fullmatch(STUDY_NAME, self.name):
            sys.exit(f"{short_path(path)}: study name {self.name!r} may only use letters, digits and _")
        # No interpolation, so % is plain text. Any [DEFAULT] is an ordinary (unknown) section, rather
        # than one whose keys configparser copies into every other section.
        parser = configparser.ConfigParser(inline_comment_prefixes=(";", "#"), interpolation=None,
                                           default_section="\0")
        parser.optionxform = str  # Keep key case (decayAngle)
        try:
            parser.read(path)
        except configparser.Error as error:
            sys.exit(str(error))

        defaults = {}
        self.raw = {}  # (stage, name) -> the section's keys as written, in file order
        self._sections = {}
        for header in parser.sections():
            values = dict(parser[header])
            match = SECTION.fullmatch(header)
            if header == "study":
                continue
            elif header == "defaults":
                defaults = values
            elif not match:
                sys.exit(f"{self.where(header)}: unknown section (use [study], [defaults] or"
                         f" [{'|'.join(STAGES)} <name>])")
            elif not re.fullmatch(NAME, match[2]):
                sys.exit(f"{self.where(header)}: name {match[2]!r} may only use letters, digits, _ and -")
            elif match.groups() in self.raw:
                sys.exit(f"{self.where(header)}: there is already a [{match[1]} {match[2]}]")
            else:
                self.raw[match.groups()] = values
        if not self.raw:
            sys.exit(f"{short_path(path)}: no [{'|'.join(STAGES)} <name>] sections")

        self.defaults = {stage: {} for stage in STAGES}  # stage -> [defaults] of that stage, without prefix
        for key, val in defaults.items():
            stage, _, rest = key.partition(".")
            if stage not in STAGES or not rest:
                sys.exit(f"{self.where('defaults')}: {key} needs a stage prefix ({', '.join(STAGES)}),"
                         f" e.g. sim.{key}")
            self.check_key(stage, rest, "defaults", key)
            self.defaults[stage][rest] = val
        for (stage, name), values in self.raw.items():
            for key in values:
                self.check_key(stage, key, f"{stage} {name}", key)

    def where(self, header):
        """A section, named for messages: <study file>: [header]."""
        return f"{short_path(self.path)}: [{header}]"

    def check_key(self, stage, key, header, written):
        """Reject a setting of stage that the driver knows is wrong. key is without the stage prefix, as
        written is as it appears in the file. The macros reject unknown settings themselves."""
        first = key.split(".", 1)[0]
        problem = (f"keys in a [{stage} <name>] section have no stage prefix" if first in STAGES and header != "defaults"
                   else "only a digi or fit has an input" if key == "input" and stage == "sim"
                   else f"{key} is a sim setting (sim.{key})" if key in ("events", "chunks") and stage != "sim"
                   else f"unknown slurm setting (use {stage}.slurm.time or .mem)"
                   if first == "slurm" and key not in ("slurm.time", "slurm.mem")
                   else None)
        if problem:
            sys.exit(f"{self.where(header)}: {written}: {problem}")

    def section(self, stage, name):
        """The resolved Section for [stage name], once per section."""
        if (stage, name) not in self._sections:
            self._sections[(stage, name)] = Section(self, stage, name)
        return self._sections[(stage, name)]

    def sections(self):
        """Every section, in file order."""
        return [self.section(*key) for key in self.raw]

    def resolve_input(self, section, spec, origin):
        """What section reads, given its input setting: a Section of the stage before it (here or in
        another study), or InputFiles for a path or glob."""
        prev = STAGES[STAGES.index(section.stage) - 1]
        where = f"{self.where(f'{section.stage} {section.name}')}: input = {spec}"
        if is_input_file(spec):
            return InputFiles(spec, prev, where)
        study, name = self, spec
        if re.fullmatch(f"{STUDY_NAME}:{NAME}", spec):
            other, name = spec.split(":")
            path = os.path.join(os.path.dirname(self.path), other + ".ini")
            if not os.path.isfile(path):
                sys.exit(f"{where}, but there is no {short_path(path)}")
            study = load_study(path)
        elif not re.fullmatch(NAME, spec):
            sys.exit(f"{where} is not a <name>, <study>:<name>, or a path to files")
        if (prev, name) not in study.raw:
            if origin == "same name":
                sys.exit(f"{self.where(f'{section.stage} {section.name}')} reads [{prev} {name}] by default,"
                         f" but there is none (add it, or set input)")
            sys.exit(f"{where}, but {short_path(study.path)} has no [{prev} {name}]")
        return study.section(prev, name)


class Section:
    """One [<stage> <name>] section: one output of one stage, possibly split into chunks.

    settings  the section's own settings, without stage prefix: [defaults] of its stage, then the
              section, then derived defaults (seed, and events and chunks for a sim)
    origins   where each setting comes from, for --show
    input     what it reads: a Section of the previous stage, InputFiles, or None for a sim
    nchunks   number of chunks: sim.chunks for a sim, else the input's
    """

    def __init__(self, study, stage, name):
        self.study, self.stage, self.name = study, stage, name
        self.settings = dict(study.defaults[stage])
        self.origins = dict.fromkeys(self.settings, "defaults")
        own = study.raw[(stage, name)]
        self.settings.update(own)
        self.origins.update(dict.fromkeys(own, "section"))
        derived = {"seed": (derived_seed(f"{study.name}-{name}.{stage}"), "from study, name and stage")}
        if stage == "sim":
            derived.update({"events": ("500", "driver default"), "chunks": ("1", "driver default")})
        for key, (val, origin) in derived.items():
            if key not in self.settings:
                self.settings[key], self.origins[key] = val, origin

        self.input = None
        if stage == "sim":
            self.nchunks = int(self.settings["chunks"])
        else:
            spec, origin = self.settings.get("input", name), self.origins.get("input", "same name")
            self.input_origin = origin
            self.input = study.resolve_input(self, spec, origin)
            self.nchunks = self.input.nchunks
        self.slurm = {opt: self.settings.get(f"slurm.{opt}", default)
                      for opt, default in SLURM_DEFAULTS[stage].items()}

    def ref(self, study):
        """How study names this section in messages: '<stage> <name>', with <study>: if in another study."""
        return f"{self.stage} " + (self.name if self.study is study else f"{self.study.name}:{self.name}")

    def stem(self, chunk=None):
        """Path of the section's files without extension: <data>/<study>-<name>.<stage>, plus .cNN for a
        chunk if the section is chunked."""
        base = os.path.join(DATA, f"{self.study.name}-{self.name}.{self.stage}")
        return base + (f".c{chunk:02d}" if chunk is not None and self.nchunks > 1 else "")

    def chain(self, study):
        """What the section reads, and what that reads in turn, e.g. 'digi a <- sim a'."""
        parts, source = [], self.input
        while source:
            parts.append(source.ref(study))
            source = source.input
        return " <- ".join(parts)

    def cfg(self, chunk):
        """The section's own settings as one chunk's macro receives them: with stage prefix, without the
        driver's settings, with seed + chunk, and for a sim with the chunk's share of sim.events."""
        cfg = {k: v for k, v in self.settings.items() if not driver_key(k)}
        cfg["seed"] = str(int(cfg["seed"]) + chunk)
        if self.stage == "sim":
            events = int(cfg["events"])
            cfg["events"] = str(events // self.nchunks + (chunk < events % self.nchunks))
        return {f"{self.stage}.{k}": v for k, v in cfg.items()}


class InputFile:
    """One file of InputFiles, which a section reads like the output of a Job that is up to date."""
    reason = None

    def __init__(self, output, cfg):
        self.output, self.cfg = output, cfg


class InputFiles:
    """Files made elsewhere that a section reads (input = <path or glob>, relative to FISSION_DATA).
    Each file is one chunk, and its settings come from the .cfg next to it. The driver never reruns or
    removes these files. Like a Section, it has input (None, since what made the files isn't known)
    and ref()."""
    input = None

    def __init__(self, pattern, stage, where):
        self.pattern = pattern
        paths = sorted(glob.glob(os.path.join(DATA, pattern)))  # An absolute pattern ignores DATA
        if not paths:
            sys.exit(f"{where}: no files match in {DATA}")
        self.chunks = []
        for path in paths:
            if not (path.endswith(".root") and os.path.exists(sidecar(path))):
                sys.exit(f"{where}: {path} has no .cfg next to it (unfinished, or not a stage output)")
            cfg = {k: v for k, v in read_cfg(sidecar(path)).items() if k not in ("input", "output")}
            for key in cfg:
                prefix = key.split(".", 1)[0]
                if prefix not in STAGES:
                    sys.exit(f"{where}: {os.path.basename(path)} was made before study files had stage"
                             f" sections ({key} has no stage prefix). Rerun it.")
                if STAGES.index(prefix) > STAGES.index(stage):
                    sys.exit(f"{where}: {os.path.basename(path)} has {key}, so it is not a {stage} output")
            self.chunks.append(InputFile(path, cfg))
        # The files must agree on everything but events and seeds
        common = [{k: v for k, v in c.cfg.items() if not k.endswith((".events", ".seed"))} for c in self.chunks]
        for path, values in zip(paths[1:], common[1:]):
            if values != common[0]:
                sys.exit(f"{where}: {os.path.basename(path)} was made with different settings"
                         f" than {os.path.basename(paths[0])}")
        self.nchunks = len(self.chunks)

    def ref(self, study):
        return f"{self.pattern} ({self.nchunks} file{'s' if self.nchunks > 1 else ''})"


class Job:
    """One chunk of one section: a single ROOT macro call.

    cfg holds what the macro is passed: the settings of prev (the Job or InputFile it reads, or None for
    a sim), then the section's own, then input and output. So a fit is passed every sim.*, digi.* and
    fit.* setting of its chain, and a change anywhere in the chain changes its cfg.

    plan_all() sets reason (why the job must run, or None), deps (jobs that must finish first) and
    external."""

    def __init__(self, section, chunk, prev):
        self.section, self.chunk = section, chunk
        self.name, self.stage = section.name, section.stage
        self.base = section.stem(chunk)
        self.output = self.base + ".root"
        self.sidecar = sidecar(self.output)
        self.cfg_in = self.base + ".cfg.in"
        self.log = self.base + ".log"
        self.deps = []
        self.external = None  # The other Study this job belongs to, if any. Such jobs never run from here.
        self.slurm = section.slurm

        cfg = {k: v for k, v in prev.cfg.items() if k not in ("input", "output")} if prev else {}
        cfg.update(section.cfg(chunk))
        if prev:
            cfg["input"] = prev.output
        cfg["output"] = self.output
        self.cfg = cfg

    def __str__(self):
        return os.path.basename(self.base)

    def stale_reason(self):
        """Why the job must run (output missing, settings changed, input newer), or None if up to date."""
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
        """Write what the macro is passed to <base>.cfg.in."""
        with open(self.cfg_in, "w") as f:
            f.write("".join(f"{k} = {v}\n" for k, v in self.cfg.items()))

    def script(self):
        """The shell command that runs the macro, here or in SLURM. It succeeds only if the macro reached
        the end and wrote a new .cfg (RunConfig::Finish), whatever ROOT's exit code."""
        sidecar = shlex.quote(self.sidecar)
        root = ["root", "-l", "-b", "-q", f'{MACROS[self.stage]}("{self.cfg_in}")']
        return f"rm -f {sidecar}; {shlex.join(root)}; test -f {sidecar}"

    def run_local(self):
        """Run the macro here, and return whether it succeeded."""
        self.write_cfg()
        start = time.time()
        with open(self.log, "w") as log:
            result = subprocess.run(self.script(), shell=True, cwd=HERE, stdout=log, stderr=subprocess.STDOUT)
        ok = result.returncode == 0
        print(f"{'done' if ok else 'FAILED':6} {self} ({time.time() - start:.0f} s)"
              + ("" if ok else f", see {self.log}"), flush=True)
        return ok


def select(study, selectors, stages):
    """The sections to process, in file order: those matching a selector (NAME for every section with
    that name, NAME.STAGE for one), or all, then only those of the given stages."""
    keys = list(study.raw)  # (stage, name), so sections are resolved only if selected
    if selectors:
        chosen = set()
        for selector in selectors:
            sel_name, _, sel_stage = selector.partition(".")
            matches = [(stage, name) for stage, name in keys if name == sel_name and sel_stage in ("", stage)]
            if not matches:
                first_stage, first_name = keys[0]
                sys.exit(f"{short_path(study.path)}: no section matches {selector} (give NAME or NAME.STAGE,"
                         f" e.g. {first_name}.{first_stage})")
            chosen.update(matches)
        keys = [k for k in keys if k in chosen]
    if stages:
        keys = [(stage, name) for stage, name in keys if stage in stages]
        if not keys:
            sys.exit(f"No {' or '.join(stages)} sections selected")
    return [study.section(*k) for k in keys]


def plan(study, sections, force):
    """The jobs that bring the sections up to date, as (this study's jobs, other studies' jobs). This
    study's are in dependency order. Other studies' must be run from their own study first."""
    jobs = [j for js in plan_all(study, sections, force).values() for j in js if j.reason]
    return [j for j in jobs if not j.external], [j for j in jobs if j.external]


def plan_all(study, sections, force):
    """Every job of the sections and their inputs, up to date or not, as {section: [job per chunk]}
    in dependency order. Inputs shared by several sections appear once. job.reason says why a job must
    run, or is None. Sections of other studies are included with job.external set. Input files have
    no jobs."""
    planned = {}

    def add(section):
        if section in planned:
            return planned[section]
        if isinstance(section.input, Section):
            inputs = add(section.input)
        else:
            inputs = section.input.chunks if section.input else [None] * section.nchunks
        jobs = []
        for chunk, prev in enumerate(inputs):
            job = Job(section, chunk, prev)
            if section.study is not study:
                job.external = section.study
            reason = job.stale_reason()
            if force and not job.external and STAGES.index(section.stage) >= STAGES.index(force):
                reason = "forced"
            if prev and prev.reason:
                reason = reason or "upstream rerun"
                job.deps.append(prev)
            job.reason = reason
            jobs.append(job)
        planned[section] = jobs
        return jobs

    for section in sections:
        add(section)
    return planned


def status(study, sections):
    """Print a table of the sections: done, or how many chunks are missing, stale or in SLURM."""
    queued = slurm_jobs()
    planned = plan_all(study, sections, None)
    rows = []
    for section in sections:
        jobs = planned[section]
        counts = Counter("queued" if str(job) in queued else (job.reason or "done").split(":")[0] for job in jobs)
        if counts == {"done": len(jobs)}:
            state = "done"
        elif len(jobs) == 1:
            state = next(iter(counts))
        else:
            state = ", ".join(f"{n} {s}" for s, n in sorted(counts.items()))
        rows.append([section.ref(study), "<- " + section.input.ref(study) if section.input else "", state])
    header = ["section", "input", "state"]
    widths = [max(len(r[i]) for r in rows + [header]) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(f"{cell:{w}}" for cell, w in zip(row, widths)).rstrip())
    done = sum(row[2] == "done" for row in rows)
    print(f"\n{done} of {len(rows)} sections finished."
          + (" --dry-run shows why the rest would run." if done < len(rows) else ""))


def show(study, sections):
    """Print each section's settings with where each comes from, what it reads, and what its macro is
    passed."""
    planned = plan_all(study, sections, None)
    for section in sections:
        print(f"[{section.stage} {section.name}]")
        if section.input is not None:
            origin = section.input_origin
            print(f"  reads {section.chain(study)}  ; " + (origin if origin == "same name" else f"input in {origin}"))
        own = {k: v for k, v in section.settings.items() if k != "input"}
        width = max(len(f"{section.stage}.{k} = {v}") for k, v in own.items())
        for key in sorted(own):
            print(f"  {f'{section.stage}.{key} = {own[key]}':{width}}  ; {section.origins[key]}")
        job = planned[section][0]
        n = section.nchunks
        print(f"  {job}.cfg.in" + (f" (chunk 0 of {n}; chunk k uses seed + k"
                                    + (" and its share of sim.events)" if section.stage == "sim" else ")")
                                    if n > 1 else ""))
        for key, val in job.cfg.items():
            print(f"    {key} = {val}")
        print(f"  (--slurm: --time={section.slurm['time']} --mem={section.slurm['mem']})")
        print()
    print("Settings not passed to a stage use the macro's built-in default.")


def affected_sections(study, todo):
    """This study's sections whose files todo may rewrite or make obsolete: the sections it writes, and
    every section of the study that reads one of them, directly or through its input."""
    written = {job.section for job in todo}  # todo holds only this study's jobs

    def reads_written(section):
        while section:
            if section in written:
                return True
            section = section.input
        return False

    return [s for s in study.sections() if reads_written(s)]


def old_chunking(sections):
    """Outputs (with their .cfg, .cfg.in and .log) of sections left over from a different chunk count."""
    old = []
    for section in sections:
        current = {section.stem(c) for c in range(section.nchunks)}
        for path in sorted(glob.glob(section.stem() + "*.root")):
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


def job_section(name):
    """(study, name, stage) of a job name <study>-<name>.<stage>[.cNN], or None if it isn't one."""
    match = JOB_NAME.fullmatch(name)
    return match and (match["study"], match["name"], match["stage"])


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
    """Submit each job with sbatch, to start once the jobs it depends on succeed.

    Each job runs Job.script(). The old .cfg is also removed at submission, so a queued job never looks
    up to date. SLURM cancels a job whose dependency failed rather than leaving it pending.
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
            cmd.append("--dependency=afterok:" + ":".join(ids[d] for d in job.deps))
            cmd.append("--kill-on-invalid-dep=yes")
        if os.path.exists(job.sidecar):
            os.remove(job.sidecar)
        job.write_cfg()
        cmd.append(f"--wrap={job.script()}")
        ids[job] = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.strip().split(";")[0]
        print(f"submitted {job} as {ids[job]}")


def blockers(study, external, affected, queued):
    """Why the jobs can't run yet, as messages: this study's affected sections with jobs still in SLURM,
    and other studies' out-of-date sections."""
    messages = []
    # Jobs still in SLURM own their files: don't run them again here or delete their outputs. This
    # includes jobs from any chunk count, whose files may not exist yet.
    keys = {(s.study.name, s.name, s.stage) for s in affected}
    busy = sorted(name for name in queued if job_section(name) in keys)
    if busy:
        messages.append("Still queued or running in SLURM (wait, or scancel them): " + ", ".join(busy))
    if external:
        # Other studies' files are theirs to run: running them from here would change the inputs of
        # their own sections behind their back. external is in dependency order, so is the list.
        others = {}
        for job in external:
            others.setdefault(job.external.path, {})[f"{job.name}.{job.stage}"] = None  # Ordered set
        messages.append("Bring other studies' sections up to date first:\n" + "\n".join(
            f"  {shlex.quote(short_path(os.path.join(HERE, 'fission.py')))} {shlex.quote(short_path(path))} "
            + " ".join(selectors) for path, selectors in others.items()))
    return messages


def print_plan(jobs, old, queued, remove):
    """Print why each job runs, and the files of an old chunk count that are (or would be) removed."""
    for job in jobs:
        print(f"{str(job):40} {job.reason}" + (" (queued in SLURM)" if str(job) in queued else "")
              + (f" (owned by {job.external.name})" if job.external else ""))
    for path in old:
        print(f"{'remove' if remove else 'would remove'} {os.path.basename(path)} (old chunk count)")


def update(study, sections, args):
    """Run, or with --dry-run only list, the jobs that bring the sections up to date."""
    todo, external = plan(study, sections, args.force)
    if not todo and not external:
        print("Everything is up to date.")
        return
    queued = slurm_jobs()
    affected = affected_sections(study, todo)
    old = old_chunking(affected)  # Removed so plots don't pick them up
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
    parser.add_argument("selectors", nargs="*", metavar="SECTION",
                        help="NAME (every section with that name) or NAME.STAGE (one section). Default: all")
    parser.add_argument("--stage", choices=STAGES, action="append",
                        help="only sections of this stage (repeatable). Their inputs in this study still"
                             " come up to date first")
    parser.add_argument("--force", choices=STAGES, help="rerun this stage and the later ones even if up to date")
    parser.add_argument("--dry-run", action="store_true", help="only show what would run")
    parser.add_argument("--show", action="store_true",
                        help="print each section's settings, their sources, its input, and what its macro is"
                             " passed, then exit")
    parser.add_argument("--status", action="store_true",
                        help="print which sections are done, missing, stale or in SLURM, then exit")
    parser.add_argument("-j", type=int, default=1, help="jobs to run at once (default: 1)")
    parser.add_argument("--slurm", action="store_true", help="submit jobs with sbatch")
    args = parser.parse_intermixed_args()

    study = load_study(args.study)
    sections = select(study, args.selectors, args.stage)
    if args.show:
        show(study, sections)
    elif args.status:
        status(study, sections)
    else:
        update(study, sections, args)


if __name__ == "__main__":
    main()
