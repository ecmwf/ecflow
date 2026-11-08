# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import socket
from socket import gethostname
import subprocess
import os, sys, fnmatch
import datetime
import time
import shutil  # used to remove directory tree
import platform  # for python version info
import inspect
import pathlib
from enum import Enum

import ecflow as ecf


def all_files(root, patterns="*", single_level=False, yield_folders=False):
    """Expand patterns from semi-colon separated string to list"""
    patterns = patterns.split(";")
    for path, subdirs, files in os.walk(root):
        if yield_folders:
            files.extend(subdirs)
        files.sort()
        for name in files:
            for pattern in patterns:
                if fnmatch.fnmatch(name, pattern):
                    yield os.path.join(path, name)
                    break
        if single_level:
            break

        # Enable to stop data being deleted, and stop server from being terminated


def debugging():
    return False


def ecf_home(port):
    # Allowing multiple invocations of these tests, by creating a unique ECF_HOME based on:
    #  - python version
    #  - process id
    #  - build_type
    #  - server port,
    python_version = sys.version_info[0]
    build_type = "debug" if ecf.debug_build() else "release"
    pid = os.getpid()

    ecf_home_dir = (
        f"ecf_home_py{python_version}__pid_{pid}__build_{build_type}__port_{port}"
    )

    ecf_home_path = pathlib.Path.joinpath(
        pathlib.Path.cwd(), "test", "data", ecf_home_dir
    )

    return str(ecf_home_path.absolute())


def get_parent_dir(file_path):
    return os.path.dirname(file_path)


def log_file_path(port):
    return "./" + gethostname() + "." + port + ".ecf.log"


def checkpt_file_path(port):
    return "./" + gethostname() + "." + port + ".ecf.check"


def backup_checkpt_file_path(port):
    return "./" + gethostname() + "." + port + ".ecf.check.b"


def white_list_file_path(port):
    return "./" + gethostname() + "." + port + ".ecf.lists"


def print_test_start(test_name):
    print(
        "#######################################################################################"
    )
    print(test_name)
    print(
        "ecflow version("
        + ecf.Client().version()
        + ") debug build("
        + str(ecf.debug_build())
        + ")  python("
        + platform.python_version()
        + ")"
    )
    print(
        "PYTHONPATH                : " + str(os.environ["PYTHONPATH"].split(os.pathsep))
    )
    # print("sys.path                  : " + str(sys.path))
    print("Current working directory : " + str(os.getcwd()))
    print(
        "Python version            : "
        + str(sys.version_info[0])
        + "."
        + str(sys.version_info[1])
    )
    print(
        "#######################################################################################"
    )


def clean_up_server(port):
    print("   clean_up " + port)
    try:
        os.remove(log_file_path(port))
    except:
        pass
    try:
        os.remove(checkpt_file_path(port))
    except:
        pass
    try:
        os.remove(backup_checkpt_file_path(port))
    except:
        pass
    try:
        os.remove(white_list_file_path(port))
    except:
        pass


def clean_up_data(port):
    print("   Attempting to Removing ECF_HOME " + ecf_home(port))
    try:
        shutil.rmtree(ecf_home(port), True)  # True means ignore errors
        print("   Remove OK")
    except:
        print("   Remove Failed")


def listening_processes(port):
    """Describes each socket listening on the TCP port, with the process that owns
    it, as read from /proc; returns None where /proc is not available (macOS). A
    socket owned by a process of another user is described by its inode only."""
    if not os.path.exists("/proc/net/tcp"):
        return None
    inodes = []
    for table in ("/proc/net/tcp", "/proc/net/tcp6"):
        try:
            with open(table) as file:
                rows = file.read().splitlines()[1:]
        except OSError:
            continue
        for row in rows:
            fields = row.split()
            # local address (hex address:port), state (0A = LISTEN), socket inode
            if fields[3] == "0A" and int(fields[1].rsplit(":", 1)[1], 16) == port:
                inodes.append(fields[9])
    owners = {}
    if inodes:
        for pid in filter(str.isdigit, os.listdir("/proc")):
            try:
                for fd in os.listdir("/proc/" + pid + "/fd"):
                    link = os.readlink("/proc/" + pid + "/fd/" + fd)
                    if link.startswith("socket:[") and link[8:-1] in inodes:
                        with open("/proc/" + pid + "/cmdline", "rb") as file:
                            command = (
                                file.read()
                                .replace(b"\0", b" ")
                                .decode(errors="replace")
                            )
                        owners[link[8:-1]] = "pid " + pid + " (" + command.strip() + ")"
            except OSError:
                continue
    return [owners.get(inode, "socket inode " + inode) for inode in inodes]


