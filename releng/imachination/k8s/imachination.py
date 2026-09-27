#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

"""
Drives the `imachination` stack on a local Kubernetes cluster, providing the equivalent of `docker compose up`
for the kind-based deployment.

Every action is idempotent: running a command a second time either does nothing or converges on the same state.

The script is organised around commands: each command of the command line is a class whose `execute()` runs
one, or a sequence of, external commands (kind, kubectl, docker, ecflow_client). Every external command goes
through the `Runner`, which tells the commands that only read the state of the cluster (queries) from those that
change it (changes), and from those that wait for, or observe, the effect of changes (watches). With `--dryrun`,
the queries run, and the changes and the watches are shown instead of run; with `--verbose`, every command is shown
as it is about to run. Only the standard library of Python is used.
"""

import base64
import json
import os
import pathlib
import random
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

HERE = pathlib.Path(__file__).resolve().parent
IMACHINATION_DIR = HERE.parent

CLUSTER_NAME = "imachination"
NAMESPACE = "imachination"
CONTEXT = f"kind-{CLUSTER_NAME}"
CLUSTER_CONFIG = HERE / "kind-cluster.yaml"

# The administrator's files that a deployment starts from (server_environment.cfg), until
# `provision --role admin --target ecflow-server` replaces them
ADMIN_BASE_DIR = IMACHINATION_DIR / "ecflow" / "admin"

# The images named in the manifests, and those used in their place: the *_SOURCE variables select another image
# (a branch build, or one built locally), which is loaded into the cluster under its own name, and replaces the
# image of the manifests when the stack is applied. The node never pulls an image: the manifests use
# IfNotPresent, and every image is loaded beforehand.
IMAGES = {
    # manifest default                                                     environment variable
    "eccr.ecmwf.int/ecflow-dev-environments/ecflow-server-dev:latest": "ECFLOW_SOURCE",
    "eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.3.7": "AUTHOTRON_SOURCE",
    "eccr.ecmwf.int/ecflow-dev-environments/ecflow-revproxy-dev:latest": "REVPROXY_SOURCE",
    "eccr.ecmwf.int/ecflow-dev-environments/ecflow-sftp-dev:latest": "SFTP_SOURCE",
}

# The name under which the script is invoked
PROG = pathlib.Path(sys.argv[0]).name

##
# Log
##


class Failure(Exception):
    """An error already reported to the user, which ends the script with the given status."""

    def __init__(self, status=1):
        super().__init__(status)
        self.status = status


def info(message):
    print(f"\033[1m==> {message}\033[0m", flush=True)


def warn(message):
    print(f"\033[33m==> {message}\033[0m", file=sys.stderr, flush=True)


def die(message):
    print(f"\033[31m==> {message}\033[0m", file=sys.stderr, flush=True)
    raise Failure(1)


##
# Execute Process
##


