#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2017 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#
# Utility code used in the tests.
#
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
from socket import gethostname 
import os,sys,fnmatch
import fcntl
import datetime,time
import shutil   # used to remove directory tree
import platform # for python version info

from ecflow import Client, debug_build, File

def all_files(root, patterns='*', single_level=False, yield_folders=False):
    """Expand patterns from semi-colon separated string to list"""
    patterns = patterns.split(';')
    for path, subdirs, files in os.walk(root):
        if yield_folders:
            files.extend(subdirs)
        files.sort()
        for name in files:
            for pattern in patterns:
                if fnmatch.fnmatch(name,pattern):
                    yield os.path.join(path, name)
                    break
        if single_level:
            break    
        
# Enable to stop data being deleted, and stop server from being terminated
def debugging() : return False

def ecf_home(port): 
    # debug_build() is defined for ecflow. Used in test to distinguish debug/release ecflow
    # Vary ECF_HOME based on debug/release/port/python version, allowing multiple invocations of these tests
    ecf_home_base = "/test/data/ecf_home_py" + str(sys.version_info[0])
    if debug_build():
        return os.getcwd() + ecf_home_base + "_debug_" + str(port)
    return os.getcwd() + ecf_home_base + "_release_" + str(port)

def get_parent_dir(file_path):
    return os.path.dirname(file_path)

def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"
def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"
def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"
def white_list_file_path(port): return "./" + gethostname() + "." + port + ".ecf.lists"

