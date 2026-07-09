#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures the User API works as expected.

import time
import os
import pwd
import sys
import threading
from datetime import datetime
import shutil  # used to remove directory tree

import pytest

# ecflow_test_util, see File ecflow_test_util.py
import ecflow_test_util as Test
from ecflow import (
    Defs,
    Suite,
    Family,
    Task,
    Edit,
    Meter,
    Clock,
    DState,
    Style,
    State,
    RepeatDate,
    RepeatDateTime,
    PrintStyle,
    File,
    Client,
    SState,
    CheckPt,
    Cron,
    Late,
    Flag,
    FlagType,
)

import re
import platform


def disable_on(regex_platform):
    def decorator(func):
        def wrapper(*args, **kwargs):
            if re.match(regex_platform, platform.platform()):
                print(f"Skipping {func.__name__} due to match with {regex_platform}")
            else:
                return func(*args, **kwargs)

        return wrapper

    return decorator


def ecf_includes():
    return os.getcwd() + "/test/data/includes"


def debugging():
    return False  # Use to enable auto flush and disable log file tests


def create_defs(name, port, protocol):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0:
        suite_name = "s1"
    suite = defs.add_suite(suite_name)

    ecfhome = Test.ecf_home(port)
    suite.add_variable("ECF_HOME", ecfhome)
    protocol_option = "--http" if protocol == Test.Protocol.HTTP else ""
    suite.add_variable("ECF_CLIENT_EXE_PATH", f"{File.find_client()} {protocol_option}")
    suite.add_variable("SLEEP", "1")
    # not strictly required since default is 1 second
    suite.add_variable("ECF_INCLUDE", ecf_includes())

    family = suite.add_family("f1")
    family.add_task("t1")
    family.add_task("t2")
    return defs


def _can_set_host_port(ci, host, port):
    try:
        ci.set_host_port(host, port)
        return True
    except RuntimeError:
        return False


def _can_set_combined_host_port(ci, host_port):
    try:
        ci.set_host_port(host_port)
        return True
    except RuntimeError:
        return False


def _can_construct_client_with_host_port(host, port):
    try:
        Client(host, port)
        return True
    except RuntimeError:
        return False


def _can_construct_client_with_combined_host_port(host_port):
    try:
        Client(host_port)
        return True
    except RuntimeError:
        return False


def sync_local(ci):
    if ci.is_auto_sync_enabled():
        return
    ci.sync_local()


def print_test(ci, test_name):
    print(f"=====> Running: {test_name}", flush=True)
    if ci.is_auto_sync_enabled():
        ci.log_msg(
            test_name
            + " ============= AUTO SYNC ENABLED ================================ "
        )
    else:
        ci.log_msg(
            test_name
            + " ================================================================ "
        )


def get_username():
    return pwd.getpwuid(os.getuid())[0]


class CustomStdOut:

    def __init__(self, stream=sys.stdout):
        self.marker = "<<<OUTPUT CAPTURE END>>>"  # marker to indicate end of capture
        self.captured = ""
        # create a pipe to capture content temporarily
        self.o_pipe, self.i_pipe = os.pipe()
        # store the stream
        self.stream = sys.stdout if stream is None else stream
        self.stream_no = self.stream.fileno()

    def __enter__(self):
        self.captured = ""
        # make a copy of the original stdout
        self.backup_no = os.dup(self.stream_no)
        # redirect stdout into the write pipe
        os.dup2(self.i_pipe, self.stream_no)

        # start a worker thread to capture the output
        self.worker = threading.Thread(target=self._capture_output)
        self.worker.start()

        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        # write the end marker to indicate the end of capture
        self.stream.write(self.marker)
        self.stream.flush()

        # wait for the worker threadto finish capturing output
        self.worker.join()

        # close the temporary pipe
        os.close(self.i_pipe)
        os.close(self.o_pipe)
        # restore the original output fd
        os.dup2(self.backup_no, self.stream_no)

    def _capture_output(self):
        while True:
            self.captured += os.read(self.o_pipe, 1).decode(self.stream.encoding)
            if self.captured.endswith(self.marker):
                # we reached the end marker, so we stop capturing and remove the marker
                self.captured = self.captured[: -len(self.marker)]
                break

    def value(self):
        return self.captured


@pytest.fixture(
    scope="class",
    params=[Test.Protocol.CUSTOM, Test.Protocol.HTTP],
    ids=["custom", "http"],
)
def protocol(request):
    return request.param


@pytest.fixture(scope="class", params=[False, True], ids=["sync_local", "auto_sync"])
def auto_sync(request):
    return request.param


@pytest.fixture(scope="class")
def server(protocol, auto_sync):
    with Test.Server(protocol) as ctx:
        ci, protocol = ctx
        server_version = ci.server_version()
        print("Running ecflow server version " + server_version)
        print("Running ecflow client version " + ci.version())
        assert (
            ci.version() == server_version
        ), " Client version not same as server version"
        PrintStyle.set_style(Style.STATE)  # show node state
        if auto_sync:
            ci.set_auto_sync(True)
        yield ci, protocol


class TestClientConstruction:
    def test_set_host_port(self):
        print("test_set_host_port")
        ci = Client()
        ci.debug(True)
        ci.enable_http()
        print(" Client.get_host() = " + ci.get_host())
        print(" Client.get_port() = " + ci.get_port())
        assert _can_set_host_port(ci, "host", "3141"), "Expected no errors"
        assert _can_set_host_port(ci, "host", 4444), "Expected no errors"
        assert _can_set_combined_host_port(ci, "host:4444"), "Expected no errors"
        assert _can_set_combined_host_port(ci, "host@4444"), "Expected no errors"
        assert _can_set_host_port(ci, "", "") == False, "Expected errors"
        assert _can_set_host_port(ci, "host", "") == False, "Expected errors"
        assert _can_set_host_port(ci, "host", "host") == False, "Expected errors"
        assert _can_set_combined_host_port(ci, "host:host") == False, "Expected errors"
        assert _can_set_combined_host_port(ci, "3141:host") == False, "Expected errors"
        assert _can_set_combined_host_port(ci, "3141@host") == False, "Expected errors"
        assert _can_set_combined_host_port(ci, "3141@") == False, "Expected errors"

        assert _can_construct_client_with_host_port(
            "host", "3141"
        ), "Expected no errors"
        assert _can_construct_client_with_host_port("host", 4444), "Expected no errors"
        assert _can_construct_client_with_combined_host_port(
            "host:4444"
        ), "Expected no errors"
        assert _can_construct_client_with_host_port("", "") == False, "Expected errors"
        assert (
            _can_construct_client_with_host_port("host", "") == False
        ), "Expected errors"
        assert (
            _can_construct_client_with_host_port("host", "host") == False
        ), "Expected errors"
        assert (
            _can_construct_client_with_combined_host_port("host:host") == False
        ), "Expected errors"
        assert (
            _can_construct_client_with_combined_host_port("3141:host") == False
        ), "Expected errors"


