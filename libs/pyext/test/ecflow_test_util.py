#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

from platform import python_version
from socket import gethostname
import os, sys, fnmatch
import time
import fcntl
import datetime, time
import shutil  # used to remove directory tree
import platform  # for python version info
import inspect
import pathlib
from enum import Enum

import ecflow as ecf


def all_files(root, patterns='*', single_level=False, yield_folders=False):
    """Expand patterns from semi-colon separated string to list"""
    patterns = patterns.split(';')
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


def debugging(): return False


def ecf_home(port):
    # Allowing multiple invocations of these tests, by creating a unique ECF_HOME based on:
    #  - python version
    #  - process id
    #  - build_type
    #  - server port,
    python_version = sys.version_info[0]
    build_type = "debug" if ecf.debug_build() else "release"
    pid = os.getpid()

    ecf_home_dir = f"ecf_home_py{python_version}__pid_{pid}__build_{build_type}__port_{port}"

    ecf_home_path = pathlib.Path.joinpath(pathlib.Path.cwd(), "test", "data", ecf_home_dir)

    return str(ecf_home_path.absolute())


def get_parent_dir(file_path):
    return os.path.dirname(file_path)


def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"


def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"


def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"


def white_list_file_path(port): return "./" + gethostname() + "." + port + ".ecf.lists"


def print_test_start(test_name):
    print("#######################################################################################")
    print(test_name)
    print("ecflow version(" + ecf.Client().version() + ") debug build(" + str(ecf.debug_build()) + ")  python(" + platform.python_version() + ")")
    print("PYTHONPATH                : " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    # print("sys.path                  : " + str(sys.path))
    print("Current working directory : " + str(os.getcwd()))
    print("Python version            : " + str(sys.version_info[0]) + "." + str(sys.version_info[1]))
    print("#######################################################################################")


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
        pass


class Protocol(Enum):
    CUSTOM = 1  # uses custom TCP/IP access (default)
    HTTP = 2  # uses HTTP access


# =======================================================================================
class EcfPortLock(object):
    """allow debug and release version of python tests to run at the same
    time, buy generating a unique port each time"""

    def __init__(self, protocol):
        print("   EcfPortLock:__init__")
        self._protocol = protocol
        pass

    def at_time(self):
        return datetime.datetime.fromtimestamp(time.time()).strftime('%H:%M:%S')

    def find_free_port(self, seed_port):
        print("   EcfPortLock:find_free_port starting with " + str(seed_port) + " at time " + self.at_time())
        port = seed_port
        while 1:
            # port must be free for at least 15 seconds
            if self._timed_free_port(port, 3) == True:
                print("   *FOUND* free server port " + str(port) + " : " + self.at_time())
                if self.do_lock(port):
                    break
            else:
                print("   *Server* port " + str(port) + " busy( by  ping), trying next port " + self.at_time())
            port = port + 1

        return str(port)

    def _timed_free_port(self, port, wait_time=10):
        count = 0
        while count < wait_time:
            if self._free_port(port) == True:
                count = count + 1
                time.sleep(1)
            else:
                return False
        return self._free_port(port)

    def _free_port(self, port):
        try:
            ci = ecf.Client()
            if self._protocol == Protocol.HTTP:
                ci.enable_http()
            ci.set_host_port("localhost", str(port))
            ci.ping()
            return False
        except RuntimeError as e:
            return True

    def do_lock(self, port):
        file = self._lock_file(port)
        if os.path.exists(file):
            print("   *LOCKED* lock file exists " + file + " trying next port : " + self.at_time())
            return False
        try:
            fp = open(file, 'w')
            try:
                self.lock_time = self.at_time()
                fcntl.lockf(fp, fcntl.LOCK_EX | fcntl.LOCK_NB)
                self.lock_file_fp = fp
                print("   *LOCKED* file " + file + " : " + self.lock_time)
                return True;
            except IOError:
                print("   Could *NOT* lock file " + file + " trying next port : " + self.at_time())
                return False
        except IOError as e:
            print("   Could not open file " + file + " for write trying next port : " + self.at_time())
            return False

    def remove(self, port):
        file = self._lock_file(port)
        print("   Remove lock file : " + file + " : lock_time: " + self.lock_time + " release_time: " + self.at_time())
        self.lock_file_fp.close()
        os.remove(file)

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

    def __init__(self, protocol=Protocol.CUSTOM):
        print("Server:__init__: Starting server")
        self._protocol = protocol
        if not debugging():
            seed_port = int(os.getenv('ECF_FREE_PORT', '3153'))
            if sys.version_info[0] == 3:  # python3 can run at same time
                seed_port = int(os.getenv('ECF_FREE_PORT', '3154'))
            if ecf.debug_build(): seed_port = seed_port + 1
            self.lock_file = EcfPortLock(self._protocol)
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
        return datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')

    def __enter__(self):
        try:
            print("Server:__enter__: About to ping localhost: " + self.the_port + " : " + self.at_time())
            self.ci.ping()
            print("   ------- Server all ready running on port " + self.the_port + " *UNEXPECTED* -- : " + self.at_time())
            sys.exit(1)
        except RuntimeError as e:
            while 1:
                # Only worth doing this test, if the server is running
                # ON HPUX, having only one connection attempt, sometimes fails
                # ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
                # ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
                self.ci = ecf.Client("localhost", self.the_port)
                if self._protocol == Protocol.HTTP:
                    self.ci.enable_http()

                print("   ------- Server *NOT* running on port " + self.the_port + " as *EXPECTED* ------ ")
                print("   ------- Start the server on port " + self.the_port + " ---------")
                if not debugging():
                    clean_up_server(str(self.the_port))
                    clean_up_data(str(self.the_port))

                server_exe = ecf.File.find_server();
                assert len(server_exe) != 0, "Could not locate the server executable"

                server_exe += " --port=" + self.the_port + " --ecfinterval=4"
                if self._protocol == Protocol.HTTP:
                    server_exe += " --http"
                server_exe += " &"
                print("   TestClient.py: Starting server " + server_exe)
                os.system(server_exe)

                print("   Allow time for server to start " + self.at_time())
                if self.ci.wait_for_server_reply():
                    print("   Server has started " + self.at_time())
                    break
                else:
                    print("   Server failed to start after 60 second, trying next port !!!!!! " + self.at_time())
                    self.the_port = self.lock_file.find_free_port(int(self.the_port) + 1)

        print("   Run the tests, leaving Server:__enter__:")

        # return the Client, that can call to the server
        return [self.ci, self._protocol]

    def __exit__(self, exctype, value, tb):
        print("Server:__exit__: Kill the server, clean up log file, check pt files and lock files, ECF_HOME " + self.at_time())
        print("   exctype:", exctype)
        print("   value:", value)
        print("   tb:", tb)
        print("   Terminate server " + self.at_time())
        self.ci.terminate_server()
        print("   Terminate server OK " + self.at_time())
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
        self.original_env = {};
        for (name, value) in self.expected_env.items():
            if name in os.environ:
                self.original_env[name] = os.environ[name]
            else:
                self.original_env[name] = None
            os.environ[name] = value

    def __exit__(self, exception_type, exception_value, exception_traceback):
        for (name, value) in self.original_env.items():
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
        pass

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
    print("#######################################################################################")
    print(" All Tests passed")
    print("#######################################################################################")


def execute_all(clazz):
    test_started()

    for name, test in inspect.getmembers(clazz):
        if inspect.isfunction(test):
            test()

    test_success()