def print_test_start():
    print("#######################################################################################")
    print("ecflow version(" + Client().version() + ") debug build(" + str(debug_build()) +")  python(" + platform.python_version() + ")")
    print("PYTHONPATH                : " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    #print("sys.path                  : " + str(sys.path))
    print("Current working directory : " + str(os.getcwd()))
    print("Python version            : " + str(sys.version_info[0]) + "." + str(sys.version_info[1]))
    print("#######################################################################################")
    
def clean_up_server(port):
    print("   clean_up " + port)
    try: os.remove(log_file_path(port))
    except: pass
    try: os.remove(checkpt_file_path(port))
    except: pass
    try: os.remove(backup_checkpt_file_path(port))
    except: pass
    try: os.remove(white_list_file_path(port))  
    except: pass
    
def clean_up_data(port):
    print("   Attempting to Removing ECF_HOME " + ecf_home(port))
    try: 
        shutil.rmtree(ecf_home(port),True)   # True means ignore errors 
        print("   Remove OK") 
    except: 
        print("   Remove Failed") 
        pass
        
# =======================================================================================
class EcfPortLock(object):
    """allow debug and release version of python tests to run at the same
    time, buy generating a unique port each time"""
    def __init__(self):
        print("   EcfPortLock:__init__")
        pass
    
    def find_free_port(self,seed_port):
        print("   EcfPortLock:find_free_port starting with " + str(seed_port))
        at_time = datetime.datetime.fromtimestamp(time.time()).strftime('%H:%M:%S')
        port = seed_port
        while 1:
            if self._free_port(port) == True:
                print("   *FOUND* free server port " + str(port) + " : " + at_time)
                file = self._lock_file(port)
                at_time = datetime.datetime.fromtimestamp(time.time()).strftime('%H:%M:%S')
                if os.path.exists(file):
                    print("   *LOCKED* lock file exists " + file + " : " + at_time  + " ignoring")
                else:
                    break
            else:
                print("   *Server* port " + str(port) + " busy, trying next port " + at_time)
            port = port + 1
            
        return str(port)  
    
    def _free_port(self,port):
        try:
            ci = Client()
            ci.set_host_port("localhost",str(port))
            ci.ping() 
            return False
        except RuntimeError as e:
            return True
            
    def do_lock(self,port):
        file = self._lock_file(port)
        at_time = datetime.datetime.fromtimestamp(time.time()).strftime('%H:%M:%S')
        if os.path.exists(file):
            print("   *LOCKED* lock file exists " + file + " : " + at_time )
            return False
        try:
            fp = open(file, 'w') 
            try:
                self.lock_time = at_time
                fcntl.lockf(fp, fcntl.LOCK_EX | fcntl.LOCK_NB)
                self.lock_file_fp = fp
                print("   *LOCKED* file " + file + " : " + self.lock_time )
                return True;
            except IOError:
                print("   Could *NOT* lock file " + file + " trying next port : " + at_time)
                return False
        except IOError as e:
             print("   Could not open file " + file + " for write trying next port : " + at_time)
             return False
        
    def remove(self,port):
        release_time = datetime.datetime.fromtimestamp(time.time()).strftime('%H:%M:%S')
        file = self._lock_file(port)
        print("   Remove lock file : " + file + " : lock_time: " + self.lock_time + " release_time: " + release_time)
        self.lock_file_fp.close()
        os.remove(file)
    
    def _lock_file(self,port):
        if "ECF_PORT_LOCK_DIR" in os.environ:
            lock_file = os.environ["ECF_PORT_LOCK_DIR"] + "/" + str(port) + ".lock"
            #print("     EcfPortLock::_lock_file ECF_PORT_LOCK_DIR: " + lock_file)
            return lock_file
        if os.path.exists(File.source_dir()):
            lock_file = File.source_dir() + "/" + str(port) + ".lock"
            #print("     EcfPortLock::_lock_file File::source_dir(): " + lock_file)
            return lock_file
        lock_file = os.getcwd() + "/" + str(port) + ".lock"
        #print("     EcfPortLock::_lock_file os.getcwd(): " + lock_file)
        return lock_file
        
# ===============================================================================

class Server(object):
    """TestServer: allow debug and release version of python tests to run at the same
    time, by generating a unique port each time"""
    def __init__(self):
        print("Server:__init__: Starting server")      
        if not debugging():
            seed_port = 3153
            if debug_build(): seed_port = 3152
            if sys.version_info[0] == 3: # python3 can run at same time
                seed_port = 3200
                if debug_build(): seed_port = 3201
            self.lock_file = EcfPortLock()
            self.the_port = self.lock_file.find_free_port(seed_port)   
        else:
            self.the_port = "3152"
            
        # Only worth doing this test, if the server is running
        # ON HPUX, having only one connection attempt, sometimes fails
        #ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
        #ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
        self.ci = Client("localhost", self.the_port)

    def __enter__(self):
        try:
            st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
            print("Server:__enter__: About to ping localhost: " + self.the_port +  " : " + st)       
            self.ci.ping() 
            print("   ------- Server all ready running on port " + self.the_port + " *UNEXPECTED* ------")
            sys.exit(1)
        except RuntimeError as e:
            while 1:
                # Only worth doing this test, if the server is running
                # ON HPUX, having only one connection attempt, sometimes fails
                #ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
                #ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
                self.ci = Client("localhost", self.the_port)

                print("   ------- Server *NOT* running on port " + self.the_port + " as *EXPECTED* ------ ") 
                print("   ------- Start the server on port " + self.the_port + " ---------")  
                clean_up_server(str(self.the_port))
                clean_up_data(str(self.the_port))
        
                server_exe = File.find_server();
                assert len(server_exe) != 0, "Could not locate the server executable"
            
                server_exe += " --port=" + self.the_port + " --ecfinterval=4 &"
                print("   TestClient.py: Starting server " + server_exe)
                os.system(server_exe) 
            
                print("   Allow time for server to start")
                if self.ci.wait_for_server_reply() :
                    print("   Server has started, trying to lock file")
                    locked = self.lock_file.do_lock( int(self.the_port) )
                    assert locked ,"   Could not creat lock file for port " + self.the_port
                    break
                else:
                    print("   Server failed to start after 60 second, trying next port !!!!!!")
                    self.the_port = self.lock_file.find_free_port( int(self.the_port) + 1 )   
             
        print("   Run the tests, leaving Server:__enter__:") 

        # return the Client, that can call to the server
        return self.ci
    
    def __exit__(self,exctype,value,tb):
        print("Server:__exit__: Kill the server, clean up log file, check pt files and lock files, ECF_HOME")
        print("   exctype:",exctype)
        print("   value:",value)
        print("   tb:",tb)
        print("   Terminate server")
        self.ci.terminate_server() 
        time.sleep(1) 
        try:  
            self.ci.ping() 
            print("   Terminate server Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            exit(1)
        except RuntimeError as e:
            print("   Terminate server OK")
            self.lock_file.remove(self.the_port)
            if not debugging():
                clean_up_server(str(self.the_port))
        
            # Do not clean up data, if an assert was raised. This allow debug
            if exctype == None and not debugging():
                clean_up_data(str(self.the_port))
        return False
        