class Runner:
    """
    Runs every external command of the script.

    A query only reads the state of the cluster, or of the host, and its outcome decides what the script does
    next; a change modifies that state; a watch waits for, or observes, the effect of changes (a rollout, the
    output of a workload, the authenticated path). In a dry run, the queries run, as they change nothing and the
    script depends on their outcome, while the changes and the watches are shown, quoted as a shell would need
    them, and not run: a watch means nothing after changes that did not happen.

    By default, the output of a command goes to the terminal; `capture` returns its standard output instead, and
    `quiet` discards it. Standard error is kept, unless `stderr` is set to `subprocess.DEVNULL` (or to
    `subprocess.STDOUT`, to merge it into the output). With `check`, a command that fails ends the script with its
    exit status, as a shell running with `set -e` would; without it, the completed process is returned, and its
    status left to the caller. `input` is written to the standard input of the command, and `env` holds the
    variables set for it, on top of the environment of the script. A command shown in a dry run is reported as
    successful, with no output. When verbose, every command is shown as it is about to run, in the same form as in a
    dry run.
    """

    def __init__(self):
        self.dryrun = False
        self.verbose = False
        # In a dry run, whether the creation of the cluster was shown, so that the commands that follow it are
        # shown as well, as if it existed
        self.cluster_created = False

    def query(self, argv, **options):
        return self._execute(argv, **options)

    def change(self, argv, **options):
        return self._run_or_show(argv, **options)

    def watch(self, argv, **options):
        return self._run_or_show(argv, **options)

    def _run_or_show(self, argv, **options):
        if not self.dryrun:
            return self._execute(argv, **options)
        self._show("dryrun", argv, options.get("input"), options.get("env"))
        return subprocess.CompletedProcess(argv, 0, stdout="", stderr="")

    def _execute(self, argv, **options):
        if self.verbose:
            self._show("run", argv, options.get("input"), options.get("env"))
        return self._run(argv, **options)

    @staticmethod
    def _run(argv, *, input=None, capture=False, quiet=False, stderr=None, check=True, env=None):
        stdout = subprocess.PIPE if capture else (subprocess.DEVNULL if quiet else None)
        environment = dict(os.environ, **env) if env else None
        process = subprocess.run(argv, input=input, stdout=stdout, stderr=stderr, env=environment, text=True)
        if check and process.returncode != 0:
            raise Failure(process.returncode)
        return process

    @staticmethod
    def _show(label, argv, input, env):
        variables = [f"{name}={shlex.quote(value)}" for name, value in (env or {}).items()]
        line = " ".join([*variables, shlex.join(argv)])
        if input:
            line += f"  # input: {describe(input)}"
        print(f"[{label}] {line}", flush=True)


def describe(document):
    """
    Names the Kubernetes objects of a manifest given as input to a command, without their content: a Secret built
    by the script is described by the names of its keys, never by their values.
    """
    try:
        documents = [json.loads(document)]
    except ValueError:
        documents = None
    if documents:
        described = []
        for item in documents:
            name = f"{item.get('kind')}/{item.get('metadata', {}).get('name')}"
            if item.get("kind") == "Secret":
                name += f" (keys: {', '.join(item.get('data', {})) or 'none'})"
            described.append(name)
        return "; ".join(described)
    described = []
    for part in re.split(r"^---$", document, flags=re.M):
        kind = re.search(r"^kind: (\S+)$", part, flags=re.M)
        name = re.search(r"^metadata:\n(?:  .*\n)*?  name: (\S+)$", part, flags=re.M)
        if kind and name:
            described.append(f"{kind.group(1)}/{name.group(1)}")
    return f"{len(described)} objects: " + ", ".join(described)


RUN = Runner()


def kubectl(*arguments):
    """Returns the command line of kubectl, in the context of the cluster."""
    return ["kubectl", "--context", CONTEXT, *arguments]


##
# Cluster and stack
##


def require(*tools):
    for tool in tools:
        if shutil.which(tool) is None:
            die(f"'{tool}' is required, but is not installed.")


def cluster_exists():
    if RUN.dryrun and RUN.cluster_created:
        return True
    process = RUN.query(["kind", "get", "clusters"], capture=True, stderr=subprocess.DEVNULL, check=False)
    return CLUSTER_NAME in process.stdout.splitlines()


def require_cluster(hint=True):
    if not cluster_exists():
        die(f"Cluster '{CLUSTER_NAME}' does not exist." + (f" Run '{PROG} cluster' first." if hint else ""))


def image_sources():
    """Returns the image of the manifests, and the image selected in its place, for each image of the stack."""
    return {default: os.environ.get(variable) or default for default, variable in IMAGES.items()}


def manifest(document):
    """Returns a Kubernetes object as the input of `kubectl apply -f -`."""
    return json.dumps(document)


def ensure_namespace():
    namespace = {"apiVersion": "v1", "kind": "Namespace", "metadata": {"name": NAMESPACE}}
    RUN.change(kubectl("apply", "-f", "-"), input=manifest(namespace), quiet=True)


def secret_from_tree(name, directory):
    """
    Returns a Secret holding every file below `directory`, each keyed by its path relative to `directory`, with
    "__" in place of "/" (a Secret has flat keys); hidden files are left out.
    """
    data = {}
    for path in sorted(pathlib.Path(directory).rglob("*")):
        if not path.is_file() or path.is_symlink() or path.name.startswith("."):
            continue
        relative = path.relative_to(directory)
        data["__".join(relative.parts)] = base64.b64encode(path.read_bytes()).decode()
    return {
        "apiVersion": "v1",
        "kind": "Secret",
        "type": "Opaque",
        "metadata": {"name": name, "namespace": NAMESPACE},
        "data": data,
    }


