# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# This test ensures that test servers started concurrently by separate test runs
# never share a port.
#
# Each run is a separate process, and all runs start the search from the same seed
# port at the same moment. The runs use either:
#  - separate lock directories, invisible to each other, as CI jobs on one node
#    that do not share a lock directory; or
#  - one shared lock directory, as the HPC CI jobs of one node.
# Each run must end up with a server of its own.

import multiprocessing
import os
import signal
import socket
import subprocess
import time

import pytest

import ecflow as ecf
import ecflow_test_util as Test

# Seed of the port search; see the port table in the testing rules
seed_port = 16000

# Upper bound, in seconds, of each wait for the other runs
timeout = 300


def server_pid(ci):
    """Returns the process id of the server, as the server reports it"""
    ci.sync_local()
    variables = {v.name(): v.value() for v in ci.get_defs().server_variables}
    return int(variables["ECF_PID"])


def run_reservation(index, lock_dir, protocol, start, results, release):
    """Reserves a test server, reports the run index, the port and the server process
    id, and keeps the server until released. A server that does not answer within
    the start timeout makes the reservation fail, and the run report the error.
    The run leads a process group of its own, which includes its server."""
    os.setpgid(0, 0)
    os.environ["ECF_PORT_LOCK_DIR"] = lock_dir
    os.environ["ECF_FREE_PORT"] = str(seed_port)
    start.wait(timeout)
    try:
        with Test.Server(protocol) as ctx:
            ci = ctx[0]
            results.put((index, ci.get_port(), server_pid(ci)))
            release.wait(timeout)
    except BaseException as e:
        results.put((index, "error", repr(e)))


CUSTOM, HTTP = Test.Protocol.CUSTOM, Test.Protocol.HTTP


@pytest.mark.parametrize(
    "protocols,shared",
    [
        ((CUSTOM, CUSTOM), False),
        ((HTTP, HTTP), False),
        ((CUSTOM, HTTP), False),
        ((CUSTOM, CUSTOM), True),
    ],
    ids=["custom", "http", "mixed", "shared-lock-directory"],
)
def test_concurrent_runs_get_distinct_servers(protocols, shared, tmp_path):
    # Separate lock directories model jobs that cannot see each other's locks;
    # a shared one models the jobs of one node in the HPC CI
    runs = len(protocols)
    context = multiprocessing.get_context("spawn")
    start = context.Barrier(runs)
    release = context.Event()
    results = context.Queue()

    processes = []
    for i, protocol in enumerate(protocols):
        lock_dir = tmp_path / ("shared" if shared else "run" + str(i))
        lock_dir.mkdir(exist_ok=True)
        process = context.Process(
            target=run_reservation,
            args=(i, str(lock_dir), protocol, start, results, release),
        )
        process.start()
        processes.append(process)

    reservations = []
    stuck = []
    try:
        for _ in range(runs):
            reservations.append(results.get(timeout=timeout))
    finally:
        release.set()
        for i, process in enumerate(processes):
            process.join(timeout)
            if process.is_alive():
                # A run that does not finish is stopped together with its process
                # group, and so with its server, which it can no longer terminate
                stuck.append(i)
                kill_group_if_running(process.pid)
                process.kill()
                process.join()

    assert stuck == [], "runs did not finish: " + str(stuck)
    errors = [r for r in reservations if r[1] == "error"]
    assert errors == []

    ports = sorted(port for _, port, _ in reservations)
    server_pids = sorted(pid for _, _, pid in reservations)
    assert len(set(ports)) == runs, "runs share a port: " + str(ports)
    assert len(set(server_pids)) == runs, "runs share a server: " + str(server_pids)


@pytest.fixture
def lock_dir(tmp_path, monkeypatch):
    path = tmp_path / "locks"
    path.mkdir()
    monkeypatch.setenv("ECF_PORT_LOCK_DIR", str(path))
    monkeypatch.setenv("ECF_FREE_PORT", str(seed_port))
    return path


