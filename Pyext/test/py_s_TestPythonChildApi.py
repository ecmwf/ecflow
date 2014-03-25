#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2012 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#  code for testing client code in python
import time
import os
import pwd
import fcntl
from datetime import datetime
from socket import gethostname 
import shutil   # used to remove directory tree
from ecflow import Defs, Clock, DState,  Style, State, PrintStyle, File, Client, SState, debug_build

# Enable to stop data being deleted, and stop server from being terminated
def debugging() : return False

def ecf_home(): 
    # the_port is global
    # debug_build() is defined for ecflow. Used in test to distinguish debug/release ecflow
    # Vary ECF_HOME based on debug/release/port allowing multiple invocations of these tests
    if debug_build():
        return os.getcwd() + "/test/data/ecf_home_debug_" + str(the_port)
    return os.getcwd() + "/test/data/ecf_home_release_" + str(the_port)

def ecf_includes() :  return os.getcwd() + "/test/data/python_includes"

def create_defs(name=""):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    suite = defs.add_suite(suite_name);
    
    ecfhome = ecf_home();
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("SLEEP", "1");  # not strictly required since default is 1 second
    suite.add_variable("ECF_INCLUDE", ecf_includes());

    family = suite.add_family("f1")
    task = family.add_task("t1")
    task.add_event("event_fred")
    task.add_meter("meter", 0, 100)
    task.add_label("label_name", "value")

    return defs;
    

def get_username(): return pwd.getpwuid(os.getuid())[ 0 ]

def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"
def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"
def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"
  
def test_client_run(ci):            
    print "test_client_run"
    ci.delete_all()     
    defs = create_defs("test_client_run")  
    suite = defs.find_suite("test_client_run")
    suite.add_defstatus(DState.suspended)

    # create the ecf file
    dir = ecf_home() + "/test_client_run/f1"
    if not os.path.exists(dir): os.makedirs(dir)
    file = dir + "/t1.ecf"
    
    contents = "%include <head.h>\n\n"
    contents += "print 'doing some work'\n"
    contents += "try:\n"
    contents += "    ci.child_event('event_fred')\n"
    contents += "    ci.child_meter('meter',100)\n"
    contents += "    ci.child_label('label_name','100')\n"
    contents += "except:\n"
    contents += "    ci.child_abort()\n\n"
    contents += "%include <tail.h>\n"
     
    open(file,'w').write(contents)
      
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    ci.run("/test_client_run", False)
    
    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        suite = ci.get_defs().find_suite("test_client_run")
        assert suite != None, "Expected to find suite test_client_run:\n" + str(ci.get_defs())
        if suite.get_state() == State.complete:
            break;
        assert suite.get_state() != State.aborted, "Suite aborted"
        time.sleep(2)
        if count > 20:
            assert False, "test_client_run aborted after " + str(count) + " loops:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    
    if not debugging():
        dir_to_remove = ecf_home() + "/" + "test_client_run"
        shutil.rmtree(dir_to_remove)      

           
# =======================================================================================
class EcfPortLock(object):
    """allow debug and release version of python tests to run at the same
    time, buy generating a unique port each time"""
    def __init__(self):
        pass
    
    def find_free_port(self,seed_port):
        print "EcfPortLock:find_free_port starting with " + str(seed_port)
        port = seed_port
        while 1:
            if self._free_port(port) == True:
                print "   *FOUND* free server port " + str(port)
                if self._do_lock(port) == True:
                    break;
            else:
                 print "   *Server* port " + str(port) + " busy, trying next port"
            port = port + 1
            
        return str(port)  
    
    def _free_port(self,port):
        try:
            ci = Client()
            ci.set_host_port("localhost",str(port))
            ci.ping() 
            return False
        except RuntimeError, e:
            return True
            
    def _do_lock(self,port):
        file = self._lock_file(port)
        try:
            fp = open(file, 'w') 
            try:
                fcntl.lockf(fp, fcntl.LOCK_EX | fcntl.LOCK_NB)
                self.lock_file_fp = fp
                print "   *LOCKED* file " + file
                return True;
            except IOError:
                print "   Could *NOT* lock file " + file + " trying next port"
                return False
        except IOError, e:
             print "   Could not open file " + file + " for write trying next port"
             return False
        
    def remove(self,port):
        self.lock_file_fp.close()
        os.remove(self._lock_file(port))
    
    def _lock_file(self,port):
        lock_file = str(port) + ".lock"
        return lock_file
        
        
def clean_up(port):
    try: os.remove(log_file_path(port))
    except: pass
    try: os.remove(checkpt_file_path(port))
    except: pass
    try: os.remove(backup_checkpt_file_path(port))
    except: pass
    try: os.remove(white_list_file_path(port))  
    except: pass
    try: 
        print "Removing ECF_HOME " + ecf_home()
        shutil.rmtree(ecf_home(),True)   # True means ignore errors  
    except: pass
               
               
if __name__ == "__main__":
    print "####################################################################"
    print "Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")"
    print "####################################################################"

    # =========================================================================
    # server dependent tests
    if not debugging():
        seed_port = 3153
        if debug_build(): seed_port = 3152
        lock_file = EcfPortLock()
        global the_port
        the_port = lock_file.find_free_port(seed_port)   
    else:
        the_port = "3152"

    print "Creating client: on port " + str(the_port)
     
    # Only worth doing this test, if the server is running
    # ON HPUX, have only one connection attempt, sometimes fails
    #ci.set_connection_attempts(1)     # improve responsiveness only make 1 attempt to connect to server
    #ci.set_retry_connection_period(0) # Only applicable when make more than one attempt. Added to check api.
    ci = Client("localhost", the_port)
    print "About to ping localhost:" + the_port
    try:
        ci.ping() 
        print "------- Server all ready running *UNEXPECTED* ------"
    except RuntimeError, e:
        print "------- Server *NOT* running as *EXPECTED* ------ " 
        print "------- Start the server on port " + the_port + " ---------"  
        clean_up(the_port)
    
        server_exe = File.find_server();
        assert len(server_exe) != 0, "Could not locate the server executable"
        
        server_exe += " --port=" + the_port + " --ecfinterval=4 &"
        print "TestClient.py: Starting server ", server_exe
        os.system(server_exe) 
        
        print "allow time for server to start"
        if ci.wait_for_server_reply() :
            print "Server has started"
        else:
            print "Server failed to start after 60 second !!!!!!"
            assert False , "Server failed to start after 60 second !!!!!!"
    try:
        print "run the tests" 

        PrintStyle.set_style( Style.STATE ) # show node state 
        test_client_run(ci)  

        print "All Tests pass ======================================================================"    
    finally:
        print "Finally, Kill the server, clean up log file, check pt files and lock files used in test"
        if not debugging():
            ci.terminate_server()  
            clean_up(the_port)
            lock_file.remove(the_port)
