#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

"""
Writes the proof suite of a deployment of ecflow-chart: the definition and the scripts
of a suite shaped like a small research setup, two experiments over two dates, each
with a build, daily cycles with limits, meters, events and labels, a lagged archive and
a final cancellation, plus one task that aborts on its first try and completes on the
second. Nothing in it is realistic, but every job runs for real, inside the server
container, and reports to the server over loopback (the chart runs the jobs where the
server runs; see D2 of the task report).

    make_suite.py --setup    [--out DIR] [--name NAME]    # write the suite (the default)
    make_suite.py --teardown [--out DIR] [--name NAME]    # remove what --setup wrote

The output directory holds NAME.def, include/ and files/, laid out as the server
expects them under /workspace: the include files under include/, the scripts under
files/<suite>/... Upload include/ and files/ to the workspace over SFTP, then load the
definition from the host through the reverse proxy. The job files and their output are
written next to the scripts' paths under ECF_HOME, which the server creates itself.
"""

import argparse
import pathlib
import shutil

WORKSPACE = "/workspace"
FIRST_DATE, LAST_DATE = 20260101, 20260102


class Node:
    def __init__(self, kind, name, *attributes, script=None):
        self.kind, self.name, self.attributes, self.script = kind, name, list(attributes), script
        self.children = []

    def add(self, *children):
        self.children.extend(children)
        return self

    def tasks(self, prefix=""):
        path = f"{prefix}/{self.name}"
        if self.kind == "task":
            yield path, self
        for child in self.children:
            yield from child.tasks(path)

    def render(self, indent=0):
        pad = "  " * indent
        lines = [f"{pad}{self.kind} {self.name}"]
        lines += [f"{pad}  {attribute}" for attribute in self.attributes]
        lines += [child.render(indent + 1) for child in self.children]
        if self.kind != "task":
            lines.append(f"{pad}end{self.kind}")
        return "\n".join(lines)


def suite(name, *attributes):
    return Node("suite", name, *attributes)


def family(name, *attributes):
    return Node("family", name, *attributes)


def task(name, *attributes, script="default"):
    return Node("task", name, *attributes, script=script)


def make_family():
    return family("make").add(
        task("setup"),
        task("get_source", "trigger setup == complete"),
        task("build", "trigger get_source == complete", "event installed", 'label version ""', script="build"),
    )


def cancel_family(main):
    return family("cancel", f"trigger {main} == complete").add(
        family("archive").add(task("archive_scripts"), task("archive_source")),
        task("cancel", "trigger archive == complete"),
    )


def forecast_experiment(name, exp, length):
    """A forecast experiment: make, a daily forecast (main) and its lagged archive."""
    fc = family("fc", "limit hpc 2", "limit archive 1", f"edit FCLENGTH '{length}'").add(
        make_family(),
        family("main", "trigger make == complete", f"repeat date YMD {FIRST_DATE} {LAST_DATE}").add(
            task("getini", "edit ECF_TRIES '2'", script="flaky"),
            task("model", "trigger getini == complete", f"inlimit /{name}/{exp}/fc:hpc",
                 f"meter step -1 {length} {length}", 'label pace ""', script="model"),
            task("save", "trigger model == complete"),
        ),
        family("lag", "trigger lag:YMD lt main:YMD or main == complete",
               f"repeat date YMD {FIRST_DATE} {LAST_DATE}").add(
            family("archive").add(*[
                task(stream, f"inlimit /{name}/{exp}/fc:archive", "event fdb", script="archive")
                for stream in ["pl", "sfc", "wave"]
            ]),
            task("clean", "trigger archive == complete"),
        ),
    )
    return fc, "fc"