def process_details(pid):
    """Describes where a process is, as read from /proc: its state, and for each
    thread the kernel function it waits in and its current system call (number and
    arguments), followed by its open files and sockets; returns None where /proc is
    not available (macOS)"""
    base = "/proc/" + str(pid)
    if not os.path.isdir(base):
        return None

    def read(path):
        try:
            with open(path, errors="replace") as file:
                return file.read().strip()
        except OSError as e:
            return "(" + e.strerror + ")"

    details = []
    for line in read(base + "/status").splitlines():
        if line.startswith(("State:", "Threads:")):
            details.append(" ".join(line.split()))
    try:
        tasks = sorted(os.listdir(base + "/task"), key=int)
    except OSError:
        tasks = []
    for tid in tasks:
        task = base + "/task/" + tid
        details.append(
            "thread "
            + tid
            + ": wchan="
            + read(task + "/wchan")
            + ", syscall="
            + read(task + "/syscall")
        )
    try:
        fds = sorted(os.listdir(base + "/fd"), key=int)
    except OSError:
        fds = []
    targets = []
    for fd in fds:
        try:
            targets.append(fd + "=" + os.readlink(base + "/fd/" + fd))
        except OSError:
            continue
    details.append("open files: " + ", ".join(targets))
    return details


# Test ports stay below the ephemeral range of Linux (32768-60999), from which
# outgoing connections take their source port
max_test_port = 32768


class Protocol(Enum):
    CUSTOM = 1  # uses custom TCP/IP access (default)
    HTTP = 2  # uses HTTP access