def render_stack():
    """
    Returns the objects of the stack, with the images of the manifests replaced by those selected by the *_SOURCE
    variables. Each image of the manifests is matched exactly, as the whole value of an `image:` field, which may
    open an item of a list (`- image:`).
    """
    rendered = RUN.query(["kubectl", "kustomize", str(IMACHINATION_DIR)], capture=True).stdout
    for default, source in image_sources().items():
        rendered = re.sub(rf"^([ -]*image: ){re.escape(default)}$", rf"\g<1>{source}", rendered, flags=re.M)
    return rendered


# A Deployment of the rendered stack, and its name
STACK_DEPLOYMENT = r"^kind: Deployment$\n(?:[^\n]*\n)*?metadata:\n(?:  [^\n]*\n)*?  name: (\S+)$"


def diagnose(workload):
    """Reports why a workload has not converged, from the events of its pods and what it last printed."""

    def tail(argv, lines):
        process = RUN.query(argv, capture=True, stderr=subprocess.STDOUT, check=False)
        for line in process.stdout.splitlines()[-lines:]:
            print(line, flush=True)

    warn(f"Diagnosis for {workload}")
    tail(kubectl("get", "pods", "-n", NAMESPACE), 1000)
    warn("Recent events")
    tail(kubectl("get", "events", "-n", NAMESPACE, "--sort-by=.lastTimestamp"), 10)
    warn(f"Last output of {workload}")
    tail(kubectl("logs", workload, "-n", NAMESPACE, "--tail=20", "--all-containers"), 20)