def assimilation_experiment(name, exp, length):
    """An assimilation experiment: make, 4D-Var cycles at 00 and 12 UTC (main) and their lagged archive."""
    def cycle(hh, *attributes):
        return family(hh, *attributes, f"edit HH '{hh}'").add(
            family("obs").add(*[task(f"get_{kind}", 'label nobs ""', script="get_obs") for kind in ["conv", "sat"]]),
            family("4dvar", "trigger obs == complete").add(
                task("traj_0", f"inlimit /{name}/{exp}/an:hpc"),
                task("min_0", "trigger traj_0 == complete", f"inlimit /{name}/{exp}/an:hpc",
                     "meter iter 0 20 20", 'label costf ""', script="min"),
                task("traj_1", "trigger min_0 == complete", f"inlimit /{name}/{exp}/an:hpc"),
            ),
            task("model", "trigger 4dvar == complete", f"inlimit /{name}/{exp}/an:hpc",
                 f"meter step -1 {length} {length}", 'label pace ""', script="model"),
        )

    an = family("an", "limit hpc 2", "limit archive 1", f"edit FCLENGTH '{length}'").add(
        make_family(),
        family("main", "trigger make == complete", f"repeat date YMD {FIRST_DATE} {LAST_DATE}").add(
            cycle("00"),
            cycle("12", "trigger 00 == complete"),
        ),
        family("lag", "trigger lag:YMD lt main:YMD or main == complete",
               f"repeat date YMD {FIRST_DATE} {LAST_DATE}").add(
            family("archive").add(*[
                task(stream, f"inlimit /{name}/{exp}/an:archive", "event fdb", script="archive")
                for stream in ["an", "fc"]
            ]),
            task("clean", "trigger archive == complete"),
        ),
    )
    return an, "an"


def make_suite(name):
    root = suite(
        name,
        f"edit ECF_HOME '{WORKSPACE}'",
        f"edit ECF_FILES '{WORKSPACE}/files'",
        f"edit ECF_INCLUDE '{WORKSPACE}/include'",
        "edit ECF_TRIES '1'",
        "edit YMD '%s'" % FIRST_DATE,
        "edit HH '00'",
        "edit SLEEP '1'",
        "edit MODEL_SLEEP '0.5'",
        "edit OUTPUT_STEP '6'",
    )
    for exp, kind, length, resolution, description in [
        ("ifc1", "fc", 24, "TCo399", "Forecast with the ecflow-chart deployment"),
        ("ian1", "an", 12, "TCo199", "Assimilation trial with the ecflow-chart deployment"),
    ]:
        main, main_name = (forecast_experiment if kind == "fc" else assimilation_experiment)(name, exp, length)
        root.add(family(
            exp,
            f'label INFO "{resolution} running in the server container"',
            f'label DESCRIPTION "{description}"',
            f"edit EXPVER '{exp}'",
            f"edit RESOL '{resolution}'",
        ).add(main, cancel_family(main_name)))
    return root


HEAD = """\
#!/usr/bin/env bash
# The head of every task script. The job runs inside the server container (the chart
# runs the jobs where the server runs), so the child commands reach the server over
# loopback, with the HTTP protocol the server speaks, and need no credentials.
set -e
set -u
set -x

export ECF_PORT=%ECF_PORT%
export ECF_HOST=localhost
export ECF_NAME=%ECF_NAME%
export ECF_PASS=%ECF_PASS%
export ECF_TRYNO=%ECF_TRYNO%
export ECF_RID=$$

ecf() {
    ecflow_client --http "$@"
}

ecf --init=$$

ERROR() {
    set +e
    wait
    ecf --abort=trap
    trap 0
    exit 0
}
trap ERROR 0
trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15
"""

EXPERIMENT = """\
# The environment of the experiment, shared by every task script
export EXPVER=%EXPVER%
export RESOL=%RESOL%
export YMD=%YMD%
export HH=%HH%
export SLEEP=%SLEEP%
export DATA=%ECF_HOME%/data/${EXPVER}
export CYCLE_DIR=${DATA}/${YMD}${HH}
mkdir -p ${CYCLE_DIR}
produce() {
    echo "${EXPVER} ${YMD}${HH} ${ECF_NAME} $2" > $1
}
echo "Experiment ${EXPVER} (${RESOL}), ${YMD} ${HH} UTC, task ${ECF_NAME} try ${ECF_TRYNO} in $(hostname) as $(id -un)"
"""