class TestClientApi:
    @pytest.fixture(autouse=True)
    def setup(self, server):
        self.ci, self.protocol = server

    def test_version(self):
        print_test(self.ci, "test_version")
        client_version = self.ci.version()
        server_version = self.ci.server_version()
        print("  client_version: ", client_version)
        print("  server_version: ", server_version)
        assert client_version == server_version, (
            "Expected client version("
            + client_version
            + ") and server version("
            + server_version
            + ") to match\n"
        )

    def test_client_get_server_defs(self):
        print_test(self.ci, "test_client_get_server_defs")
        self.ci.delete_all()  # start fresh
        self.ci.load(create_defs("", self.ci.get_port(), self.protocol))
        self.ci.get_server_defs()
        assert (
            self.ci.get_defs().find_suite("s1") is not None
        ), "Expected to find suite of name s1:\n" + str(self.ci.get_defs())

        self.ci.delete_all()  # start fresh
        self.ci.load(create_defs("", self.ci.get_port(), self.protocol))
        sync_local(self.ci)

        assert (
            self.ci.get_defs().find_suite("s1") is not None
        ), "Expected to find suite of name s1:\n" + str(self.ci.get_defs())

    def test_client_new_log(self):
        print_test(self.ci, "test_client_new_log")
        log_path = Test.log_file_path(self.ci.get_port())
        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

        new_log_file_name = "./test_client_new_log_" + str(os.getpid()) + ".log"
        try:
            os.remove(new_log_file_name)  # delete file if it exists
        except:
            pass

        self.ci.new_log(new_log_file_name)
        self.ci.flush_log()  # close log file and force write to disk
        assert os.path.exists(new_log_file_name), (
            new_log_file_name + " : New log does not exist"
        )
        try:
            os.remove(new_log_file_name)
        except:
            pass

        # reset new log to original
        self.ci.new_log(log_path)
        self.ci.ping()
        self.ci.flush_log()  # close log file and force write to disk
        log_file = open(log_path)
        try:
            log_text = log_file.read()
            # assume log file not to big
        finally:
            log_file.close()
        assert log_text.find("--ping") != -1, "Expected to find --ping in log file"

        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

    def test_client_clear_log(self):
        if debugging():
            return  # dont run this test when debugging as log file is lost
        print_test(self.ci, "test_client_clear_log")
        log_path = Test.log_file_path(self.ci.get_port())
        # populate log
        self.ci.ping()
        self.ci.ping()
        self.ci.flush_log()  # close log file and force write to disk
        log_file = open(log_path)
        try:
            log_text = log_file.read()
            # assume log file not to big
        finally:
            log_file.close()
        assert log_text.find("--ping") != -1, "Expected to find --ping in log file"

        self.ci.clear_log()
        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

        log_file = open(log_path)
        try:
            log_text = log_file.read()
            # assume log file not to big
        finally:
            log_file.close()
        assert len(log_text) == 0, "Expected log file to be empty but found " + log_text

        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

    def test_client_log_msg(self):
        if debugging():
            return  # dont run this test when debugging

        print_test(self.ci, "test_client_log_msg")
        log_path = Test.log_file_path(self.ci.get_port())
        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

        # Send a message to the log file, then make sure it was written
        self.ci.log_msg("Humpty dumpty sat on a wall!")
        self.ci.flush_log()
        # flush and close log file, so we can open it
        log_file = open(log_path)
        try:
            log_text = log_file.read()
            # assume log file not to big
        finally:
            log_file.close()
        assert (
            log_text.find("Humpty dumpty sat on a wall!") != -1
        ), "Expected to find Humpty dumpty in the log file"

        if not os.path.exists(log_path):
            print(log_path + " : log does not exist ?")

    def test_client_restart_server(self):
        print_test(self.ci, "test_client_restart_server")
        self.ci.restart_server()
        sync_local(self.ci)
        assert (
            self.ci.get_defs().get_server_state() == SState.RUNNING
        ), "Expected server to be running"

        paths = list(self.ci.changed_node_paths)
        assert len(paths) == 1, "expected changed node to be the root node: " + paths
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])

    def test_client_halt_server(self):
        print_test(self.ci, "test_client_halt_server")
        self.ci.halt_server()
        sync_local(self.ci)
        assert (
            self.ci.get_defs().get_server_state() == SState.HALTED
        ), "Expected server to be halted"

        paths = list(self.ci.changed_node_paths)
        assert len(paths) == 1, "expected changed node to be the root node"
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])
        self.ci.restart_server()

    def test_client_shutdown_server(self):
        print_test(self.ci, "test_client_shutdown_server")
        self.ci.shutdown_server()
        sync_local(self.ci)
        assert (
            self.ci.get_defs().get_server_state() == SState.SHUTDOWN
        ), "Expected server to be shutdown"

        paths = list(self.ci.changed_node_paths)
        assert len(paths) == 1, "expected changed node to be the root node"
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])

    def test_client_load_in_memory_defs(self):
        print_test(self.ci, "test_client_load_in_memory_defs")
        self.ci.delete_all()  # start fresh
        self.ci.load(create_defs("", self.ci.get_port(), self.protocol))
        sync_local(self.ci)
        assert (
            self.ci.get_defs().find_suite("s1") is not None
        ), "Expected to find suite of name s1:\n" + str(self.ci.get_defs())

    def test_client_load_from_disk(self):
        print_test(self.ci, "test_client_load_from_disk")
        self.ci.delete_all()  # start fresh
        defs = create_defs("", self.ci.get_port(), self.protocol)
        defs_file = "test_client_load_from_disk_" + str(os.getpid()) + ".def"
        defs.save_as_defs(defs_file)
        assert os.path.exists(defs_file), (
            "Expected file " + defs_file + " to exist after defs.save_as_defs()"
        )
        self.ci.load(defs_file)  # open and parse defs file, and load into server.\n"

        sync_local(self.ci)
        assert (
            self.ci.get_defs().find_suite("s1") is not None
        ), "Expected to find suite of name s1:\n" + str(self.ci.get_defs())
        os.remove(defs_file)

    def test_client_checkpt(self):
        print_test(self.ci, "test_client_checkpt")
        # start fresh
        self.ci.delete_all()
        port = self.ci.get_port()
        try:
            os.remove(Test.checkpt_file_path(port))
            os.remove(Test.backup_checkpt_file_path(port))
        except:
            pass

        self.ci.load(create_defs("", self.ci.get_port(), self.protocol))
        self.ci.checkpt()
        assert os.path.exists(
            Test.checkpt_file_path(port)
        ), "Expected check pt file to exist after ci.checkpt()"
        assert (
            os.path.exists(Test.backup_checkpt_file_path(port)) == False
        ), "Expected back up check pt file to *NOT* exist"

        self.ci.checkpt()  # second check pt should cause backup check pt to be written
        assert os.path.exists(
            Test.backup_checkpt_file_path(port)
        ), "Expected back up check pt file to exist after second ci.checkpt()"

        self.ci.checkpt(CheckPt.NEVER)  # switch of check pointing
        self.ci.checkpt(CheckPt.ALWAYS)  # always check point, at any state change
        self.ci.checkpt(
            CheckPt.ON_TIME
        )  # Check point periodically, by interval set in server
        self.ci.checkpt(
            CheckPt.ON_TIME, 200
        )  # Check point periodically, by interval set in server
        self.ci.checkpt(CheckPt.UNDEFINED, 0, 35)  # Change check point save time alarm

        os.remove(Test.checkpt_file_path(port))
        os.remove(Test.backup_checkpt_file_path(port))

    def test_client_restore_from_checkpt(self):
        print_test(self.ci, "test_client_restore_from_checkpt")
        # start fresh
        self.ci.delete_all()
        port = self.ci.get_port()
        try:
            os.remove(Test.checkpt_file_path(port))
            os.remove(Test.backup_checkpt_file_path(port))
        except:
            pass

        self.ci.load(create_defs("", self.ci.get_port(), self.protocol))
        self.ci.checkpt()
        self.ci.delete_all()

        sync_local(self.ci)
        assert (
            self.ci.get_defs().find_suite("s1") is None
        ), "Expected all suites to be delete:\n"

        self.ci.halt_server()  # server must be halted, otherwise restore_from_checkpt will throw
        self.ci.restore_from_checkpt()

        sync_local(self.ci)
        assert (
            self.ci.get_defs().find_suite("s1") is not None
        ), "Expected to find suite s1 after restore from checkpt:\n" + str(
            self.ci.get_defs()
        )

        os.remove(Test.checkpt_file_path(port))
        self.ci.restart_server()

    def test_client_reload_wl_file(self):
        print_test(self.ci, "test_client_reload_wl_file")

        expected = False
        try:
            self.ci.reload_wl_file()
        except:
            expected = True
        assert expected, "Expected reload to fail when no white list specified"

        # create a white list file
        port = self.ci.get_port()
        wl_file = open(Test.white_list_file_path(port), "w")
        wl_file.write("#\n")
        wl_file.write("4.4.14   #   comment\n\n")
        wl_file.write("# These user have read and write access to the server\n")
        wl_file.write(
            get_username() + "\n"
        )  # add current user otherwise remaining test's, wont have access from server anymore
        wl_file.write("axel  # admin\n")
        wl_file.write("john  # admin\n\n")
        wl_file.write("# Read only users\n")
        wl_file.write("-fred   # needs read access only\n")
        wl_file.write("-joe90  # needs read access only\n")
        wl_file.close()

        self.ci.reload_wl_file()
        os.remove(Test.white_list_file_path(port))

    def test_client_run(self):
        print_test(self.ci, "test_client_run")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_client_run", port, self.protocol)
        suite = defs.find_suite("test_client_run")
        suite.add_defstatus(DState.suspended)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()
        self.ci.run("/test_client_run", False)

        count = 0
        while 1:
            count += 1
            self.ci.sync_local()  # get the changes, synced with local defs
            suite = self.ci.get_defs().find_suite("test_client_run")
            assert suite is not None, "Expected to find suite test_client_run:\n" + str(
                self.ci.get_defs()
            )
            print(f"Suite state: {suite.get_state()}, {count}, at {datetime.now()}")
            if suite.get_state() == State.complete:
                break
            assert (
                suite.get_state() != State.aborted
            ), "Expected suite to complete, but found it was aborted"
            time.sleep(3)
            if count > 20:
                assert False, (
                    "test_client_run aborted after "
                    + str(count)
                    + " loops:\n"
                    + str(self.ci.get_defs())
                )

        self.ci.log_msg("Looped " + str(count) + " times")
        dir_to_remove = Test.ecf_home(port) + "/" + "test_client_run"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_run_with_multiple_paths(self):
        print_test(self.ci, "test_client_run_with_multiple_paths")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_client_run_with_multiple_paths", port, self.protocol)
        suite = defs.find_suite("test_client_run_with_multiple_paths")
        suite.add_defstatus(DState.suspended)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()
        path_list = [
            "/test_client_run_with_multiple_paths/f1/t1",
            "/test_client_run_with_multiple_paths/f1/t2",
        ]
        self.ci.run(path_list, False)

        count = 0
        while 1:
            count += 1
            self.ci.sync_local()  # get the changes, synced with local defs
            suite = self.ci.get_defs().find_suite("test_client_run_with_multiple_paths")
            assert (
                suite is not None
            ), "Expected to find suite test_client_run_with_multiple_paths:\n" + str(
                self.ci.get_defs()
            )
            if suite.get_state() == State.complete:
                break
            time.sleep(3)
            if count > 20:
                assert False, (
                    "test_client_run_with_multiple_paths aborted after "
                    + str(count)
                    + " loops:\n"
                    + str(self.ci.get_defs())
                )

        self.ci.log_msg("Looped " + str(count) + " times")

        dir_to_remove = (
            Test.ecf_home(port) + "/" + "test_client_run_with_multiple_paths"
        )
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_requeue(self):
        print_test(self.ci, "test_client_requeue")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_client_requeue", port, self.protocol)
        suite = defs.find_suite("test_client_requeue")
        suite.add_defstatus(DState.suspended)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()

        self.ci.force_state_recursive("/test_client_requeue", State.unknown)
        sync_local(self.ci)
        suite = self.ci.get_defs().find_suite("test_client_requeue")
        assert (
            suite.get_state() == State.unknown
        ), "Expected to find suite with state unknown"

        self.ci.requeue("/test_client_requeue")
        sync_local(self.ci)
        suite = self.ci.get_defs().find_suite("test_client_requeue")
        assert (
            suite.get_state() == State.queued
        ), "Expected to find suite with state queued"

        dir_to_remove = Test.ecf_home(port) + "/" + "test_client_requeue"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_requeue_with_multiple_paths(self):
        print_test(self.ci, "test_client_requeue_with_multiple_paths")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs(
            "test_client_requeue_with_multiple_paths", port, self.protocol
        )
        suite = defs.find_suite("test_client_requeue_with_multiple_paths")
        suite.add_defstatus(DState.suspended)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()

        self.ci.force_state_recursive(
            "/test_client_requeue_with_multiple_paths", State.unknown
        )
        sync_local(self.ci)
        task1 = self.ci.get_defs().find_abs_node(
            "/test_client_requeue_with_multiple_paths/f1/t1"
        )
        task2 = self.ci.get_defs().find_abs_node(
            "/test_client_requeue_with_multiple_paths/f1/t2"
        )
        assert (
            task1.get_state() == State.unknown
        ), "Expected to find t1 with state unknown"
        assert (
            task2.get_state() == State.unknown
        ), "Expected to find t2 with state unknown"

        path_list = [
            "/test_client_requeue_with_multiple_paths/f1/t1",
            "/test_client_requeue_with_multiple_paths/f1/t2",
        ]
        self.ci.requeue(path_list)
        sync_local(self.ci)
        task1 = self.ci.get_defs().find_abs_node(
            "/test_client_requeue_with_multiple_paths/f1/t1"
        )
        task2 = self.ci.get_defs().find_abs_node(
            "/test_client_requeue_with_multiple_paths/f1/t2"
        )
        assert (
            task1.get_state() == State.queued
        ), "Expected to find task t1 with state queued"
        assert (
            task2.get_state() == State.queued
        ), "Expected to find task t2 with state queued"

        dir_to_remove = (
            Test.ecf_home(port) + "/" + "test_client_requeue_with_multiple_paths"
        )
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_free_dep(self):
        print_test(self.ci, "test_client_free_dep")
        self.ci.delete_all()

        # add a value clock, since we are adding date dependencies
        # Note: adding a future time dependency on a task, will cause it to requeue, when complete
        # Hence even when we free these dependency they get requeued.
        # So we use todays date.
        ltime = time.localtime()
        day = ltime.tm_mday
        month = ltime.tm_mon
        year = ltime.tm_year

        defs = Defs()
        suite = defs.add_suite("test_client_free_dep")
        suite.add_clock(Clock(False))  # true means hybrid, False means value
        ecfhome = Test.ecf_home(self.ci.get_port())
        suite.add_variable("ECF_HOME", ecfhome)
        protocol_option = "--http" if self.protocol == Test.Protocol.HTTP else ""
        suite.add_variable(
            "ECF_CLIENT_EXE_PATH", f"{File.find_client()} {protocol_option}"
        )
        suite.add_variable("SLEEPTIME", "1")
        suite.add_variable("ECF_INCLUDE", ecf_includes())
        family = suite.add_family("f1")
        family.add_task("t1").add_time("00:01")
        family.add_task("t2").add_date(day, month, year)
        family.add_task("t3").add_trigger("1 == 0")
        t4 = family.add_task("t4")
        t4.add_time("00:01")
        t4.add_date(day, month, year)
        t4.add_trigger("1 == 0")

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()

        t1_path = "/test_client_free_dep/f1/t1"
        t2_path = "/test_client_free_dep/f1/t2"
        t3_path = "/test_client_free_dep/f1/t3"
        t4_path = "/test_client_free_dep/f1/t4"
        while 1:
            self.ci.sync_local()
            t1 = self.ci.get_defs().find_abs_node(t1_path)
            t2 = self.ci.get_defs().find_abs_node(t2_path)
            t3 = self.ci.get_defs().find_abs_node(t3_path)
            t4 = self.ci.get_defs().find_abs_node(t4_path)

            if t1.get_state() == State.queued:
                self.ci.free_time_dep(t1_path)
            if t2.get_state() == State.queued:
                self.ci.free_date_dep(t2_path)
            if t3.get_state() == State.queued:
                self.ci.free_trigger_dep(t3_path)
            if t4.get_state() == State.queued:
                self.ci.free_all_dep(t4_path)

            suite = self.ci.get_defs().find_suite("test_client_free_dep")
            if suite.get_state() == State.complete:
                break
            time.sleep(3)

        dir_to_remove = Test.ecf_home(self.ci.get_port()) + "/" + "test_client_free_dep"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_suites(self):
        print_test(self.ci, "test_client_suites")
        self.ci.delete_all()
        assert len(self.ci.suites()) == 0, "expected 0 suite "

        defs = create_defs("test_client_suites", self.ci.get_port(), self.protocol)
        self.ci.load(defs)
        assert len(self.ci.suites()) == 1, "expected 1 suite "

        self.ci.delete_all()
        defs.add_suite("s2")
        self.ci.load(defs)
        assert len(self.ci.suites()) == 2, "expected 2 suite "

    def test_client_ch_with_drops_handles(self):
        print_test(self.ci, "test_client_ch_with_drops_handles")
        try:
            port = self.ci.get_port()
            print(
                "  test using with but without register, handle should be zero, ie do nothing"
            )
            with Client("localhost", port) as local_ci:
                local_ci.debug(True)
                if self.protocol == Test.Protocol.HTTP:
                    local_ci.enable_http()
                assert local_ci.ch_handle() == 0, "Expected handle to be zero"
                local_ci.ch_suites()
            print("  Test Client with register, should drop handle ")
            with Client("localhost", port) as local_ci:
                local_ci.debug(True)
                if self.protocol == Test.Protocol.HTTP:
                    local_ci.enable_http()
                local_ci.ch_register(True, ["s1", "s2"])
                # print("Handle:",local_ci .ch_handle())
                local_ci.ch_suites()
                # raise RuntimeError("xxx") # check exeption is still caught
        except RuntimeError as e:
            print("Exception", e)
        print("after with:")
        # valuely need a way to get hold of the suites, via python api.
        self.ci.ch_suites()  # should be empty

    def test_client_ch_suites(self):
        print_test(self.ci, "test_client_ch_suites")
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered
        self.ci.delete_all()

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 after load, but found " + str(
            len(list((self.ci.get_defs().suites)))
        )

        suite_names = ["s1", "s2", "s3"]
        self.ci.ch_register(
            True, suite_names
        )  # register interest in suites s1,s2,s3 and any new suites
        print("ch_handle after register : ", self.ci.ch_handle())
        self.ci.ch_suites()  # writes to standard out, list of suites and handles
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 3
        ), "Expected 3 registered suites but found " + str(
            len(list((self.ci.get_defs().suites)))
        )

        self.ci.ch_register(
            False, ["s1"]
        )  # register interest in suites s1. ci remembers the last client handle
        self.ci.ch_suites()  # writes to standard out, list of suites and handles
        sync_local(self.ci)
        self.ci.ch_suites()  # writes to standard out, list of suites and handles
        assert (
            len(list(self.ci.get_defs().suites)) == 1
        ), "Expected 1 registered suites but found " + str(
            len(list((self.ci.get_defs().suites)))
        )

    def test_client_ch_register(self):
        print_test(self.ci, "test_client_ch_register")
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered
        self.ci.delete_all()

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        suite_names = ["s1", "s2", "s3"]
        self.ci.ch_register(
            True, suite_names
        )  # register interest in suites s1,s2,s3 and any new suites
        self.ci.ch_register(
            False, suite_names
        )  # register interest in suites s1,s2,s3 only

        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 3
        ), "Expected 3 registered suites but found " + str(
            len(list((self.ci.get_defs().suites)))
        )

    def test_client_ch_drop(self):
        print_test(self.ci, "test_client_ch_drop")
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered
        self.ci.delete_all()

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        try:
            # register interest in suites s1,s2,s3 and any new suites
            suite_names = ["s1", "s2", "s3"]
            self.ci.ch_register(True, suite_names)
        finally:
            self.ci.ch_drop()  # drop using handle stored in ci., from last register

        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 suites but found " + str(len(list((self.ci.get_defs().suites))))

    def test_client_ch_drop_user(self):
        print_test(self.ci, "test_client_ch_drop_user")
        self.ci.delete_all()
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        try:
            # register interest in suites s1,s2,s3 and any new suites
            suite_names = ["s1", "s2", "s3"]
            self.ci.ch_register(True, suite_names)
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 3
            ), "Expected 3 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )
        except RuntimeError as e:
            print(str(e))

        self.ci.ch_drop_user("")  # drop all handle associated with current user
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 suites but found " + str(len(list((self.ci.get_defs().suites))))

    def test_client_ch_add(self):
        print_test(self.ci, "test_client_ch_add")
        self.ci.delete_all()
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        try:
            suite_names = []
            self.ci.ch_register(
                True, suite_names
            )  # register interest in any new suites
            suite_names = ["s1", "s2"]
            self.ci.ch_add(suite_names)  # add suites s1,s2 to the last added handle
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 2
            ), "Expected 2 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )

            suite_names = ["s3", "s4"]
            self.ci.ch_add(
                self.ci.ch_handle(), suite_names
            )  # add suites s3,s4 using last handle
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 4
            ), "Expected 4 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )

        except RuntimeError as e:
            print(str(e))

        self.ci.ch_drop_user("")  # drop all handle associated with current user
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 suites but found " + str(len(list((self.ci.get_defs().suites))))

    def test_client_ch_auto_add(self):
        print_test(self.ci, "test_client_ch_auto_add")
        self.ci.delete_all()
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        try:
            suite_names = ["s1", "s2", "s3"]
            self.ci.ch_register(
                True, suite_names
            )  # register interest in suites s1,s2,s3 and any new suites
            self.ci.ch_auto_add(
                False
            )  # disable adding newly created suites to last registered handle\n"
            self.ci.ch_auto_add(
                True
            )  # enable adding newly created suites to last registered handle\n"
            self.ci.ch_auto_add(
                self.ci.ch_handle(), False
            )  # disable adding newly created suites to handle\n"
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 3
            ), "Expected 3 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )
        except RuntimeError as e:
            print(str(e))

        self.ci.ch_drop_user("")  # drop all handle associated with current user
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 suites but found " + str(len(list((self.ci.get_defs().suites))))

    def test_client_ch_remove(self):
        print_test(self.ci, "test_client_ch_remove")
        self.ci.delete_all()
        try:
            self.ci.ch_drop_user("")  # drop all handle associated with current user
        except:
            pass  # Drop throws if no handle registered

        defs = Defs()
        for i in range(1, 7):
            defs.add_suite("s" + str(i))
        self.ci.load(defs)

        try:
            suite_names = ["s1", "s2", "s3"]
            self.ci.ch_register(
                True, suite_names
            )  # register interest in suites s1,s2,s3 and any new suites
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 3
            ), "Expected 3 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )

            suite_names = ["s1"]
            self.ci.ch_remove(
                suite_names
            )  # remove suites s1 from the last added handle\n"
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 2
            ), "Expected 2 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )

            suite_names = ["s2"]
            self.ci.ch_remove(
                self.ci.ch_handle(), suite_names
            )  # remove suites s2 from the last added handle\n"
            sync_local(self.ci)
            assert (
                len(list(self.ci.get_defs().suites)) == 1
            ), "Expected 1 suites but found " + str(
                len(list((self.ci.get_defs().suites)))
            )

        except RuntimeError as e:
            print(str(e))

        self.ci.ch_drop_user("")  # drop all handle associated with current user
        sync_local(self.ci)
        assert (
            len(list(self.ci.get_defs().suites)) == 6
        ), "Expected 6 suites but found " + str(len(list((self.ci.get_defs().suites))))

    def test_client_get_file(self):
        print_test(self.ci, "test_client_get_file")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_client_get_file", port, self.protocol)

        # Also test where user has specified his OWN ECF_JOBOUT (rare)
        t2 = defs.find_abs_node("/test_client_get_file/f1/t2")
        t2_jobout = Test.ecf_home(self.ci.get_port()) + "/test_client_get_file/t2.xx"
        t2.add_variable("ECF_JOBOUT", t2_jobout)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()

        while 1:
            if self.ci.news_local():
                self.ci.sync_local()  # get the changes, synced with local defs
                suite = self.ci.get_defs().find_suite("test_client_get_file")
                assert suite is not None, "Expected to find suite"
                if suite.get_state() == State.complete:
                    break
            time.sleep(1)

        try:
            for task in ["/test_client_get_file/f1/t1", "/test_client_get_file/f1/t2"]:

                result_string = self.ci.get_file(task)
                assert isinstance(
                    result_string, str
                ), f"Expected ci.get_file({task}) to return 'str'"
                assert (
                    len(result_string) > 0
                ), f"Expected ci.get_file({task}) to return something"

                result_string = self.ci.get_file(task, as_bytes=False)
                assert isinstance(
                    result_string, str
                ), f"Expected ci.get_file({task}, as_bytes=False) to return 'str'"
                assert (
                    len(result_string) > 0
                ), f"Expected ci.get_file({task}, as_bytes=False) to return something"

                result_bytes = self.ci.get_file(task, as_bytes=True)
                assert isinstance(
                    result_bytes, bytes
                ), f"Expected ci.get_file({task}, as_bytes=True) to return 'bytes'"
                assert (
                    len(result_bytes) > 0
                ), f"Expected ci.get_file({task}, as_bytes=True) to return something"

                assert len(result_string) == len(
                    result_bytes
                ), f"Expected 'str' result and 'bytes' result to have the same length"

                result_string = self.ci.get_file(task, max_lines="10")
                assert isinstance(
                    result_string, str
                ), f"Expected ci.get_file({task}, max_lines='10') to return 'str'"
                assert (
                    len(result_string) > 0
                ), f"Expected ci.get_file({task}, max_lines='10') to return something"

                result_string = self.ci.get_file(task, max_lines="10", as_bytes=False)
                assert isinstance(
                    result_string, str
                ), f"Expected ci.get_file({task}, max_lines='10', as_bytes=False) to return 'str'"
                assert (
                    len(result_string) > 0
                ), f"Expected ci.get_file({task}, max_lines='10', as_bytes=False) to return something"

                result_bytes = self.ci.get_file(task, max_lines="10", as_bytes=True)
                assert isinstance(
                    result_bytes, bytes
                ), f"Expected ci.get_file({task}, max_lines='10', as_bytes=True) to return 'bytes'"
                assert (
                    len(result_bytes) > 0
                ), f"Expected ci.get_file({task}, max_lines='10', as_bytes=True) to return something"

                assert len(result_string) == len(
                    result_bytes
                ), f"Expected 'str' result and 'bytes' result to have the same length"

                for file_type in ["script", "job", "jobout", "manual"]:
                    result_string = self.ci.get_file(task, file_type)
                    assert isinstance(
                        result_string, str
                    ), f"Expected ci.get_file({task}, {file_type}) to return 'str'"
                    assert (
                        len(result_string) > 0
                    ), f"Expected ci.get_file({task}, {file_type}) to return something"

                    result_string = self.ci.get_file(task, file_type, as_bytes=False)
                    assert isinstance(
                        result_string, str
                    ), f"Expected ci.get_file({task}, {file_type}, as_bytes=False) to return 'str'"
                    assert (
                        len(result_string) > 0
                    ), f"Expected ci.get_file({task}, {file_type}, as_bytes=False) to return something"

                    result_bytes = self.ci.get_file(task, file_type, as_bytes=True)
                    assert isinstance(
                        result_bytes, bytes
                    ), f"Expected ci.get_file({task}, {file_type}, as_bytes=True) to return 'bytes'"
                    assert (
                        len(result_bytes) > 0
                    ), f"Expected ci.get_file({task}, {file_type}, as_bytes=True) to return something"

                    assert len(result_string) == len(
                        result_bytes
                    ), f"Expected 'str' result and 'bytes' result to have the same length"

                    result_string = self.ci.get_file(task, file_type, "10")
                    assert isinstance(
                        result_string, str
                    ), f"Expected ci.get_file({task}, {file_type}, '10') to return 'str'"
                    assert (
                        len(result_string) > 0
                    ), f"Expected ci.get_file({task}, {file_type}, '10') to return something"

                    result_string = self.ci.get_file(
                        task, file_type, "10", as_bytes=False
                    )
                    assert isinstance(
                        result_string, str
                    ), f"Expected ci.get_file({task}, {file_type}, '10', as_bytes=False) to return 'str'"
                    assert (
                        len(result_string) > 0
                    ), f"Expected ci.get_file({task}, {file_type}, '10', as_bytes=False) to return something"

                    result_bytes = self.ci.get_file(
                        task, file_type, "10", as_bytes=True
                    )
                    assert isinstance(
                        result_bytes, bytes
                    ), f"Expected ci.get_file({task}, {file_type}, '10', as_bytes=True) to return 'bytes'"
                    assert (
                        len(result_bytes) > 0
                    ), f"Expected ci.get_file({task}, {file_type}, '10', as_bytes=True) to return something"

                    assert len(result_string) == len(
                        result_bytes
                    ), f"Expected 'str' result and 'bytes' result to have the same length"

            assert os.path.exists(
                t2_jobout
            ), f"User specified ECF_JOBOUT file not created {t2_jobout}"

        except RuntimeError as e:
            print(str(e))

        dir_to_remove = Test.ecf_home(port) + "/" + "test_client_get_file"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_alter_sort(self):
        print_test(self.ci, "test_client_alter_sort")
        self.ci.delete_all()

        defs = create_defs("test_client_alter_sort", self.ci.get_port(), self.protocol)
        t1 = "/test_client_alter_sort/f1/t1"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("z", "value").add_variable("y", "value").add_variable(
            "x", "value"
        )
        task_t1.add_event("z").add_event("y").add_event("x")
        task_t1.add_meter("z", 0, 100, 100).add_meter("y", 0, 100, 100).add_meter(
            "x", 0, 100, 100
        )
        task_t1.add_label("z", "name").add_label("y", "name").add_label("x", "name")
        task_t1.add_limit("z", 10).add_limit("y", 10).add_limit("x", 10)

        self.ci.load(defs)

        self.ci.sort_attributes(t1, "variable")
        self.ci.sort_attributes(t1, "event")
        self.ci.sort_attributes(t1, "meter")
        self.ci.sort_attributes(t1, "label")
        self.ci.sort_attributes(t1, "limit")

        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.variables)) == 3, "Expected 3 variable :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.events)) == 3, "Expected 3 events :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.meters)) == 3, "Expected 3 meters :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.labels)) == 3, "Expected 3 labels :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.limits)) == 3, "Expected 3 limits :\n" + str(
            self.ci.get_defs()
        )
        expected = ["x", "y", "z"]
        vactual = []
        eactual = []
        mactual = []
        lactual = []
        liactual = []
        for v in task_t1.variables:
            vactual.append(v.name())
        for v in task_t1.events:
            eactual.append(v.name())
        for v in task_t1.meters:
            mactual.append(v.name())
        for v in task_t1.labels:
            lactual.append(v.name())
        for v in task_t1.limits:
            liactual.append(v.name())
        assert expected == vactual, (
            "variable Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(vactual)
        )
        assert expected == eactual, (
            "event Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(eactual)
        )
        assert expected == mactual, (
            "meter Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(mactual)
        )
        assert expected == lactual, (
            "label Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(lactual)
        )
        assert expected == liactual, (
            "limit Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(liactual)
        )

    def test_client_alter_sort_defs(self):
        print_test(self.ci, "test_client_alter_sort_defs")
        self.ci.delete_all()

        defs = create_defs(
            "test_client_alter_sort_defs", self.ci.get_port(), self.protocol
        )
        t1 = "/test_client_alter_sort_defs/f1/t1"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("z", "value").add_variable("y", "value").add_variable(
            "x", "value"
        )
        task_t1.add_event("z").add_event("y").add_event("x")
        task_t1.add_meter("z", 0, 100, 100).add_meter("y", 0, 100, 100).add_meter(
            "x", 0, 100, 100
        )
        task_t1.add_label("z", "name").add_label("y", "name").add_label("x", "name")
        task_t1.add_limit("z", 10).add_limit("y", 10).add_limit("x", 10)

        self.ci.load(defs)

        self.ci.sort_attributes("/", "variable", True)
        self.ci.sort_attributes("/", "event", True)
        self.ci.sort_attributes("/", "meter", True)
        self.ci.sort_attributes("/", "label", True)
        self.ci.sort_attributes("/", "limit", True)

        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.variables)) == 3, "Expected 3 variable :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.events)) == 3, "Expected 3 events :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.meters)) == 3, "Expected 3 meters :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.labels)) == 3, "Expected 3 labels :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.limits)) == 3, "Expected 3 limits :\n" + str(
            self.ci.get_defs()
        )
        expected = ["x", "y", "z"]
        vactual = []
        eactual = []
        mactual = []
        lactual = []
        liactual = []
        for v in task_t1.variables:
            vactual.append(v.name())
        for v in task_t1.events:
            eactual.append(v.name())
        for v in task_t1.meters:
            mactual.append(v.name())
        for v in task_t1.labels:
            lactual.append(v.name())
        for v in task_t1.limits:
            liactual.append(v.name())
        assert expected == vactual, (
            "variable Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(vactual)
        )
        assert expected == eactual, (
            "event Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(eactual)
        )
        assert expected == mactual, (
            "meter Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(mactual)
        )
        assert expected == lactual, (
            "label Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(lactual)
        )
        assert expected == liactual, (
            "limit Attributes not sorted, expected:"
            + str(expected)
            + " but found:"
            + str(liactual)
        )

    def test_client_alter_add(self):
        print_test(self.ci, "test_client_alter_add")
        self.ci.delete_all()
        self.ci.load(
            create_defs("test_client_alter_add", self.ci.get_port(), self.protocol)
        )

        t1 = "/test_client_alter_add/f1/t1"
        self.ci.alter(t1, "add", "variable", "var", "var_name")
        self.ci.alter(t1, "add", "variable", "var2", "--")
        self.ci.alter(t1, "add", "variable", "var3", "--fred")
        self.ci.alter(t1, "add", "variable", "var4", " --fred ")
        self.ci.alter(t1, "add", "variable", "var5", "--fred --jake")
        self.ci.alter(t1, "add", "variable", "var6", " --fred --jake ")
        self.ci.alter(t1, "add", "variable", "var7", "")
        self.ci.alter(t1, "add", "time", "+00:30")
        self.ci.alter(t1, "add", "time", "01:30")
        self.ci.alter(t1, "add", "time", "01:30 20:00 00:30")
        self.ci.alter(t1, "add", "today", "+00:30")
        self.ci.alter(t1, "add", "today", "01:30")
        self.ci.alter(t1, "add", "today", "01:30 20:00 00:30")
        self.ci.alter(t1, "add", "date", "01.01.2001")
        self.ci.alter(t1, "add", "date", "*.01.2001")
        self.ci.alter(t1, "add", "date", "*.*.2001")
        self.ci.alter(t1, "add", "date", "*.*.*")
        self.ci.alter(t1, "add", "day", "sunday")
        self.ci.alter(t1, "add", "day", "monday")
        self.ci.alter(t1, "add", "day", "tuesday")
        self.ci.alter(t1, "add", "day", "wednesday")
        self.ci.alter(t1, "add", "day", "thursday")
        self.ci.alter(t1, "add", "day", "friday")
        self.ci.alter(t1, "add", "day", "saturday")
        self.ci.alter(t1, "add", "late", "late -s +00:15 -a 20:00 -c +02:00")
        self.ci.alter(t1, "add", "label", "label_name", "label_value")
        self.ci.alter(t1, "add", "label", "label_name2", "/a/label/with/path/values")

        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.variables)) == 7, "Expected 7 variable :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.times)) == 3, "Expected 3 time :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.todays)) == 3, "Expected 3 today's :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.dates)) == 4, "Expected 4 dates :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.days)) == 7, "Expected 7 days :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.labels)) == 2, "Expected 2 labels :\n" + str(
            self.ci.get_defs()
        )
        assert (
            str(task_t1.get_late()) == "late -s +00:15 -a 20:00 -c +02:00"
        ), "Expected late 'late -s +00:15 -a 20:00 -c +02:00'" + str(self.ci.get_defs())

    def test_client_alter_delete(self):
        print_test(self.ci, "test_client_alter_delete")
        self.ci.delete_all()
        defs = create_defs(
            "test_client_alter_delete", self.ci.get_port(), self.protocol
        )
        suite_with_limits = defs.add_suite("suite_with_limits")
        suite_with_limits.add_limit("limitX", 10)
        suite_with_limits_X = defs.add_suite("suite_with_limits_X")
        suite_with_limits_X.add_limit("limitX", 10)

        t1 = "/test_client_alter_delete/f1/t1"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("var", "value")
        task_t1.add_variable("var1", "value")
        task_t1.add_time("00:30")
        task_t1.add_time("00:31")
        task_t1.add_today("00:30")
        task_t1.add_today("00:31")
        task_t1.add_date(1, 1, 2001)
        task_t1.add_date(1, 1, 2002)
        task_t1.add_day("sunday")
        task_t1.add_day("monday")
        cron = Cron()
        cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
        cron.set_time_series("+00:30")
        task_t1.add_cron(cron)
        task_t1.add_cron(cron)
        task_t1.add_event("event")
        task_t1.add_event("event1")
        task_t1.add_meter("meter", 0, 100, 100)
        task_t1.add_meter("meter1", 0, 100, 100)
        task_t1.add_label("label", "name")
        task_t1.add_label("label1", "name")
        task_t1.add_limit("limit", 10)
        task_t1.add_limit("limit1", 10)
        task_t1.add_inlimit("limit", t1, 2)
        task_t1.add_inlimit("limit1", t1, 2)
        task_t1.add_inlimit("limitX", "/suite_with_limits", 2)
        task_t1.add_inlimit("limitX", "/suite_with_limits_X", 2)
        task_t1.add_trigger("t2 == active")
        task_t1.add_complete("t2 == complete")

        assert task_t1.get_late() is None, "expected no late"
        late = Late()
        late.submitted(20, 10)
        late.active(20, 10)
        late.complete(20, 10, True)
        task_t1.add_late(late)
        assert task_t1.get_late() is not None, "expected late"

        t2 = "/test_client_alter_delete/f1/t2"
        task_t2 = defs.find_abs_node(t2)
        task_t2.add_repeat(
            RepeatDate("date", 20100111, 20100115, 2)
        )  # can't add cron and repeat at the same level

        # print(defs)
        self.ci.load(defs)

        self.ci.alter(t1, "delete", "variable", "var")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.variables)) == 1, "Expected 1 variable :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "variable")  # delete all veriables
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.variables)) == 0, "Expected 0 variable :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "time", "00:30")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.times)) == 1, "Expected 1 time :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "time")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.times)) == 0, "Expected 0 time :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "today", "00:30")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.todays)) == 1, "Expected 1 today :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "today")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.todays)) == 0, "Expected 0 today :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "date", "01.01.2001")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.dates)) == 1, "Expected 1 date :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "date")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.dates)) == 0, "Expected 0 date :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "day", "sunday")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.days)) == 1, "Expected 1 day :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "day")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.days)) == 0, "Expected 0 day :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "event", "event")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.events)) == 1, "Expected 1 event :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "event")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.events)) == 0, "Expected 0 event :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "meter", "meter")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.meters)) == 1, "Expected 1 meter :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "meter")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.meters)) == 0, "Expected 0 meter :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "label", "label")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.labels)) == 1, "Expected 1 label :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "label")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.labels)) == 0, "Expected 0 label :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "inlimit", "limit")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.inlimits)) == 3, "Expected 3 inlimit :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "inlimit", "/suite_with_limits:limitX")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.inlimits)) == 2, "Expected 2 inlimit :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "inlimit", "/suite_with_limits_X:limitX")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.inlimits)) == 1, "Expected 1 inlimit :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "inlimit")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.inlimits)) == 0, "Expected 0 inlimit :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "limit", "limit")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.limits)) == 1, "Expected 1 limit :\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "limit")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.limits)) == 0, "Expected 0 limit :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "cron")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.crons)) == 0, "Expected 0 crons :\n" + str(
            self.ci.get_defs()
        )

        self.ci.alter(t1, "delete", "late")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert task_t1.get_late() is None, "expected no late after delete"

        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert task_t1.get_trigger() is not None, "Expected trigger:\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "trigger")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert task_t1.get_trigger() is None, "Expected trigger to be deleted:\n" + str(
            self.ci.get_defs()
        )

        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert task_t1.get_complete() is not None, "Expected complete:\n" + str(
            self.ci.get_defs()
        )
        self.ci.alter(t1, "delete", "complete")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert (
            task_t1.get_complete() is None
        ), "Expected complete to be deleted:\n" + str(self.ci.get_defs())

        self.ci.alter(t2, "delete", "repeat")
        sync_local(self.ci)
        task_t2 = self.ci.get_defs().find_abs_node(t2)
        repeat = task_t2.get_repeat()
        assert repeat.empty(), "Expected repeat to be deleted:\n" + str(
            self.ci.get_defs()
        )

    def test_client_alter_change(self):
        print_test(self.ci, "test_client_alter_change")

        #
        # ISSUE:
        # - Currently we can only change clock attr if we have one.
        # - Even when we have a clock attr, it only makes sense to apply clock attr changes
        #   before begin(), i.e how do we apply change in gain after begin ??
        #
        # " change clock-type name        # The name must be one of 'hybrid' or 'value'.\n"
        # " change clock-gain name        # The gain must be convertible to an integer.\n"
        # " change label name value       # sets the label\n"
        # " change repeat value           # If the repeat is a date, then the value must be a valid YMD (ie. yyyymmdd)\n"
        # "                               # and be convertible to an integer, additionally the value must be in range\n"
        # "                               # of the repeat start and end dates. Likewise for repeat integer. For repeat\n"
        # "                               # string and enum, the name must either be an integer, that is a valid index, or\n"
        # "                               # if it is a string, it must correspond to one of enum's or strings list\n"
        #

        self.ci.delete_all()
        defs = create_defs(
            "test_client_alter_change", self.ci.get_port(), self.protocol
        )
        t1 = "/test_client_alter_change/f1/t1"
        repeat_date_path = "/test_client_alter_change/f1/repeat_date"
        repeat_datetime_path = "/test_client_alter_change/f1/repeat_datetime"

        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("var", "value")
        task_t1.add_variable("var1", "value")
        task_t1.add_event("event")
        task_t1.add_event("event1")
        task_t1.add_meter("meter", 0, 100, 100)
        task_t1.add_meter("meter1", 0, 100, 100)
        task_t1.add_label("label", "name")
        task_t1.add_label("label1", "name1")
        task_t1.add_limit("limit", 10)
        task_t1.add_limit("limit1", 10)
        task_t1.add_inlimit("limit", t1, 2)
        task_t1.add_inlimit("limit1", t1, 2)
        task_t1.add_trigger("t2 == active")
        task_t1.add_complete("t2 == complete")
        task_t1.add_time("00:31")
        task_t1.add_today("00:30")

        late = Late()
        late.submitted(20, 10)
        late.active(20, 10)
        late.complete(20, 10, True)
        task_t1.add_late(late)

        f1 = defs.find_abs_node("/test_client_alter_change/f1")
        # Add a new Repeat (based on date)
        repeat_date = f1.add_task("repeat_date")
        repeat_date.add_repeat(
            RepeatDate("date", 20100111, 20100115, 2)
        )  # can't add cron and repeat at the same level
        # Add a new Repeat (based on date+time)
        repeat_datetime = f1.add_task("repeat_datetime")
        repeat_datetime.add_repeat(
            RepeatDateTime("datetime", "20100111T000000", "20100115T000000", "48:00:00")
        )

        self.ci.load(defs)

        self.ci.alter(t1, "change", "time", "00:31", "10:10")
        self.ci.alter(t1, "change", "today", "00:30", "10:10")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        assert len(list(task_t1.times)) == 1, "Expected 1 time :\n" + str(
            self.ci.get_defs()
        )
        assert len(list(task_t1.todays)) == 1, "Expected 1 time :\n" + str(
            self.ci.get_defs()
        )
        for t in task_t1.times:
            assert str(t) == "time 10:10", " expected 'time 10:10' but found:\n" + str(
                t
            )
        for t in task_t1.todays:
            assert (
                str(t) == "today 10:10"
            ), " expected 'today 10:10' but found:\n" + str(t)

        self.ci.alter(t1, "change", "late", "-s +10:00")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        variable = task_t1.get_late()
        assert (
            str(task_t1.get_late()) == "late -s +10:00"
        ), "Expected alter of late to be 'late -s +10:00' but found " + str(
            task_t1.get_late()
        )

        self.ci.alter(t1, "change", "variable", "var", "changed_var")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        variable = task_t1.find_variable("var")
        assert variable.value() == "changed_var", (
            "Expected alter of variable to be 'change_var' but found "
            + variable.value()
        )
        res = self.ci.query("variable", task_t1.get_abs_node_path(), "var")
        assert res == "changed_var", (
            "Expected alter of variable to be 'change_var' but found " + res
        )

        self.ci.alter(t1, "change", "variable", "var", "--")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        variable = task_t1.find_variable("var")
        assert variable.value() == "--", (
            "Expected alter of variable to be '--' but found " + variable.value()
        )
        res = self.ci.query("variable", task_t1.get_abs_node_path(), "var")
        assert res == "--", "Expected alter of variable to be '--' but found " + res

        self.ci.alter(t1, "change", "variable", "var", "--fred")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        variable = task_t1.find_variable("var")
        assert variable.value() == "--fred", (
            "Expected alter of variable to be '--fred' but found " + variable.value()
        )

        self.ci.alter(t1, "change", "variable", "var", "--fred --bill")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        variable = task_t1.find_variable("var")
        assert variable.value() == "--fred --bill", (
            "Expected alter of variable to be '--fred --bill' but found "
            + variable.value()
        )

        self.ci.alter(t1, "change", "meter", "meter", "10")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        meter = task_t1.find_meter("meter")
        assert meter.value() == 10, "Expected alter of meter to be 10 but found " + str(
            meter.value()
        )
        res = self.ci.query("meter", task_t1.get_abs_node_path(), "meter")
        assert res == "10", "Expected alter of meter to be 10 but found " + res

        self.ci.alter(t1, "change", "event", "event", "set")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        event = task_t1.find_event("event")
        assert (
            event.value() == True
        ), "Expected alter of event to be set but found " + str(event.value())
        res = self.ci.query("event", task_t1.get_abs_node_path(), "event")
        assert res == "set", "Expected alter of event to be 'set' but found " + res

        self.ci.alter(t1, "change", "trigger", "t2 == aborted")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        trigger = task_t1.get_trigger()
        assert trigger.get_expression() == "t2 == aborted", (
            "Expected alter of trigger to be 't2 == aborted' but found "
            + trigger.get_expression()
        )
        res = self.ci.query("trigger", task_t1.get_abs_node_path(), "t2 == aborted")
        assert res == "false", (
            "Expected evaluation of trigger to fail, but found: " + res
        )

        self.ci.alter(
            t1, "change", "trigger", "/test_client_alter_change/f1/t2 == complete"
        )
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        trigger = task_t1.get_trigger()
        assert (
            trigger.get_expression() == "/test_client_alter_change/f1/t2 == complete"
        ), (
            "Expected alter of trigger to be '/test_client_alter_change/f1/t2 == complete' but found "
            + trigger.get_expression()
        )

        self.ci.alter(t1, "change", "complete", "t2 == aborted")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        complete = task_t1.get_complete()
        assert complete.get_expression() == "t2 == aborted", (
            "Expected alter of complete to be 't2 == aborted' but found "
            + complete.get_expression()
        )

        self.ci.alter(
            t1, "change", "complete", "/test_client_alter_change/f1/t2 == active"
        )
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        complete = task_t1.get_complete()
        assert (
            complete.get_expression() == "/test_client_alter_change/f1/t2 == active"
        ), (
            "Expected alter of complete to be '/test_client_alter_change/f1/t2 == active' but found "
            + complete.get_expression()
        )

        self.ci.alter(t1, "change", "limit_max", "limit", "2")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        limit = task_t1.find_limit("limit")
        assert limit is not None, "Expected to find limit"
        assert (
            limit.limit() == 2
        ), "Expected alter of limit_max to be 2 but found " + str(limit.limit())

        self.ci.alter(t1, "change", "limit_value", "limit", "2")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        limit = task_t1.find_limit("limit")
        assert limit is not None, "Expected to find limit"
        assert (
            limit.value() == 2
        ), "Expected alter of limit_value to be 2 but found " + str(limit.value())

        self.ci.alter(t1, "change", "label", "label", "new-value")
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        label = task_t1.find_label("label")
        assert label.new_value() == "new-value", (
            "Expected alter of label to be 'new-value' but found " + label.new_value()
        )

        self.ci.alter(repeat_date_path, "change", "repeat", "20100113")
        sync_local(self.ci)
        task = self.ci.get_defs().find_abs_node(repeat_date_path)
        repeat = task.get_repeat()
        assert (
            repeat.value() == 20100113
        ), "Expected alter of repeat to be 20100113 but found " + str(repeat.value())
        res = self.ci.query("variable", task.get_abs_node_path(), repeat.name())
        assert res == "20100113", (
            "Expected alter of repeat to be 20100113 but found " + res
        )

        self.ci.alter(repeat_datetime_path, "change", "repeat", "20100113T000000")
        sync_local(self.ci)
        task = self.ci.get_defs().find_abs_node(repeat_datetime_path)
        repeat = task.get_repeat()
        assert repeat.value() == 1263340800, (
            "Expected alter of repeat to be 1263340800 (i.e. seconds between 19700101T000000 and 20100113T000000) but found "
            + str(repeat.value())
        )
        res = self.ci.query("variable", task.get_abs_node_path(), repeat.name())
        assert res == "20100113T000000", (
            "Expected alter of repeat to be 20100113T000000 but found " + res
        )

    def test_client_alter_flag(self):
        print_test(self.ci, "test_client_alter_flag")
        self.ci.delete_all()
        defs = create_defs("test_client_alter_flag", self.ci.get_port(), self.protocol)
        t1 = "/test_client_alter_flag/f1/t1"

        task_t1 = defs.find_abs_node(t1)

        self.ci.load(defs)

        flag = Flag()
        flag_list = flag.list()  # flag_list is of type FlagTypeVec
        for flg in flag_list:
            self.ci.alter(t1, "set_flag", flag.type_to_string(flg))
            sync_local(self.ci)
            task_t1 = self.ci.get_defs().find_abs_node(t1)
            task_flag = task_t1.get_flag()
            assert task_flag.is_set(
                flg
            ), "expected flag %r to be set" % task_flag.type_to_string(flg)

            # alter itself causes the flag message to be set, and preserved
            if flg == FlagType.message:
                continue

            self.ci.alter(t1, "clear_flag", flag.type_to_string(flg))
            sync_local(self.ci)
            task_t1 = self.ci.get_defs().find_abs_node(t1)
            task_flag = task_t1.get_flag()
            assert not task_flag.is_set(
                flg
            ), "expected flag %r NOT to be set" % task_flag.type_to_string(flg)

    def test_client_force(self):
        print_test(self.ci, "test_client_force")
        self.ci.delete_all()
        defs = create_defs("test_client_force", self.ci.get_port(), self.protocol)

        path_list = ["/test_client_force/f1/t1", "/test_client_force/f1/t2"]
        t1 = path_list[0]
        for path in path_list:
            task = defs.find_abs_node(path)
            assert task is not None, "Expected to find task at path " + path
            task.add_event("event")

        self.ci.load(defs)

        state_list = [
            State.unknown,
            State.active,
            State.complete,
            State.queued,
            State.submitted,
            State.aborted,
        ]
        for state in state_list:
            self.ci.force_state(t1, state)
            sync_local(self.ci)
            task = self.ci.get_defs().find_abs_node(t1)
            assert task.get_state() == state, (
                "Expected state " + state + " but found " + str(task.get_state())
            )
        for state in state_list:
            self.ci.force_state(path_list, state)
            sync_local(self.ci)
            for path in path_list:
                task = self.ci.get_defs().find_abs_node(path)
                assert task.get_state() == state, (
                    "Expected state " + state + " but found " + str(task.get_state())
                )

        for state in state_list:
            self.ci.force_state_recursive("/test_client_force", state)
            sync_local(self.ci)
            task = self.ci.get_defs().find_abs_node(t1)
            assert task.get_state() == state, (
                "Expected state " + state + " but found " + str(task.get_state())
            )
        suite_paths = ["/test_client_force"]
        for state in state_list:
            self.ci.force_state_recursive(suite_paths, state)
            sync_local(self.ci)
            task = self.ci.get_defs().find_abs_node(t1)
            assert task.get_state() == state, (
                "Expected state " + state + " but found " + str(task.get_state())
            )

        event_states = ["set", "clear"]
        for ev_state in event_states:
            for path in path_list:
                self.ci.force_event(path + ":event", ev_state)
                sync_local(self.ci)
                task = self.ci.get_defs().find_abs_node(path)
                event_fnd = False
                for event in task.events:
                    event_fnd = True
                    if ev_state == "set":
                        assert event.value() == True, " Expected event value to be set"
                    else:
                        assert (
                            event.value() == False
                        ), " Expected event value to be clear"
                assert event_fnd == True, " Expected event to be found"

        event_path_list = [
            "/test_client_force/f1/t1:event",
            "/test_client_force/f1/t2:event",
        ]
        event_states = ["set", "clear"]
        for ev_state in event_states:
            self.ci.force_event(event_path_list, ev_state)
            sync_local(self.ci)
            for path in path_list:
                task = self.ci.get_defs().find_abs_node(path)
                event_fnd = False
                for event in task.events:
                    event_fnd = True
                    if ev_state == "set":
                        assert event.value() == True, " Expected event value to be set"
                    else:
                        assert (
                            event.value() == False
                        ), " Expected event value to be clear"
                assert event_fnd == True, " Expected event to be found"

    @pytest.mark.parametrize("on_disk", [False, True], ids=["in_memory", "on_disk"])
    def test_client_replace(self, on_disk):
        print_test(self.ci, "test_client_replace client_defs on disk = " + str(on_disk))
        # Create and load the following defs
        # s1
        #   f1
        #     t1
        #     t2
        self.ci.delete_all()
        self.ci.load(create_defs("s1", self.ci.get_port(), self.protocol))

        # ===============================================================================
        # Example of using replace to ADD a *NEW* node hierarchy to an existing suite
        # we should end up with:
        # s1
        #   f1
        #     t1
        #     t2
        #   f2
        #     t1
        #     t2
        test_client_replace_def_file = (
            "test_client_replace_" + str(os.getpid()) + ".def"
        )
        client_def = create_defs("s1", self.ci.get_port(), self.protocol)
        client_def.find_suite("s1").add_family("f2").add_task("t1")
        if on_disk:
            client_def.save_as_defs(test_client_replace_def_file)
            client_def = test_client_replace_def_file

        self.ci.replace(
            "/s1/f2", client_def, True, False
        )  # True means create parents as needed, False means don't bypass checks/zombies
        self.ci.get_server_defs()
        assert len(list(self.ci.get_defs().suites)) == 1, "Expected 1 suites:\n" + str(
            self.ci.get_defs()
        )
        assert (
            self.ci.get_defs().find_abs_node("/s1/f2/t1") is not None
        ), "Expected to find task /s1/f2/t1\n" + str(self.ci.get_defs())

        # Example of using replace to *REMOVE* node hierarchy to an existing suite, could have used delete
        assert (
            self.ci.get_defs().find_abs_node("/s1/f1/t1") is not None
        ), "Expected to find task /s1/f1/t1\n" + str(self.ci.get_defs())
        assert (
            self.ci.get_defs().find_abs_node("/s1/f2/t1") is not None
        ), "Expected to find task /s1/f2/t1\n" + str(self.ci.get_defs())
        client_def = Defs()
        client_def.add_suite("s1")  # should only have the suite
        if on_disk:
            client_def.save_as_defs(test_client_replace_def_file)
            client_def = test_client_replace_def_file

        self.ci.replace("/s1", client_def)
        self.ci.get_server_defs()
        assert len(list(self.ci.get_defs().suites)) == 1, "Expected 1 suites:\n" + str(
            self.ci.get_defs()
        )
        assert (
            self.ci.get_defs().find_abs_node("/s1/f1/t1") is None
        ), "Expected NOT to find task /s1/f1/t1\n" + str(self.ci.get_defs())
        assert (
            self.ci.get_defs().find_abs_node("/s1/f2/t1") is None
        ), "Expected NOT to find task /s1/f2/t1\n" + str(self.ci.get_defs())

        # ===============================================================================
        # Example of using replace to add a *NEW* suite
        client_def = Defs()
        client_def.add_suite("s2")
        if on_disk:
            client_def.save_as_defs(test_client_replace_def_file)
            client_def = test_client_replace_def_file

        self.ci.replace(
            "/s2", client_def, True, False
        )  # True means create parents as needed, False means don't bypass checks/zombies
        self.ci.get_server_defs()
        assert (
            len(list(self.ci.get_defs().suites)) == 2
        ), " Expected two suites:\n" + str(self.ci.get_defs())

        # replace added suite s2 with a new s2 which has a task,
        # s2 must exist on the client defs
        client_def = Defs()
        client_def.add_suite("s2").add_task("t1")
        if on_disk:
            client_def.save_as_defs(test_client_replace_def_file)
            client_def = test_client_replace_def_file

        self.ci.replace("/s2", client_def)

        self.ci.get_server_defs()
        assert (
            len(list(self.ci.get_defs().suites)) == 2
        ), " Expected two suites:\n" + str(self.ci.get_defs())
        assert (
            self.ci.get_defs().find_abs_node("/s2/t1") is not None
        ), "Expected to find task /s2/t1\n" + str(self.ci.get_defs())
        if on_disk:
            os.remove(test_client_replace_def_file)

    def test_node_replace(self):
        print_test(self.ci, "test_node_replace")
        PrintStyle.set_style(Style.MIGRATE)  # show node state
        self.ci.delete_all()
        defs = Defs() + (Suite("s1") + Family("f1").add(Task("t1"), Task("t2")))
        self.ci.load(defs)

        # We should have 4 nodes
        sync_local(self.ci)
        ci_defs = self.ci.get_defs()
        node_vec = ci_defs.get_all_nodes()
        assert len(list(node_vec)) == 4, "Expected two 4 nodes: \n" + str(
            self.ci.get_defs()
        )

        # replace each node, add variable first, then check, it was added
        for node in node_vec:
            node += Edit(var="XX", var2="xx")
            node.replace_on_server(
                self.ci
            )  # default is to suspend server node first, so replaced node is also suspended

            sync_local(self.ci)
            replace_node = self.ci.get_defs().find_abs_node(node.get_abs_node_path())
            assert len(list(replace_node.variables)) == 2, (
                "Expected two 2 variable: \n"
                + str(replace_node)
                + "\n"
                + str(self.ci.get_defs())
            )
            assert replace_node.get_dstate() == DState.suspended, (
                "Expected node to be suspended:\n"
                + str(replace_node)
                + "\n"
                + str(self.ci.get_defs())
            )

        # resume nodes, test that when False passed in we do not suspend the replaced node
        for node in node_vec:
            self.ci.resume(node.get_abs_node_path())
            sync_local(self.ci)
            replace_node = self.ci.get_defs().find_abs_node(node.get_abs_node_path())
            assert (
                replace_node.get_dstate() != DState.suspended
            ), "Expected node not to suspended:\n" + str(replace_node)

            replace_node += Meter("meter", 0, 100)

            # cant use node i.e this is only client side, and is still suspended
            # When we replace is server the client defs is sent to server. hence node would be suspended
            replace_node.replace_on_server(self.ci, suspend_node_first=False)
            sync_local(self.ci)

            replace_node = self.ci.get_defs().find_abs_node(node.get_abs_node_path())
            assert len(list(replace_node.meters)) == 1, "Expected 1 meter: \n" + str(
                replace_node
            )
            assert (
                len(list(replace_node.variables)) == 2
            ), "Expected two 2 variable: \n" + str(replace_node)
            assert (
                replace_node.get_dstate() != DState.suspended
            ), "Expected node not to suspended:\n" + str(replace_node)

        # replace the suite
        defs = Defs() + Suite("s1")
        host_port = self.ci.get_host() + ":" + self.ci.get_port()
        defs.s1.replace_on_server(self.ci)

        self.ci.get_server_defs()
        assert len(list(self.ci.get_defs().s1)) == 0, "Expected 0 family: \n" + str(
            self.ci.get_defs()
        )
        assert (
            self.ci.get_defs().s1.get_dstate() == DState.suspended
        ), "Expected node to be suspended:\n" + str(self.ci.get_defs())

    def test_client_suspend(self):
        print_test(self.ci, "test_client_suspend")
        self.ci.delete_all()
        defs = create_defs("test_client_suspend", self.ci.get_port(), self.protocol)
        suite = defs.find_suite("test_client_suspend")
        suite.add_variable("ECF_DUMMY_TASK", "")

        self.ci.load(defs)
        self.ci.begin_all_suites()

        self.ci.suspend("/test_client_suspend")

        sync_local(self.ci)
        suite = self.ci.get_defs().find_suite("test_client_suspend")
        assert suite.is_suspended(), "Expected to find suite suspended"

        assert self.ci.query("dstate", suite.get_abs_node_path()) == "suspended", (
            "Expected to find suite dstate suspended but found: " + res
        )
        assert self.ci.query("state", suite.get_abs_node_path()) == "queued", (
            "Expected to find suite state queued but found: " + res
        )

    def test_client_suspend_multiple_paths(self):
        print_test(self.ci, "test_client_suspend_multiple_paths")
        self.ci.delete_all()
        defs = create_defs(
            "test_client_suspend_multiple_paths", self.ci.get_port(), self.protocol
        )
        suite = defs.find_suite("test_client_suspend_multiple_paths")
        suite.add_variable("ECF_DUMMY_TASK", "")

        self.ci.load(defs)
        self.ci.begin_all_suites()

        path_list = [
            "/test_client_suspend_multiple_paths/f1/t1",
            "/test_client_suspend_multiple_paths/f1/t2",
        ]
        self.ci.suspend(path_list)

        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(
            "/test_client_suspend_multiple_paths/f1/t1"
        )
        task_t2 = self.ci.get_defs().find_abs_node(
            "/test_client_suspend_multiple_paths/f1/t2"
        )
        assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
        assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"

    def test_client_resume(self):
        print_test(self.ci, "test_client_resume")
        self.ci.delete_all()
        defs = create_defs("test_client_resume", self.ci.get_port(), self.protocol)
        suite = defs.find_suite("test_client_resume")
        suite.add_variable("ECF_DUMMY_TASK", "")

        self.ci.load(defs)
        self.ci.begin_all_suites()

        self.ci.suspend("/test_client_resume")
        sync_local(self.ci)
        suite = self.ci.get_defs().find_suite("test_client_resume")
        assert suite.is_suspended(), "Expected to find suite suspended"

        self.ci.resume("/test_client_resume")
        sync_local(self.ci)
        suite = self.ci.get_defs().find_suite("test_client_resume")
        assert suite.is_suspended() == False, "Expected to find suite resumed"

    def test_client_resume_multiple_paths(self):
        print_test(self.ci, "test_client_resume_multiple_paths")
        self.ci.delete_all()
        defs = create_defs(
            "test_client_resume_multiple_paths", self.ci.get_port(), self.protocol
        )
        suite = defs.find_suite("test_client_resume_multiple_paths")
        suite.add_variable("ECF_DUMMY_TASK", "")

        self.ci.load(defs)
        self.ci.begin_all_suites()

        path_list = [
            "/test_client_resume_multiple_paths/f1/t1",
            "/test_client_resume_multiple_paths/f1/t2",
        ]
        self.ci.suspend(path_list)

        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(
            "/test_client_resume_multiple_paths/f1/t1"
        )
        task_t2 = self.ci.get_defs().find_abs_node(
            "/test_client_resume_multiple_paths/f1/t2"
        )
        assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
        assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"

        self.ci.resume(path_list)
        sync_local(self.ci)
        task_t1 = self.ci.get_defs().find_abs_node(
            "/test_client_resume_multiple_paths/f1/t1"
        )
        task_t2 = self.ci.get_defs().find_abs_node(
            "/test_client_resume_multiple_paths/f1/t2"
        )
        assert task_t1.is_suspended() == False, "Expected to find task t1 to be resumed"
        assert task_t2.is_suspended() == False, "Expected to find task t2 to be resumed"

    def test_client_delete_node(self):
        print_test(self.ci, "test_client_delete_node")
        self.ci.delete_all()
        defs = create_defs("test_client_delete_node", self.ci.get_port(), self.protocol)

        task_vec = defs.get_all_tasks()
        assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)

        self.ci.load(defs)
        sync_local(self.ci)
        for task in task_vec:
            node = self.ci.get_defs().find_abs_node(task.get_abs_node_path())
            assert node is not None, (
                "Expected to find task "
                + task.get_abs_node_path()
                + ":\n"
                + str(self.ci.get_defs())
            )

        for task in task_vec:
            self.ci.delete(task.get_abs_node_path())

        sync_local(self.ci)
        for task in task_vec:
            node = self.ci.get_defs().find_abs_node(task.get_abs_node_path())
            assert node is None, (
                "Expected not to find task "
                + task.get_abs_node_path()
                + " as it should have been deleted:\n"
                + str(self.ci.get_defs())
            )

    def test_client_delete_node_multiple_paths(self):
        print_test(self.ci, "test_client_delete_node_multiple_paths")
        self.ci.delete_all()
        defs = create_defs(
            "test_client_delete_node_multiple_paths", self.ci.get_port(), self.protocol
        )

        task_vec = defs.get_all_tasks()
        assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)

        paths = []
        for task in task_vec:
            paths.append(task.get_abs_node_path())

        self.ci.load(defs)

        sync_local(self.ci)
        for task in task_vec:
            node = self.ci.get_defs().find_abs_node(task.get_abs_node_path())
            assert node is not None, (
                "Expected to find task "
                + task.get_abs_node_path()
                + ":\n"
                + str(self.ci.get_defs())
            )

        self.ci.delete(paths)

        sync_local(self.ci)
        for task in task_vec:
            node = self.ci.get_defs().find_abs_node(task.get_abs_node_path())
            assert node is None, (
                "Expected not to find task "
                + task.get_abs_node_path()
                + " as it should have been deleted:\n"
                + str(self.ci.get_defs())
            )

    def test_client_archive_and_restore(self):
        suite_name = "test_client_archive_and_restore"
        print_test(self.ci, suite_name)
        self.ci.delete_all()
        defs = create_defs(suite_name, self.ci.get_port(), self.protocol)
        suite = defs.find_suite(suite_name)
        suite_path = suite.get_abs_node_path()

        self.ci.restart_server()
        self.ci.load(defs)

        sync_local(self.ci)  # get the changes, synced with local defs
        node_vec = self.ci.get_defs().get_all_nodes()
        assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))

        self.ci.archive(suite_path)
        sync_local(self.ci)  # get the changes, synced with local defs
        node_vec = self.ci.get_defs().get_all_nodes()
        assert len(node_vec) == 1, "Expected 1 nodes, but found " + str(len(node_vec))
        the_suite = self.ci.get_defs().find_suite(suite_name)
        assert the_suite is not None, "Expected to find suite"
        assert the_suite.get_flag().is_set(
            FlagType.archived
        ), " expected archive flag to be set"

        self.ci.restore(suite_path)
        sync_local(self.ci)  # get the changes, synced with local defs
        node_vec = self.ci.get_defs().get_all_nodes()
        assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))
        the_restored_suite = self.ci.get_defs().find_suite(suite_name)
        assert the_restored_suite is not None, "Expected to find suite"
        assert the_restored_suite.get_flag().is_set(
            FlagType.restored
        ), " expected restored flag to be set"
        assert not the_restored_suite.get_flag().is_set(
            FlagType.archived
        ), "expected archive flag to be cleared"

    def test_client_check(self):
        print_test(self.ci, "test_client_check")
        self.ci.delete_all()

        defs = Defs()
        defs.add_extern("/a/b/c/d")
        defs.add_extern("/a/b/c/d/e:event")
        defs.add_extern("/a/b/c/d/e:meter")
        defs.add_extern("/made/up/redundant/extren")
        defs.add_extern("/made/up/redundant/extren")
        defs.add_extern("/limits:c1a")
        defs.add_extern("/limits:c1a")
        defs.add_extern("fred")
        defs.add_extern("limits:hpcd")
        defs.add_extern("/suiteName:sg1")
        defs.add_extern("/obs/limits:hpcd")
        suite = defs.add_suite("extern")
        family_f1 = suite.add_family("f1")
        family_f1.add_task("p").add_trigger("/a/b/c/d == complete")  # extern path
        family_f1.add_task("q").add_trigger(
            "/a/b/c/d/e:event == set"
        )  # extern event path
        family_f1.add_task("r").add_trigger(
            "/a/b/c/d/e:meter le 30"
        )  # extern meter path

        suite.add_inlimit("c1a", "/limits")
        suite.add_inlimit("fred")

        family_anon = suite.add_family("anon")
        family_anon.add_inlimit("hpcd", "limits")
        family_anon.add_task("t1").add_inlimit("sg1", "/suiteName")
        family_anon.add_task("t2").add_inlimit("hpcd", "/obs/limits")
        family_anon.add_task("t3").add_inlimit("c1a", "/limits")

        # CLIENT side check
        client_check = defs.check()
        assert len(client_check) == 0, (
            "Expected clean defs check due to externs but found:\n"
            + client_check
            + "\n"
            + str(defs)
        )

        # SERVER side check
        self.ci.load(defs)
        server_check = self.ci.check(
            ""
        )  # empty string means check the whole defs, otherwise a node path can be specified.
        # print(server_check)
        assert (
            len(server_check) > 0
        ), "Expected defs to fail, since no externs in server "

    def test_client_check_defstatus(self):
        print_test(self.ci, "test_client_check_defstatus")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_client_check_defstatus", port, self.protocol)

        # stop defs form running when begin is called.
        suite = defs.find_suite("test_client_check_defstatus")
        suite.add_defstatus(DState.suspended)

        t1 = "/test_client_check_defstatus/f1/t1"
        t2 = "/test_client_check_defstatus/f1/t2"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_defstatus(DState.suspended)

        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)
        self.ci.begin_all_suites()

        sync_local(self.ci)  # get the changes, synced with local defs
        # print(ci.get_defs())
        task_t1 = self.ci.get_defs().find_abs_node(t1)
        task_t2 = self.ci.get_defs().find_abs_node(t2)
        assert task_t1 is not None, "Could not find t1"
        assert task_t2 is not None, "Could not find t2"

        assert (
            task_t1.get_state() == State.queued
        ), "Expected state queued but found " + str(task_t1.get_state())
        assert task_t2.get_state() == State.queued, "Expected state queued " + str(
            task_t2.get_state()
        )

        assert (
            task_t1.get_dstate() == DState.suspended
        ), "Expected state suspended but found " + str(task_t1.get_state())
        assert (
            task_t2.get_dstate() == DState.queued
        ), "Expected state queued but found " + str(task_t2.get_state())

        dir_to_remove = Test.ecf_home(port) + "/" + "test_client_check_defstatus"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_client_stats(self):
        print_test(self.ci, "test_client_stats")
        out = CustomStdOut()
        with out:
            stats = self.ci.stats()
            assert "Statistics" in stats, "Expected 'Statistics' in the response"
        assert (
            "Statistics" in out.value()
        ), "Expected 'Statistics' in the captured output"

    def test_client_stats_with_stdout(self):
        print_test(self.ci, "test_client_stats_with_stdout")
        out = CustomStdOut()
        with out:
            stats = self.ci.stats(True)
            assert "Statistics" in stats, "Expected 'Statistics' in the response"
        assert (
            "Statistics" in out.value()
        ), "Expected 'Statistics' in the captured output"

    def test_client_stats_without_stdout(self):
        print_test(self.ci, "test_client_stats_without_stdout")
        out = CustomStdOut()
        with out:
            stats = self.ci.stats(False)
            assert "Statistics" in stats, "Expected 'Statistics' in the response"
        assert not out.value(), "No captured output expected, but found: " + out.value()

    def test_client_stats_reset(self):
        print_test(self.ci, "test_client_stats_reset")
        self.ci.stats_reset()
        stats = self.ci.stats()
        assert "Statistics" in stats, "Expected 'Statistics' in the response"

    def test_client_debug_server_on_off(self):
        print_test(self.ci, "test_client_debug_server_on_off")
        self.ci.debug_server_on()  # writes to standard out
        self.ci.debug_server_off()

    def test_ECFLOW_189(self):
        print_test(self.ci, "test_ECFLOW_189")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_ECFLOW_189", port, self.protocol)
        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)

        self.ci.suspend("/test_ECFLOW_189")
        self.ci.suspend("/test_ECFLOW_189/f1/t1")
        self.ci.suspend("/test_ECFLOW_189/f1/t2")

        self.ci.begin_all_suites()

        sync_local(self.ci)  # get the changes, synced with local defs
        # print(ci.get_defs())
        task_t1 = self.ci.get_defs().find_abs_node("/test_ECFLOW_189/f1/t1")
        task_t2 = self.ci.get_defs().find_abs_node("/test_ECFLOW_189/f1/t2")
        assert task_t1 is not None, "Could not find /test_ECFLOW_189/f1/t1"
        assert task_t2 is not None, "Could not find /test_ECFLOW_189/f1/t2"

        assert (
            task_t1.get_state() == State.queued
        ), "Expected state queued but found " + str(task_t1.get_state())
        assert (
            task_t2.get_state() == State.queued
        ), "Expected state queued but found " + str(task_t2.get_state())
        assert (
            task_t1.get_dstate() == DState.suspended
        ), "Expected state suspended but found " + str(task_t1.get_dstate())
        assert (
            task_t2.get_dstate() == DState.suspended
        ), "Expected state suspended but found " + str(task_t2.get_dstate())

        # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
        self.ci.resume("/test_ECFLOW_189/f1/t1")
        self.ci.resume("/test_ECFLOW_189/f1/t2")

        time.sleep(3)
        sync_local(self.ci)  # get the changes, synced with local defs
        # print(ci.get_defs())
        task_t1 = self.ci.get_defs().find_abs_node("/test_ECFLOW_189/f1/t1")
        task_t2 = self.ci.get_defs().find_abs_node("/test_ECFLOW_189/f1/t2")
        assert (
            task_t1.get_state() == State.queued
        ), "Expected state queued but found " + str(task_t1.get_state())
        assert (
            task_t2.get_state() == State.queued
        ), "Expected state queued but found " + str(task_t2.get_state())
        assert (
            task_t1.get_dstate() == DState.queued
        ), "Expected state queued but found " + str(task_t1.get_dstate())
        assert (
            task_t2.get_dstate() == DState.queued
        ), "Expected state queued but found " + str(task_t2.get_dstate())

        dir_to_remove = Test.ecf_home(port) + "/" + "test_ECFLOW_189"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_ECFLOW_199(self):
        print_test(self.ci, "test_ECFLOW_199")
        self.ci.delete_all()
        port = self.ci.get_port()
        defs = create_defs("test_ECFLOW_199", port, self.protocol)
        defs.generate_scripts()
        msg = defs.check_job_creation(verbose=True)
        assert len(msg) == 0, msg

        self.ci.restart_server()
        self.ci.load(defs)

        self.ci.suspend("/test_ECFLOW_199")
        self.ci.suspend("/test_ECFLOW_199/f1/t1")
        self.ci.suspend("/test_ECFLOW_199/f1/t2")

        self.ci.begin_all_suites()

        sync_local(self.ci)  # get the changes, synced with local defs
        # print(ci.get_defs())
        assert len(list(self.ci.changed_node_paths)) == 0, (
            "Expected first call to sync_local, to have no changed paths but found "
            + str(len(list(self.ci.changed_node_paths)))
        )

        # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
        # Note: ECFLOW-1512, we may get additional paths.( i.e. suite path) due to changes in suite calendar
        self.ci.resume("/test_ECFLOW_199/f1/t1")
        sync_local(self.ci)
        found_path = False
        for path in self.ci.changed_node_paths:
            print("   changed node path " + path)
            if path == "/test_ECFLOW_199/f1/t1":
                found_path = True
        assert found_path, "Expected '/test_ECFLOW_199/f1/t1' in list of changed paths"

        self.ci.resume("/test_ECFLOW_199/f1/t2")
        sync_local(self.ci)
        found_path = False
        for path in self.ci.changed_node_paths:
            print("   changed node path " + path)
            if path == "/test_ECFLOW_199/f1/t2":
                found_path = True
        assert found_path, "Expected '/test_ECFLOW_199/f1/t2' in list of changed paths"

        dir_to_remove = Test.ecf_home(port) + "/" + "test_ECFLOW_199"
        shutil.rmtree(dir_to_remove, ignore_errors=True)

    def test_ECFLOW_1761(self):
        print_test(self.ci, "test_ECFLOW_1761")
        print("# get log: last 10 lines")
        self.ci.get_log(10)

        ci_defs = self.ci.get_defs()
        task = ci_defs.get_all_nodes()[0]  # take first task