def wait_for_workloads(*names):
    """
    Waits for the named deployments to become available, or for every declared one when none is named, so that a
    workload that fails to start is reported here, rather than discovered later.
    """
    timeout = os.environ.get("ROLLOUT_TIMEOUT", "300s")
    if names:
        workloads = [f"deployment.apps/{name}" for name in names]
    elif RUN.dryrun:
        # The changes were not made, so that the cluster may not hold the workloads yet: those of the stack are
        # waited for
        workloads = [f"deployment.apps/{name}" for name in re.findall(STACK_DEPLOYMENT, render_stack(), flags=re.M)]
    else:
        process = RUN.query(
            kubectl("get", "deployments", "-n", NAMESPACE, "-o", "name"),
            capture=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        workloads = [line for line in process.stdout.splitlines() if line]

    failed = False
    for workload in workloads:
        info(f"Waiting for {workload} (timeout {timeout})")
        status = RUN.watch(
            kubectl("rollout", "status", workload, "-n", NAMESPACE, f"--timeout={timeout}"), check=False
        ).returncode
        if status != 0:
            diagnose(workload)
            failed = True
    if failed:
        die("One or more workloads did not become available.")


def current_pods(workload=None):
    """Returns the pods of the named workload, or of every workload when none is named, as pod/<name>."""
    selector = ["-l", f"app={workload}"] if workload else []
    process = RUN.query(
        kubectl("get", "pods", *selector, "-n", NAMESPACE, "-o", "name"),
        capture=True,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return [line for line in process.stdout.splitlines() if line]


def wait_for_removal(pods):
    """
    Waits for the given pods to be deleted. A rollout is complete as soon as the new pods are ready, while the old
    ones may still be stopping, and still receive connections through the node ports until the routing catches up;
    waiting for their removal ensures that every connection that follows reaches the new pods.
    """
    if not pods:
        return
    timeout = os.environ.get("ROLLOUT_TIMEOUT", "300s")
    info(f"Waiting for the replaced pods to stop (timeout {timeout})")
    RUN.watch(kubectl("wait", "--for=delete", *pods, "-n", NAMESPACE, f"--timeout={timeout}"), quiet=True)


##
# Command: 'cluster'
##


class ClusterCmd:
    """Creates the cluster, if absent."""

    def execute(self):
        require("kind", "kubectl", "docker")
        if cluster_exists():
            info(f"Cluster '{CLUSTER_NAME}' already exists; leaving it alone.")
        else:
            info(f"Creating cluster '{CLUSTER_NAME}'")
            RUN.change(["kind", "create", "cluster", "--config", str(CLUSTER_CONFIG)])
            RUN.cluster_created = True
        RUN.watch(kubectl("wait", "--for=condition=Ready", "nodes", "--all", "--timeout=180s"))


##
# Command: 'images'
##


class ImagesCmd:
    """
    Pulls the images on the host, and loads them into the cluster.

    Every image is published, for both linux/amd64 and linux/arm64, so that the variant of the host is pulled and
    no emulation is involved. An image already cached is not pulled, so that a working cluster does not depend on
    the registry; with `refresh`, as `reload` sets, it is pulled even when cached, so that a tag published again
    (such as `latest`) reaches the cluster, and an image that cannot be pulled, such as one built locally, is used
    as cached.
    """

    def __init__(self, refresh=False):
        self.refresh = refresh

    def execute(self):
        require("kind", "docker")
        require_cluster()
        for image in image_sources().values():
            self._ensure_pulled(image)
            info(f"Loading {image} into the cluster")
            RUN.change(["kind", "load", "docker-image", image, "--name", CLUSTER_NAME])

    def _ensure_pulled(self, image):
        inspected = RUN.query(
            ["docker", "image", "inspect", image], quiet=True, stderr=subprocess.DEVNULL, check=False
        )
        cached = inspected.returncode == 0
        if cached and not self.refresh:
            info(f"Already cached: {image}")
            return
        info(f"Pulling {image}")
        if RUN.change(["docker", "pull", image], check=False).returncode != 0:
            if not cached:
                die(f"Failed to pull {image}. A 'docker login eccr.ecmwf.int' may be required.")
            warn(f"Failed to pull {image}; using the cached image.")


##
# Command: 'apply'
##


class ApplyCmd:
    """
    Declares every object of the stack, and waits for the workloads to become available.

    Applying an unchanged tree again changes nothing, and an edited configuration file yields a new generated
    name, which replaces the pods that use it. The server Pod cannot start without the administrator's files: the
    base files are loaded when none are, and the files already loaded are kept.
    """

    def execute(self):
        require("kubectl")
        require_cluster()
        ensure_namespace()
        loaded = RUN.query(
            kubectl("get", "secret", "ecflow-admin", "-n", NAMESPACE),
            quiet=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if loaded.returncode != 0:
            info("Loading the base administrator's files into Secret ecflow-admin")
            RUN.change(kubectl("apply", "-f", "-"), input=manifest(secret_from_tree("ecflow-admin", ADMIN_BASE_DIR)))

        info("Applying the stack")
        RUN.change(kubectl("apply", "-f", "-"), input=render_stack())

        wait_for_workloads()
        if not RUN.dryrun:
            info("The stack is available.")


##
# Command: 'provision'
##


class ProvisionCmd:
    """
    Copies a directory of files to one of the destinations of the stack, as the role that owns it.

    The administrator (role admin) provisions, through the cluster, the files of the ecFlow server (target
    ecflow-server), which become the Secret ecflow-admin, expanded into /admin by the ecFlow server Pod
    (server_environment.cfg, troikaw, troika/<user>/, secrets/<user>/, ...), and the keys of the SFTP sidecar
    (target ecflow-sftp), which become the Secret sftp-keys (<user>.authorized_keys and the host key). Each
    replaces the content of its Secret as a whole, and the ecFlow server Pod is then replaced, so that it uses the
    new files.

    A user (role user) provisions the workspace (target workspace, the default) through the SFTP sidecar only,
    with no access to the cluster: the directory is copied into /workspace, merging into the directories that
    exist there. The user logs in with the private key <host>/<user>/id_ed25519, and checks the host key of the
    sidecar against <host>/known_hosts, where <host> is the directory given with --host, by default the sibling
    host/ of the provisioned directory.
    """

    TARGETS = {
        "admin": {"ecflow-server": "ecflow-admin", "ecflow-sftp": "sftp-keys"},
        "user": {"workspace": None},
    }
    OPTIONS = ("--role", "--target", "--dir", "--user", "--host")

    def __init__(self, arguments):
        options = {}
        remaining = list(arguments)
        while remaining:
            argument = remaining.pop(0)
            name, separator, value = argument.partition("=")
            if name not in self.OPTIONS:
                die(f"Unknown option for provision: {argument}. Try '{PROG} help'.")
            if not separator:
                if not remaining:
                    die(f"The option {name} requires a value.")
                value = remaining.pop(0)
            options[name.lstrip("-")] = value
        self.role = options.get("role")
        self.target = options.get("target")
        self.directory = options.get("dir")
        self.user = options.get("user") or os.environ.get("USER", "")
        self.host = options.get("host")

    def execute(self):
        if self.role not in self.TARGETS:
            die(f"The provision command requires --role {' or '.join(self.TARGETS)}. Try '{PROG} help'.")
        targets = self.TARGETS[self.role]
        if self.target is None and len(targets) == 1:
            self.target = next(iter(targets))
        if self.target not in targets:
            die(f"The role {self.role} provisions --target {' or '.join(targets)}.")
        if not self.directory:
            die(f"The provision command requires --dir. Try '{PROG} help'.")
        directory = pathlib.Path(self.directory).expanduser()
        if not directory.is_dir():
            die(f"No such directory: {self.directory}")
        directory = directory.resolve()

        if self.role == "admin":
            self._provision_secret(targets[self.target], directory)
        else:
            self._provision_workspace(directory)

    def _provision_secret(self, secret, directory):
        require("kubectl")
        require_cluster()
        ensure_namespace()
        info(f"Loading {directory} into Secret {secret}")
        RUN.change(kubectl("apply", "-f", "-"), input=manifest(secret_from_tree(secret, directory)))

        deployed = RUN.query(
            kubectl("get", "deployment/ecflow-server", "-n", NAMESPACE),
            quiet=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if deployed.returncode == 0:
            RestartCmd("ecflow-server").execute()
            if not RUN.dryrun:
                warn("The ecFlow server restarted halted: issue 'ecflow_client --https --restart' as an administrator.")

    def _provision_workspace(self, directory):
        require("sftp")
        if not self.user:
            die("The user role requires --user.")
        host = pathlib.Path(self.host).expanduser() if self.host else directory.parent / "host"
        key = host / self.user / "id_ed25519"
        known_hosts = host / "known_hosts"
        for path in (key, known_hosts):
            if not path.is_file():
                die(f"No such file: {path}")

        server = os.environ.get("SFTP_HOST", "localhost")
        port = os.environ.get("SFTP_PORT", "2222")
        sftp = [
            "sftp", "-q", "-P", port, "-i", str(key),
            "-o", "IdentitiesOnly=yes", "-o", "IdentityAgent=none", "-o", "BatchMode=yes",
            "-o", f"UserKnownHostsFile={known_hosts}",
            "-b", "-", f"{self.user}@{server}",
        ]
        accepted = RUN.query(sftp, input="pwd\n", quiet=True, stderr=subprocess.DEVNULL, check=False)
        if accepted.returncode != 0:
            die(f"The SFTP sidecar at {server}:{port} does not accept the key of '{self.user}' ({key}): the "
                "administrator provisions it with --role admin --target ecflow-sftp.")

        info(f"Copying {directory} into the workspace, as {self.user}@{server}:{port}")
        # put -r merges into the directories that already exist in the workspace
        RUN.change(sftp, input=f"lcd {directory}\nput -pr *\n", quiet=True)
        if not RUN.dryrun:
            info("The workspace is provisioned.")


##
# Command: 'restart'
##


class RestartCmd:
    """
    Replaces every workload, or the one named, so that it reads its configuration again.

    The ecFlow server reads server_environment.cfg only when it starts, and offers no command to reload
    ECF_PERMISSIONS, so that editing that file has no effect until the Pod is replaced. Only the workloads
    restarted are waited for, so that the report does not name workloads that were left running, and the command
    returns only once the replaced pods are gone.
    """

    def __init__(self, target=None):
        self.target = target

    def execute(self):
        require("kubectl")
        require_cluster(hint=False)
        if self.target:
            exists = RUN.query(
                kubectl("get", f"deployment/{self.target}", "-n", NAMESPACE),
                quiet=True,
                stderr=subprocess.DEVNULL,
                check=False,
            )
            if exists.returncode != 0:
                die(f"No such workload: {self.target}. Try '{PROG} status'.")
            replaced = current_pods(self.target)
            info(f"Restarting deployment/{self.target}")
            RUN.change(kubectl("rollout", "restart", f"deployment/{self.target}", "-n", NAMESPACE))
            wait_for_workloads(self.target)
        else:
            replaced = current_pods()
            info("Restarting every workload")
            RUN.change(kubectl("rollout", "restart", "deployment", "-n", NAMESPACE))
            wait_for_workloads()
        wait_for_removal(replaced)
        if not RUN.dryrun:
            info("The stack is available.")


##
# Command: 'up'
##


class UpCmd:
    """Brings the whole stack up from nothing: the cluster, the images and the workloads."""

    def execute(self):
        ClusterCmd().execute()
        ImagesCmd().execute()
        ApplyCmd().execute()


##
# Command: 'reload'
##


class ReloadCmd:
    """
    Pulls the images again, even when cached, loads them, applies the stack again, so that the images selected by
    the *_SOURCE variables replace those in use, and restarts every workload, or the one named, onto them.
    """

    def __init__(self, target=None):
        self.target = target

    def execute(self):
        ImagesCmd(refresh=True).execute()
        ApplyCmd().execute()
        RestartCmd(self.target).execute()


##
# Command: 'logs'
##


class LogsCmd:
    """Follows the output of one workload, or of every workload, each line prefixed with the pod it came from."""

    def __init__(self, target=None):
        self.target = target

    def execute(self):
        require("kubectl")
        require_cluster(hint=False)
        if self.target:
            RUN.watch(
                kubectl("logs", "-f", f"deployment/{self.target}", "-n", NAMESPACE, "--all-containers", "--tail=50")
            )
        else:
            RUN.watch(
                kubectl(
                    "logs",
                    "-f",
                    "-n",
                    NAMESPACE,
                    "-l",
                    "app in (ecflow-server, authotron, revproxy)",
                    "--all-containers",
                    "--prefix",
                    "--tail=20",
                    "--max-log-requests=6",
                )
            )


##
# Command: 'verify'
##


class VerifyCmd:
    """
    Exercises the authenticated path end to end with the real client, from the host: valid credentials reach the
    server, while missing and wrong credentials are refused by the reverse proxy, and leave the log of the server
    untouched.
    """

    SERVER = "https://localhost:443"
    LOG = "/workspace/ecflow-server.8888.ecf.log"

    def execute(self):
        require("ecflow_client")
        require_cluster(hint=False)
        user = os.environ.get("VERIFY_USER", "admin")
        password = os.environ.get("VERIFY_PASSWORD", "somesecret#admin")

        # The refused requests query a node path unique to this run, so that any of them reaching the server is
        # found in its log, whatever other clients (the readiness probe, ecflow_ui) write there meanwhile. The log
        # is in the workspace, on a volume of the cluster, and is searched within the Pod.
        marker = f"/imachination-verify-{os.getpid()}-{random.randrange(32768)}"
        self.failed = False

        with tempfile.TemporaryDirectory() as scratch:
            valid = self._tokens(scratch, "valid", [self._basic(user, password)])
            wrong = self._tokens(scratch, "wrong", [self._basic(user, f"{password}-wrong")])
            none = self._tokens(scratch, "none", [])

            outputs = [
                self._client(valid, "--ping"),
                self._client(none, "--query", "state", marker),
                self._client(wrong, "--query", "state", marker),
            ]
        if not RUN.dryrun:
            self._expect(f"valid credentials ({user}) reach the server", "succeeded", outputs[0])
            self._expect("no credentials are refused", "Unauthorized (401)", outputs[1])
            self._expect("a wrong password is refused", "Unauthorized (401)", outputs[2])
            time.sleep(1)

        # grep exits with 0 when the marker is found, 1 when it is not, and 2 when the log cannot be read; kubectl
        # exec passes the status on
        grep = RUN.watch(
            kubectl(
                "exec", "deployment/ecflow-server", "-n", NAMESPACE, "-c", "ecflow-server", "--",
                "grep", "-c", "-e", marker, self.LOG,
            ),
            capture=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if RUN.dryrun:
            return
        if grep.returncode == 0:
            warn(f"FAIL  a refused request reached the server: {grep.stdout.strip()} line(s) in its log")
            self.failed = True
        elif grep.returncode == 1:
            info("PASS  refused requests leave the server log untouched")
        else:
            warn(f"SKIP  server log not readable in the ecFlow server Pod: {self.LOG}")

        if self.failed:
            die("The authenticated path does not behave as expected.")
        info("The authenticated path behaves as expected.")

    def _basic(self, user, password):
        return {"type": "basic", "server": self.SERVER, "api": {"username": user, "password": password}}

    @staticmethod
    def _tokens(directory, name, tokens):
        """Writes an ecflowapirc file, readable by its owner only, and returns its path."""
        path = os.path.join(directory, name)
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
        with os.fdopen(descriptor, "w") as out:
            out.write(json.dumps({"version": 1, "tokens": tokens}) + "\n")
        return path

    @staticmethod
    def _client(tokens, *arguments):
        environment = {"ECF_AUTHTOKENS": tokens, "ECF_HOST": "localhost", "ECF_PORT": "443"}
        process = RUN.watch(
            ["ecflow_client", "--https", *arguments],
            capture=True,
            stderr=subprocess.STDOUT,
            check=False,
            env=environment,
        )
        return process.stdout

    def _expect(self, what, wanted, output):
        if wanted in output:
            info(f"PASS  {what}")
        else:
            warn(f"FAIL  {what}: " + " ".join(output.splitlines()[:2]) + " ")
            self.failed = True


##
# Command: 'down'
##


class DownCmd:
    """
    Deletes the stack, leaving the cluster and its loaded images in place. The volumes of the stack go with it:
    the workspace and the checkpoint are lost.
    """

    def execute(self):
        require("kubectl")
        require_cluster(hint=False)
        info(f"Deleting namespace '{NAMESPACE}'")
        RUN.change(kubectl("delete", "namespace", NAMESPACE, "--ignore-not-found"))


##
# Command: 'status'
##


class StatusCmd:
    """Reports the state of the cluster, of the images loaded into it, and of the objects of the stack."""

    def execute(self):
        require("kubectl")
        if not cluster_exists():
            warn(f"Cluster '{CLUSTER_NAME}' does not exist.")
            return
        info("Nodes")
        RUN.query(kubectl("get", "nodes"))

        info("Images loaded into the cluster")
        images = RUN.query(
            ["docker", "exec", f"{CLUSTER_NAME}-control-plane", "crictl", "images"],
            capture=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        pattern = re.compile(r"IMAGE|ecflow-dev-environments|auth-o-tron")
        lines = [line for line in images.stdout.splitlines() if pattern.search(line)]
        if lines:
            print("\n".join(lines), flush=True)
        else:
            warn("No images of the stack loaded yet.")

        info(f"Objects in namespace '{NAMESPACE}'")
        objects = RUN.query(
            kubectl("get", "all,configmap,secret", "-n", NAMESPACE),
            capture=True,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        lines = [line for line in objects.stdout.splitlines() if not line.startswith("configmap/kube-root-ca.crt")]
        if lines:
            print("\n".join(lines), flush=True)
        else:
            warn(f"Namespace '{NAMESPACE}' does not exist yet.")


##
# Command: 'destroy'
##


class DestroyCmd:
    """Deletes the cluster."""

    def execute(self):
        require("kind")
        if cluster_exists():
            info(f"Deleting cluster '{CLUSTER_NAME}'")
            RUN.change(["kind", "delete", "cluster", "--name", CLUSTER_NAME])
        else:
            info(f"Cluster '{CLUSTER_NAME}' does not exist; nothing to delete.")


##
# Command: 'help'
##


class UsageCmd:
    """Prints the usage of the script."""

    def execute(self):
        print(
            f"""Usage: {PROG} [--dryrun] [--verbose] <command> [arguments]

Options:
  --dryrun   Show the commands that change the cluster, or wait for their
             effect, instead of running them; those that only read its state
             still run
  --verbose  Show every command as it is about to run

Commands:
  up         Create the cluster, load the images and apply the stack, in one go
  cluster    Create the cluster, if absent
  images     Pull, tag and load the container images into the cluster
  apply      Declare the stack in the cluster and wait for it to become available
  provision --role admin --target ecflow-server|ecflow-sftp --dir DIR
             Load DIR as the files of the ecFlow server (/admin), or as the keys
             of the SFTP sidecar, replacing what was loaded before, then replace
             the ecFlow server Pod
  provision --role user [--target workspace] --dir DIR [--user NAME] [--host HOST]
             Copy DIR into the workspace over SFTP, as NAME (default: $USER),
             with the key HOST/NAME/id_ed25519 and HOST/known_hosts (default
             HOST: the directory host/ beside DIR)
  verify     Exercise the authenticated path with ecflow_client, from the host
  status     Report the state of the cluster, its images and the stack
  logs       Follow the output of every workload, or of the one named
  restart    Replace every workload, or the one named, so that it re-reads its
             configuration; required after editing server_environment.cfg
  reload     Pull the images again, even when cached, load them, apply the
             stack, and restart every workload, or the one named, onto them
  down       Delete the stack, with its workspace and checkpoint, keeping the
             cluster and its images
  destroy    Delete the cluster

Environment:
  ECFLOW_SOURCE      Source image for the ecFlow server, the equivalent of the
                     ECFLOW_IMAGE that compose.yaml accepts
  AUTHOTRON_SOURCE   Source image for the authentication service
  REVPROXY_SOURCE    Source image for the reverse proxy, the equivalent of the
                     REVPROXY_IMAGE that compose.yaml accepts
  SFTP_SOURCE        Source image for the SFTP sidecar of the ecFlow server
  SFTP_HOST, SFTP_PORT
                     SFTP sidecar, as published on the host, which provision
                     --role user connects to (default: localhost, 2222)
  ROLLOUT_TIMEOUT    How long to wait for a workload to become available
                     (default: 300s; an emulated server starts slowly)
  VERIFY_USER        User of the plain provider that verify authenticates as
  VERIFY_PASSWORD    Its password (default: the test user admin, as defined in
                     authotron/config.yaml)""",
            flush=True,
        )


##
#
#
# Main
#
#
##


def main(argv):
    options = {"--dryrun", "--verbose"}
    arguments = [value for value in argv[1:] if value not in options]
    RUN.dryrun = "--dryrun" in argv[1:]
    RUN.verbose = "--verbose" in argv[1:]
    command = arguments[0] if arguments else ""
    argument = arguments[1] if len(arguments) > 1 else ""

    prototypes = {
        "up": lambda: UpCmd(),
        "cluster": lambda: ClusterCmd(),
        "images": lambda: ImagesCmd(),
        "apply": lambda: ApplyCmd(),
        "provision": lambda: ProvisionCmd(arguments[1:]),
        "verify": lambda: VerifyCmd(),
        "status": lambda: StatusCmd(),
        "logs": lambda: LogsCmd(argument),
        "restart": lambda: RestartCmd(argument),
        "reload": lambda: ReloadCmd(argument),
        "down": lambda: DownCmd(),
        "destroy": lambda: DestroyCmd(),
        "": lambda: UsageCmd(),
        "-h": lambda: UsageCmd(),
        "--help": lambda: UsageCmd(),
        "help": lambda: UsageCmd(),
    }

    try:
        if RUN.dryrun and command in prototypes and prototypes[command]().__class__ is not UsageCmd:
            info("Dry run: the commands that change the cluster, or wait for their effect, are shown, not run")
        if command not in prototypes:
            UsageCmd().execute()
            die(f"Unknown command: {command}")
        prototypes[command]().execute()
    except Failure as failure:
        return failure.status
    except KeyboardInterrupt:
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