TAIL = """\
wait
ecf --complete
trap 0
exit 0
"""

SCRIPTS = {
    "default": """\
sleep ${SLEEP}
produce ${CYCLE_DIR}/%TASK%.done "done"
""",
    "flaky": """\
# Aborts on its first try, so that the server requeues it (ECF_TRIES 2), and completes on the second
if [ "${ECF_TRYNO}" = "1" ]; then
    echo "first try: failing on purpose"
    false
fi
sleep ${SLEEP}
produce ${CYCLE_DIR}/%TASK%.done "done on try ${ECF_TRYNO}"
""",
    "build": """\
mkdir -p ${DATA}/bin
sleep $(( SLEEP * 2 ))
produce ${DATA}/bin/ifsMASTER.DP "CY50R1"
ecf --event=installed
ecf --label=version "CY50R1"
""",
    "get_obs": """\
kind=%TASK%
kind=${kind#get_}
sleep ${SLEEP}
nobs=$(( 100000 + RANDOM * 20 ))
produce ${CYCLE_DIR}/obs_${kind}.bufr "${nobs} reports"
ecf --label=nobs "${nobs} ${kind} reports"
""",
    "min": """\
costf=250000
iter=0
while [ ${iter} -lt 20 ]; do
    sleep 0.2
    iter=$(( iter + 1 ))
    costf=$(( costf * 97 / 100 ))
    ecf --meter=iter ${iter}
done
ecf --label=costf "J=${costf} after ${iter} iterations"
produce ${CYCLE_DIR}/%TASK%.inc "increment"
""",
    "model": """\
step=0
while [ ${step} -le %FCLENGTH% ]; do
    sleep %MODEL_SLEEP%
    produce ${CYCLE_DIR}/ICMGG${EXPVER}+$(printf "%%06d" ${step}) "step ${step}"
    ecf --meter=step ${step}
    step=$(( step + %OUTPUT_STEP% ))
done
ecf --label=pace "$(( step / %OUTPUT_STEP% )) steps"
""",
    "archive": """\
sleep $(( SLEEP * 2 ))
produce ${CYCLE_DIR}/%TASK%.archived "archived expver=${EXPVER}"
ecf --event=fdb
""",
}


def write(path, content, executable=False):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    if executable:
        path.chmod(0o755)


def outputs(out, name):
    """The paths that --setup writes and --teardown removes: the definition, include/ and files/."""
    return [out / f"{name}.def", out / "include", out / "files"]


def setup(out, name):
    root = make_suite(name)
    write(out / f"{name}.def", f"# Written by make_suite.py\n{root.render()}\n")
    write(out / "include" / "head.h", HEAD)
    write(out / "include" / "experiment.h", EXPERIMENT)
    write(out / "include" / "tail.h", TAIL)
    count = 0
    for path, node in root.tasks():
        script = f"%include <head.h>\n%include <experiment.h>\n\n{SCRIPTS[node.script]}\n%include <tail.h>\n"
        write(out / "files" / (path.lstrip("/") + ".ecf"), script)
        count += 1
    print(f"{name}.def with {count} tasks, include/ and files/ written to {out}")


def teardown(out, name):
    """Removes the definition, include/ and files/ from the output directory, and nothing else."""
    for path in outputs(out, name):
        if path.is_dir():
            shutil.rmtree(path)
            print(f"removed {path}/")
        elif path.exists():
            path.unlink()
            print(f"removed {path}")


def main():
    parser = argparse.ArgumentParser(description="Write, or remove, the proof suite of an ecflow-chart deployment.")
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--setup", action="store_true", help="write the definition, include/ and files/ (the default)")
    action.add_argument("--teardown", action="store_true", help="remove what --setup wrote from the output directory")
    parser.add_argument("--out", default=".", type=pathlib.Path, help="output directory (default: here)")
    parser.add_argument("--name", default="proof", help="name of the suite (default: proof)")
    arguments = parser.parse_args()

    if arguments.teardown:
        teardown(arguments.out, arguments.name)
    else:
        setup(arguments.out, arguments.name)


if __name__ == "__main__":
    main()