# =======================================================================================
class EcfPortLock(object):
    """Reserves a port for a test server, with the lock scheme of the C++ test
    scaffold: a <port>.lock file, shared by the test runs that use the same lock
    directory, followed by a bind probe, which also detects servers started by test
    runs using another lock directory."""

    def __init__(self):
        print("   EcfPortLock:__init__")
        self.lock_file_fp = None

    def at_time(self):
        return datetime.datetime.fromtimestamp(time.time()).strftime("%H:%M:%S")

    def find_free_port(self, seed_port):
        print(
            "   EcfPortLock:find_free_port starting with "
            + str(seed_port)
            + " at time "
            + self.at_time()
        )
        port = seed_port
        while 1:
            if port >= max_test_port:
                raise RuntimeError(
                    "No free test port from "
                    + str(seed_port)
                    + " up to the ephemeral range at "
                    + str(max_test_port)
                )
            # The lock is taken before probing, so that no test run using the same
            # lock directory can claim the port in between; a port bound by any
            # other process is rejected, releasing its lock
            if self.do_lock(port):
                if self.is_port_free(port):
                    print(
                        "   *FOUND* free server port "
                        + str(port)
                        + " : "
                        + self.at_time()
                    )
                    break
                print(
                    "   *Server* port "
                    + str(port)
                    + " busy (bind probe), trying next port "
                    + self.at_time()
                )
                self.remove(port)
            port = port + 1

        return str(port)

    @staticmethod
    def is_port_free(port):
        """Checks whether the given TCP port can be bound, as the ecFlow server binds
        it (IPv4, SO_REUSEADDR), whichever protocol a server already listening on it
        uses"""
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            try:
                probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                probe.bind(("", int(port)))
                return True
            except OSError:
                return False

    def do_lock(self, port):
        """Creates the lock file of the given port atomically (O_CREAT | O_EXCL), as
        the C++ test scaffold does, so that the lock holds across processes and on
        shared file systems without fcntl locks"""
        file = self._lock_file(port)
        try:
            fd = os.open(file, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
        except FileExistsError:
            print(
                "   *LOCKED* lock file exists "
                + file
                + " trying next port : "
                + self.at_time()
            )
            return False
        except OSError as e:
            # Every port would fail alike (e.g. a missing lock directory), so the
            # search stops here
            raise RuntimeError("Could not create lock file " + file + ": " + str(e))
        fp = None
        try:
            fp = os.fdopen(fd, "w")
            fp.write(self._describe_current_test())
            fp.flush()
        except BaseException:
            # A lock that cannot be described is not kept, so that the port stays
            # usable by later test runs; closing may fail again (e.g. on a full file
            # system), which must not keep the lock either
            try:
                if fp is not None:
                    fp.close()
                else:
                    os.close(fd)
            except OSError:
                pass
            os.remove(file)
            raise
        self.lock_time = self.at_time()
        self.lock_file_fp = fp
        print("   *LOCKED* file " + file + " : " + self.lock_time)
        return True

    def remove(self, port):
        """Releases the lock file of the given port; a lock already released is
        ignored, so that cleanup paths may release it unconditionally"""
        if self.lock_file_fp is None:
            return
        file = self._lock_file(port)
        print(
            "   Remove lock file : "
            + file
            + " : lock_time: "
            + self.lock_time
            + " release_time: "
            + self.at_time()
        )
        self.lock_file_fp.close()
        self.lock_file_fp = None
        os.remove(file)

    @staticmethod
    def _describe_current_test():
        """Describe the running test process, one "key: value" per line, so that a
        leftover lock file (e.g. after a killed test run) can be traced back to the
        test script that created it. Mirrors the description written by the C++ test scaffold."""
        created = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        # Under pytest, PYTEST_CURRENT_TEST names the test being executed (as "file::test (phase)");
        # otherwise the command line identifies the test script
        test = os.environ.get("PYTEST_CURRENT_TEST") or " ".join(sys.argv)
        return (
            "created: " + created + "\n"
            "host: " + gethostname() + "\n"
            "pid: " + str(os.getpid()) + "\n"
            "cwd: " + os.getcwd() + "\n"
            "executable: " + sys.executable + "\n"
            "test: " + test + "\n"
        )

    def _lock_file(self, port):
        if "ECF_PORT_LOCK_DIR" in os.environ:
            lock_file = os.environ["ECF_PORT_LOCK_DIR"] + "/" + str(port) + ".lock"
            # print("     EcfPortLock::_lock_file ECF_PORT_LOCK_DIR: " + lock_file)
            return lock_file
        if os.path.exists(ecf.File.source_dir()):
            lock_file = ecf.File.source_dir() + "/" + str(port) + ".lock"
            return lock_file
        lock_file = os.getcwd() + "/" + str(port) + ".lock"
        return lock_file


# ===============================================================================


class Server(object):
    """TestServer: allow debug and release version of python tests to run at the same
    time, by generating a unique port each time"""

    # Upper bound, in seconds, of the wait for a started server to answer
    start_timeout = 60

    def __init__(self, protocol=Protocol.CUSTOM):
        print("Server:__init__: Starting server")
        self._protocol = protocol
        self._process = None
        if not debugging():
            seed_port = int(os.getenv("ECF_FREE_PORT", "3153"))
            if sys.version_info[0] == 3:  # python3 can run at same time
                seed_port = int(os.getenv("ECF_FREE_PORT", "3154"))
            if ecf.debug_build():
                seed_port = seed_port + 1
            self.lock_file = EcfPortLock()
            self.the_port = self.lock_file.find_free_port(seed_port)
        else:
            self.the_port = "3152"

        # Only worth doing this test, if the server is running
        # ON HPUX, having only one connection attempt, sometimes fails
        # ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
        # ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
        self.ci = ecf.Client("localhost", self.the_port)
        if self._protocol == Protocol.HTTP:
            self.ci.enable_http()

    def at_time(self):
        return datetime.datetime.fromtimestamp(time.time()).strftime(
            "%Y-%m-%d %H:%M:%S"
        )

    def __enter__(self):
        try:
            while not self._start_own_server():
                # Another server answers on the port: a test run using another lock
                # directory started it in between, so the port is released and the
                # next one is tried
                if debugging():
                    raise RuntimeError(
                        "Another server answers on the debugging port " + self.the_port
                    )
                self.lock_file.remove(self.the_port)
                self.the_port = self.lock_file.find_free_port(int(self.the_port) + 1)
        except BaseException:
            # __exit__ is not called when __enter__ fails, so the server process and
            # the port reservation are released here
            self._stop_own_process(kill=True)
            if not debugging():
                self.lock_file.remove(self.the_port)
            raise

        print("   Run the tests, leaving Server:__enter__:")

        # return the Client, that can call to the server
        return [self.ci, self._protocol]

    def _make_client(self):
        ci = ecf.Client("localhost", self.the_port)
        if self._protocol == Protocol.HTTP:
            ci.enable_http()
        return ci

    def _start_own_server(self):
        """Starts a server on the reserved port, and returns whether the server
        answering on it is the one started; otherwise the started server is stopped"""
        self.ci = self._make_client()

        print("   ------- Start the server on port " + self.the_port + " ---------")
        if not debugging():
            clean_up_server(str(self.the_port))
            clean_up_data(str(self.the_port))

        server_exe = ecf.File.find_server()
        assert len(server_exe) != 0, "Could not locate the server executable"

        command = [server_exe, "--port=" + self.the_port, "--ecfinterval=4"]
        if self._protocol == Protocol.HTTP:
            command.append("--http")
        print("   TestClient.py: Starting server " + " ".join(command))
        self._process = subprocess.Popen(command)

        print("   Allow time for server to start " + self.at_time())
        if self._wait_for_own_server():
            print("   Server has started " + self.at_time())
            return True

        print(
            "   Server "
            + str(self._process.pid)
            + " is not the one answering on port "
            + self.the_port
            + ", trying next port !!!!!! "
            + self.at_time()
        )
        # Another server answers on the port, so the process started could not listen
        # on it: it has exited, or is about to exit, and is killed outright
        self._stop_own_process(kill=True)
        return False

    def _make_probe(self, protocol):
        ci = ecf.Client("localhost", self.the_port)
        if protocol == Protocol.HTTP:
            ci.enable_http()
        ci.set_connection_attempts(1)
        ci.set_retry_connection_period(0)
        return ci

    def _wait_for_own_server(self):
        """Waits for a server to answer on the port, and returns whether it is the
        process started; stops waiting as soon as that process exits, or as soon as
        a server of the other protocol answers (which cannot be the own server).
        Raises RuntimeError when no server answers within start_timeout seconds, or
        when the process exits while no other server listens on the port (the
        process then failed for a reason that another port would not change)."""
        other_protocol = (
            Protocol.CUSTOM if self._protocol == Protocol.HTTP else Protocol.HTTP
        )
        probe = self._make_probe(self._protocol)
        other_probe = self._make_probe(other_protocol)
        attempts = {"count": 0, "last": None, "errors": {}}
        deadline = time.monotonic() + self.start_timeout
        while time.monotonic() < deadline:
            if self._process.poll() is not None:
                if self._port_has_listener():
                    return False
                reason = (
                    "Server on port "
                    + self.the_port
                    + " exited with code "
                    + str(self._process.returncode)
                )
                self._print_start_diagnostics(reason, attempts)
                raise RuntimeError(reason)
            attempts["count"] += 1
            attempts["last"] = self.at_time()
            try:
                probe.ping()
            except RuntimeError as e:
                attempts["errors"][self._protocol.name] = str(e)
                try:
                    other_probe.ping()
                    return False
                except RuntimeError as e:
                    attempts["errors"][other_protocol.name] = str(e)
                    time.sleep(0.2)
                    continue
            return self._answering_server_is_own()
        reason = (
            "No server answered on port "
            + self.the_port
            + " within "
            + str(self.start_timeout)
            + " s"
        )
        self._print_start_diagnostics(reason, attempts)
        raise RuntimeError(reason)

    def _print_start_diagnostics(self, reason, attempts):
        """Prints what is known about a failed start, before the server log and the
        process are gone: the process state, the probes made, the listeners on the
        port and the end of the server log"""
        print("   Start diagnostics for port " + self.the_port + ": " + reason)
        code = self._process.poll()
        print(
            "      process "
            + str(self._process.pid)
            + ": "
            + ("running" if code is None else "exited with code " + str(code))
        )
        print(
            "      probes: "
            + str(attempts["count"])
            + ", the last at "
            + str(attempts["last"])
        )
        for protocol, error in sorted(attempts["errors"].items()):
            print("      last " + protocol + " error: " + error.strip())
        if code is None:
            details = process_details(self._process.pid)
            if details is None:
                print("      process details: not available on this platform")
            else:
                for detail in details:
                    print("      " + detail)
        print(
            "      listener on the port: "
            + ("yes" if self._port_has_listener() else "no")
        )
        listeners = listening_processes(int(self.the_port))
        if listeners is None:
            print("      listening processes: not available on this platform")
        else:
            print("      listening processes: " + (", ".join(listeners) or "none"))
        log_file = log_file_path(self.the_port)
        try:
            with open(log_file, errors="replace") as file:
                tail = file.readlines()[-20:]
        except OSError as e:
            print("      server log " + log_file + ": " + str(e))
        else:
            print(
                "      server log " + log_file + ", last " + str(len(tail)) + " lines:"
            )
            for line in tail:
                print("         " + line.rstrip())

    def _port_has_listener(self):
        """Checks whether any process accepts connections on the port"""
        try:
            with socket.create_connection(("localhost", int(self.the_port)), timeout=5):
                return True
        except OSError:
            return False

    def _answering_server_is_own(self):
        """Checks, through its ECF_PID, that the server answering on the port is the
        process started"""
        if self._process.poll() is not None:
            return False
        try:
            ci = self._make_client()
            ci.sync_local()
            variables = {v.name(): v.value() for v in ci.get_defs().server_variables}
        except RuntimeError as e:
            print("   Could not query the server variables: " + str(e))
            return False
        answering = variables.get("ECF_PID")
        if answering != str(self._process.pid):
            print(
                "   The server answering on port "
                + self.the_port
                + " has ECF_PID "
                + str(answering)
                + ", not "
                + str(self._process.pid)
            )
            return False
        return True

    def _stop_own_process(self, kill=False):
        """Waits for the server process to exit, killing it when it does not (or
        immediately, when asked); ecflow_server ignores SIGTERM, so only SIGKILL
        stops it"""
        if self._process is None:
            return
        if kill:
            self._process.kill()
        try:
            self._process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self._process.kill()
            self._process.wait()
        self._process = None

    def __exit__(self, exctype, value, tb):
        print(
            "Server:__exit__: Kill the server, clean up log file, check pt files and lock files, ECF_HOME "
            + self.at_time()
        )
        print("   exctype:", exctype)
        print("   value:", value)
        print("   tb:", tb)
        print("   Terminate server " + self.at_time())
        terminated = False
        try:
            # The server is terminated only while it is still the one answering on
            # the port, never a server that another test run started there since
            if self._process is not None and self._answering_server_is_own():
                self.ci.terminate_server()
                terminated = True
                print("   Terminate server OK " + self.at_time())
            else:
                print("   Server is no longer the one answering, not terminated")
        finally:
            self._stop_own_process(kill=not terminated)
            if not debugging():
                self.lock_file.remove(self.the_port)

        # Do not clean up data, if an assert was raised. This allow debug
        if exctype is None:
            if not debugging():
                clean_up_server(str(self.the_port))
                clean_up_data(str(self.the_port))
        return False


# ===============================================================================
class MockEnvironment:
    def __init__(self, env):
        self.expected_env = env

    def __enter__(self):
        self.original_env = {}
        for name, value in self.expected_env.items():
            if name in os.environ:
                self.original_env[name] = os.environ[name]
            else:
                self.original_env[name] = None
            os.environ[name] = value

    def __exit__(self, exception_type, exception_value, exception_traceback):
        for name, value in self.original_env.items():
            if value is None:
                del os.environ[name]
            else:
                os.environ[name] = value

    def __str__(self):
        return f"Test.MockEnvironment({self.expected_env})"


# ===============================================================================
class MockFile:
    def __init__(self, fname, fcontent=""):
        self.name = fname
        self.content = fcontent

    def __enter__(self):
        with open(self.name, "w") as file:
            file.write(self.content)

    def __exit__(self, exception_type, exception_value, exception_traceback):
        os.remove(self.name)

    def __str__(self):
        return f"Test.MockFile({self.name}, {self.content})"


# ==============================================================================
def name_this_test():
    """Set the name of the test based on the current function name"""
    test_name = inspect.stack()[1].function
    print(f"Test: {test_name}")


# ==============================================================================
def test_started():
    print_test_start(inspect.stack()[1].filename)


# ==============================================================================
def test_success():
    print(
        "#######################################################################################"
    )
    print(" All Tests passed")
    print(
        "#######################################################################################"
    )


def execute_all(clazz):
    test_started()

    for name, test in inspect.getmembers(clazz):
        if inspect.isfunction(test):
            test()

    test_success()