def kill_group_if_running(pgid):
    try:
        os.killpg(pgid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def process_exists(pid):
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False


def wait_for_reply(ci):
    """Polls the server with a bounded wait, and returns whether it answered"""
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        try:
            ci.ping()
            return True
        except RuntimeError:
            time.sleep(0.2)
    return False


def wait_for_free_port(port):
    """Polls the port with a bounded wait, and returns whether it can be bound"""
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        if Test.EcfPortLock.is_port_free(port):
            return True
        time.sleep(0.2)
    return False


def test_exit_spares_a_server_started_by_another_run(lock_dir, tmp_path):
    # The own server dies during the test, and another run starts a server on the
    # same port: leaving the context must not terminate that other server
    other_home = tmp_path / "other"
    other_home.mkdir()
    other = None
    try:
        with Test.Server(CUSTOM) as ctx:
            port = ctx[0].get_port()
            os.kill(server_pid(ctx[0]), signal.SIGKILL)
            assert wait_for_free_port(port), "the own server still holds the port"
            other = subprocess.Popen(
                [ecf.File.find_server(), "--port=" + port, "--ecfinterval=4"],
                cwd=str(other_home),
            )
            ci = ecf.Client("localhost", port)
            ci.set_connection_attempts(1)
            assert wait_for_reply(ci), "the other server is not answering"
        assert wait_for_reply(ci), "the other server was terminated"
        assert other.poll() is None, "the other server was stopped"
        ci.terminate_server()
    finally:
        if other is not None:
            other.kill()
            other.wait()


def test_start_moves_on_when_another_server_holds_the_port(
    lock_dir, tmp_path, monkeypatch
):
    # Another run, using another lock directory, starts its server on the reserved
    # port after the bind probe: the own server cannot bind it, and the reservation
    # moves on to the next port (the helper starts the search one port higher in
    # debug builds)
    reserved_port = seed_port + 1 if ecf.debug_build() else seed_port
    other_home = tmp_path / "other"
    other_home.mkdir()
    other = subprocess.Popen(
        [ecf.File.find_server(), "--port=" + str(reserved_port), "--ecfinterval=4"],
        cwd=str(other_home),
    )
    try:
        ci = ecf.Client("localhost", str(reserved_port))
        ci.set_connection_attempts(1)
        assert wait_for_reply(ci), "the other server is not answering"
        monkeypatch.setattr(
            Test.EcfPortLock, "is_port_free", staticmethod(lambda port: True)
        )
        with Test.Server(CUSTOM) as ctx:
            assert ctx[0].get_port() != str(reserved_port)
            assert server_pid(ctx[0]) != other.pid
        assert other.poll() is None, "the other server was stopped"
    finally:
        other.kill()
        other.wait()


def test_start_fails_when_the_server_exits(lock_dir, tmp_path, monkeypatch, capsys):
    # A server that exits while no other server holds the port fails the start, as
    # another port would not change the outcome; the diagnostics show the exit code
    # and the end of the server log, which holds the cause
    work = tmp_path / "work"
    work.mkdir()
    monkeypatch.chdir(work)
    failing = tmp_path / "failing_server"
    failing.write_text(
        "#!/bin/sh\n"
        + "port=${1#--port=}\n"
        + 'echo "Could not bind port $port: Address already in use" >> "./'
        + socket.gethostname()
        + '.$port.ecf.log"\n'
        + "exit 3\n"
    )
    failing.chmod(0o755)
    monkeypatch.setattr(ecf.File, "find_server", staticmethod(lambda: str(failing)))
    with pytest.raises(RuntimeError, match="exited with code 3") as failure:
        with Test.Server(CUSTOM):
            pass
    port = str(failure.value).split()[3]
    out = capsys.readouterr().out
    assert "Start diagnostics for port " + port in out
    assert ": exited with code 3" in out
    assert "listener on the port: no" in out
    assert "Could not bind port " + port + ": Address already in use" in out
    assert list(lock_dir.iterdir()) == []


class FailingTerminate:
    """Stands in for the client of a server whose terminate request fails"""

    def terminate_server(self):
        raise RuntimeError("terminate failed")


def test_exit_kills_the_server_when_terminate_fails(lock_dir):
    server = Test.Server(CUSTOM)
    with pytest.raises(RuntimeError, match="terminate failed"):
        with server as ctx:
            pid = server_pid(ctx[0])
            server.ci = FailingTerminate()
    assert not process_exists(pid), "the server is still running"
    assert list(lock_dir.iterdir()) == []


def test_failed_start_releases_the_port(lock_dir, monkeypatch):
    monkeypatch.setattr(ecf.File, "find_server", staticmethod(lambda: ""))
    with pytest.raises(AssertionError, match="Could not locate the server"):
        with Test.Server(CUSTOM):
            pass
    assert list(lock_dir.iterdir()) == []


def test_search_stops_below_the_ephemeral_range(lock_dir, monkeypatch):
    monkeypatch.setenv("ECF_FREE_PORT", str(Test.max_test_port))
    with pytest.raises(RuntimeError, match="No free test port"):
        Test.Server(CUSTOM)
    assert list(lock_dir.iterdir()) == []


def test_unusable_lock_directory_fails_immediately(tmp_path, monkeypatch):
    # A lock directory that cannot hold lock files fails every port alike, so the
    # reservation must stop with an error instead of searching forever
    monkeypatch.setenv("ECF_PORT_LOCK_DIR", str(tmp_path / "missing"))
    monkeypatch.setenv("ECF_FREE_PORT", str(seed_port))
    with pytest.raises(RuntimeError, match="Could not create lock file"):
        Test.Server(CUSTOM)


def test_start_fails_when_the_server_never_answers(
    lock_dir, tmp_path, monkeypatch, capsys
):
    # A server that keeps running without ever answering fails the start, instead of
    # the reservation moving on from port to port; the diagnostics show that the
    # process was running without a listener, and why the probes failed
    pid_file = tmp_path / "silent.pid"
    silent = tmp_path / "silent_server"
    silent.write_text("#!/bin/sh\necho $$ > " + str(pid_file) + "\nexec sleep 600\n")
    silent.chmod(0o755)
    monkeypatch.setattr(ecf.File, "find_server", staticmethod(lambda: str(silent)))
    monkeypatch.setattr(Test.Server, "start_timeout", 2)
    with pytest.raises(RuntimeError, match="No server answered"):
        with Test.Server(CUSTOM):
            pass
    pid = int(pid_file.read_text())
    out = capsys.readouterr().out
    assert "process " + str(pid) + ": running" in out
    assert "listener on the port: no" in out
    assert "last CUSTOM error: " in out
    assert "last HTTP error: " in out
    if os.path.exists("/proc/net/tcp"):
        assert "      State: " in out
        assert "      thread " + str(pid) + ": wchan=" in out
    else:
        assert "process details: not available on this platform" in out
    assert not process_exists(pid)
    assert list(lock_dir.iterdir()) == []


def test_failed_lock_write_releases_the_port(lock_dir, monkeypatch):
    # A lock file whose description cannot be written (e.g. on a full file system)
    # is removed, so that the port stays usable by later test runs
    def fail():
        raise OSError("No space left on device")

    monkeypatch.setattr(Test.EcfPortLock, "_describe_current_test", staticmethod(fail))
    with pytest.raises(OSError, match="No space left"):
        Test.Server(CUSTOM)
    assert list(lock_dir.iterdir()) == []


def test_listening_processes_names_the_owner(lock_dir):
    # The start diagnostics name the process listening on the port, where /proc
    # tells it (Linux), and report the lookup as unavailable elsewhere
    reservation = Test.EcfPortLock()
    port = int(reservation.find_free_port(seed_port))
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
            # As the bind probe does, so that earlier connections in TIME_WAIT on
            # the port do not prevent the bind
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind(("", port))
            listener.listen()
            listeners = Test.listening_processes(port)
            if os.path.exists("/proc/net/tcp"):
                assert [x for x in listeners if x.startswith("pid " + str(os.getpid()))]
            else:
                assert listeners is None
        if os.path.exists("/proc/net/tcp"):
            assert Test.listening_processes(port) == []
    finally:
        reservation.remove(port)
    assert list(lock_dir.iterdir()) == []
