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
    # Vary ECF_HOME based on debug/release/port allowing multiple invocations of these tests
    if debug_build():
        return os.getcwd() + "/test/data/ecf_home_debug_" + str(port)
    return os.getcwd() + "/test/data/ecf_home_release_" + str(port)

def get_parent_dir(file_path):
    return os.path.dirname(file_path)

def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"
def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"
def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"
def white_list_file_path(port): return "./" + gethostname() + "." + port + ".ecf.lists"

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
                if self._do_lock(port) == True:
                    break;
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
            
    def _do_lock(self,port):
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
        seed_port = 3153
        if debug_build(): seed_port = 3152
        self.lock_file = EcfPortLock()
        self.the_port = self.lock_file.find_free_port(seed_port)   
        print("   Server:__init__: Starting server on port " + self.the_port)      

        # Only worth doing this test, if the server is running
        # ON HPUX, having only one connection attempt, sometimes fails
        #ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
        #ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
        if 'ECF_HOME' in os.environ:     print("   ECF_HOME:",os.environ['ECF_HOME']) 
        if 'ECF_PORT' in os.environ:     print("   ECF_PORT:",os.environ['ECF_PORT']) 
        if 'ECF_HOST' in os.environ:     print("   ECF_HOST:",os.environ['ECF_HOST']) 
        if 'ECF_NODE' in os.environ:     print("   ECF_NODE:",os.environ['ECF_NODE']) 
        if 'ECF_RID' in os.environ:      print("   ECF_RID:",os.environ['ECF_RID']) 
        if 'ECF_LISTS' in os.environ:    print("   ECF_LISTS:",os.environ['ECF_LISTS']) 
        if 'ECF_CHECK' in os.environ:    print("   ECF_CHECK:",os.environ['ECF_CHECK']) 
        if 'ECF_CHECKOLD' in os.environ: print("   ECF_CHECKOLD:",os.environ['ECF_CHECKOLD']) 
        if 'ECF_LOG' in os.environ:      print("   ECF_LOG:",os.environ['ECF_LOG']) 
        self.ci = Client("localhost", self.the_port)
     
    def __enter__(self):
        try:
            st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
            print("Server:__enter__: About to ping localhost: " + self.the_port +  " : " + st)       
            self.ci.ping() 
            print("   ------- Server all ready running on port " + self.the_port + " *UNEXPECTED* ------")
            sys.exit(1)
        except RuntimeError as e:
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
                print("   Server has started")
            else:
                print("   Server failed to start after 60 second !!!!!!")
                assert False , "Server failed to start after 60 second !!!!!!"
            
        server_version = self.ci.server_version()
        print("   Server version is : " + server_version)
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
        print("   Terminate server OK")
        self.lock_file.remove(self.the_port)
        if not debugging():
            clean_up_server(str(self.the_port))
        
        # Do not clean up data, if an assert was raised. This allow debug
        if exctype == None and not debugging():
            clean_up_data(str(self.the_port))
        return False
        