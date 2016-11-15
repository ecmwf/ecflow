#!/usr/bin/env python2.7
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2016 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
import argparse # for argument parsing  
import time
import os
import pwd
from datetime import datetime
from socket import gethostname 
import shutil   # used to remove directory tree

# ecflow_test_util, see File ecflow_test_util.py
import ecflow
from ecflow import Defs, Clock, DState,  Style, State, RepeatDate, PrintStyle, File, Client, SState, \
                   JobCreationCtrl, CheckPt, Cron, Late, debug_build, Flag, FlagType

class Tester(object) : 
    def __init__(self,ci,args):
        self.ci_ = ci
        self.port_ = self.ci_.get_port()
        self.ARGS_ = args
            
    def clean_up_server(self):
        self.log_msg("   clean_up " + self.port_)
        #try: os.remove(log_file_path(port))
        #except: pass
        try: os.remove(self.checkpt_file_path())
        except: pass
        try: os.remove(self.backup_checkpt_file_path())
        except: pass
        try: os.remove(self.white_list_file_path())  
        except: pass
    
    def clean_up_data(self):
        self.log_msg("   Attempting to Removing ECF_HOME " + self.ecf_home())
        try: 
            shutil.rmtree(self.ecf_home(),True)   # True means ignore errors
            shutil.rmtree("test_gui",True)   # True means ignore errors 
            print("   Remove OK") 
        except: 
            print("   Remove Failed") 
            pass
        
    def ecf_home(self): 
        # debug_build() is defined for ecflow. Used in test to distinguish debug/release ecflow
        # Vary ECF_HOME based on debug/release/port allowing multiple invocations of these tests
        if debug_build():
            return os.getcwd() + "/test_gui/data/ecf_home_debug_" + str(self_.port_)
        return os.getcwd() + "/test_gui/data/ecf_home_release_" + str(self.port_)

    def ecf_includes(self) :  return os.getcwd() + "Pyext/test/data/includes"
    def log_file_path(self): return "./" + gethostname() + "." + self.port_ + ".ecf.log"
    def checkpt_file_path(self): return "./" + gethostname() + "." + self.port_ + ".ecf.check"
    def backup_checkpt_file_path(self): return "./" + gethostname() + "." + self.port_ + ".ecf.check.b"
    def white_list_file_path(self): return "./" + gethostname() + "." + self.port_ + ".ecf.lists"

    def path_to_ecflow_client(self):
        # to use the build, in preference, to release build
        path_to_client = File.find_client()
        if os.path.exists(path_to_client):
            return path_to_client
        return "/usr/local/apps/ecflow/" + self.ci_.version() + "/bin/ecflow_client"
    
    def create_defs(self,name=""):
        defs = Defs()
        suite_name = name
        if len(suite_name) == 0: suite_name = "s1"
        suite = defs.add_suite(suite_name);
        
        ecfhome = self.ecf_home()
        suite.add_variable("ECF_HOME", ecfhome)
        suite.add_variable("ECF_CLIENT_EXE_PATH", self.path_to_ecflow_client())
        suite.add_variable("SLEEP", "1");  # not strictly required since default is 1 second
        suite.add_variable("ECF_INCLUDE", self.ecf_includes());
    
        family = suite.add_family("f1")
        family.add_task("t1")
        family.add_task("t2")
        return defs;

    def sync_local(self,sleep_time=4):
        """Delay added so that we can see the change in the GUI.
           The GUI refresh rate should be set to 1 second
           For check test are working comment out the delay
           """
        self.ci_.sync_local()
        if self.ARGS_.sync_sleep != 0 and sleep_time != 0:
            time.sleep(sleep_time)
     
    def log_msg(self,msg):
        print(msg)
        self.ci_.log_msg("======================== " + msg + " ========================")
    
    def test_version(self):
        self.log_msg("test_version")
        client_version = self.ci_.version();
        server_version = self.ci_.server_version();
        assert client_version == server_version, "Expected client version(" + client_version +") and server version(" +  server_version + ") to match\n";
        
    def test_client_get_server_defs(self):
        test = "test_client_get_server_defs"
        self.log_msg(test)
        self.ci_.load(self.create_defs(test))  
        self.sync_local()  
        assert self.ci_.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(self.ci_.get_defs())
        
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        self.sync_local()  
    
    def test_client_restart_server(self):
        self.log_msg("test_client_restart_server")
        self.ci_.restart_server()
        self.sync_local()
        assert self.ci_.get_defs().get_server_state() == SState.RUNNING, "Expected server to be running"
        
        paths = list(self.ci_.changed_node_paths)
        #for path in self.ci_.changed_node_paths:
        #    print("   changed node path " + path);
        assert len(paths) == 1, "expected changed node to be the root node"
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])
    
    def test_client_halt_server(self):
        self.log_msg("test_client_halt_server")
        self.ci_.halt_server()
        self.sync_local()
        assert self.ci_.get_defs().get_server_state() == SState.HALTED, "Expected server to be halted"
        
        paths = list(self.ci_.changed_node_paths)
        #for path in self.ci_.changed_node_paths:
        #    print("   changed node path " + path);
        assert len(paths) == 1, "expected changed node to be the root node"
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])
        self.ci_.restart_server()   
    
    def test_client_shutdown_server(self):
        self.log_msg("test_client_shutdown_server")
        self.ci_.shutdown_server()
        self.sync_local()
        assert self.ci_.get_defs().get_server_state() == SState.SHUTDOWN, "Expected server to be shutdown"
        
        paths = list(self.ci_.changed_node_paths)
        #for path in self.ci_.changed_node_paths:
        #    print("   changed node path " + path);
        assert len(paths) == 1, "expected changed node to be the root node"
        assert paths[0] == "/", "Expected root path but found " + str(paths[0])
    
    def test_client_load_in_memory_defs(self):
        test = "test_client_load_in_memory_defs"
        self.log_msg(test)
        self.ci_.load(self.create_defs(test))  
        self.sync_local() 
        assert self.ci_.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(self.ci_.get_defs())              
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
    def test_client_load_from_disk(self):            
        test = "test_client_load_from_disk"
        self.log_msg(test)
        defs = self.create_defs(test);
        defs_file = test + ".def"
        defs.save_as_defs(defs_file)     
        assert os.path.exists(defs_file), "Expected file " + defs_file + " to exist after defs.save_as_defs()"
        self.ci_.load(defs_file) # open and parse defs file, and load into server.\n"
            
        # check load worked
        self.sync_local() 
        assert self.ci_.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(self.ci_.get_defs())
        os.remove(defs_file)
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
    def test_client_checkpt(self):
        test = "test_client_checkpt"
        self.log_msg(test)
        try:    
            os.remove(self.checkpt_file_path())
            os.remove(self.backup_checkpt_file_path())
        except: pass
        
        self.ci_.load(self.create_defs(test))  
        self.sync_local() 
        self.ci_.checkpt()
        assert os.path.exists(self.checkpt_file_path()), "Expected check pt file " + self.checkpt_file_path() + " to exist after self.ci_.checkpt()"
        assert os.path.exists(self.backup_checkpt_file_path()) == False, "Expected back up check pt file to *NOT* exist"
        
        self.ci_.checkpt()   # second check pt should cause backup check pt to be written
        assert os.path.exists(self.backup_checkpt_file_path()), "Expected back up check pt file to exist after second self.ci_.checkpt()"
    
        self.ci_.checkpt(CheckPt.NEVER)         # switch of check pointing
        self.ci_.checkpt(CheckPt.ALWAYS)        # always check point, at any state change
        self.ci_.checkpt(CheckPt.ON_TIME)       # Check point periodically, by interval set in server
        self.ci_.checkpt(CheckPt.ON_TIME, 200)  # Check point periodically, by interval set in server
        self.ci_.checkpt(CheckPt.UNDEFINED, 0, 35)  # Change check point save time alarm
    
        os.remove(self.checkpt_file_path())
        os.remove(self.backup_checkpt_file_path())
        
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
    def test_client_restore_from_checkpt(self):          
        test = "test_client_restore_from_checkpt"
        self.log_msg(test)
        try:    
            os.remove(self.checkpt_file_path())
            os.remove(self.backup_checkpt_file_path())
        except: pass
        
        self.ci_.load(self.create_defs(test))  
        self.sync_local()  
        self.ci_.checkpt()
        self.ci_.delete_all() 
        
        self.sync_local() 
        assert len(list(self.ci_.get_defs().suites)) == 0, "Expected all suites to be delete:\n"
        
        self.ci_.halt_server()  # server must be halted, otherwise restore_from_checkpt will throw
        self.ci_.restore_from_checkpt()
        
        self.sync_local() 
        assert self.ci_.get_defs().find_suite(test) != None, "Expected to find suite " + test + " after restore from checkpt:\n" + str(self.ci_.get_defs())
    
        os.remove(self.checkpt_file_path())
        self.ci_.restart_server()   
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
    def get_username(self): return pwd.getpwuid(os.getuid())[ 0 ]
    
    def test_client_reload_wl_file(self):
        test = "test_client_reload_wl_file"
        self.log_msg(test)
        
        expected = False
        try:    self.ci_.reload_wl_file();            
        except: expected = True
        assert expected, "Expected reload to fail when no white list specified"
        
        # create a white list file
        wl_file = open(self.white_list_file_path(), 'w')
        wl_file.write("#\n")
        wl_file.write("4.4.14   #   comment\n\n")
        wl_file.write("# These user have read and write access to the server\n")
        wl_file.write(self.get_username() + "\n")  # add current user otherwise remaining test's, wont have access from server anymore
        wl_file.write("axel  # admin\n")
        wl_file.write("john  # admin\n\n")
        wl_file.write("# Read only users\n")
        wl_file.write("-fred   # needs read access only\n")
        wl_file.write("-joe90  # needs read access only\n")
        wl_file.close();
        
        self.ci_.reload_wl_file();  
        os.remove(self.white_list_file_path())          
    
    def test_client_run(self):            
        test = "test_client_run"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_defstatus(DState.suspended)
    
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
        
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local() # clear changed_node_paths 
        self.ci_.begin_suite(test)
        self.ci_.run("/" + test, False)
        
        count = 0
        while 1:
            count += 1
            self.sync_local(0) # get the changes, synced with local defs
            suite = self.ci_.get_defs().find_suite(test)
            assert suite != None, "Expected to find suite " + test + ":\n" + str(self.ci_.get_defs())
            if suite.get_state() == State.complete:
                break;
            time.sleep(3)
            if count > 20:
                assert False, "test_client_run aborted after " + str(count) + " loops:\n" + str(self.ci_.get_defs())
            
        self.ci_.log_msg("Looped " + str(count) + " times")
        
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
    
    def test_client_run_with_multiple_paths(self):            
        test = "test_client_run_with_multiple_paths"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_defstatus(DState.suspended)
    
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
        
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local()  
        self.ci_.begin_suite(test)
        path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2"]
        self.ci_.run( path_list, False)
    
        count = 0
        while 1:
            count += 1
            self.sync_local(0) # get the changes, synced with local defs
            suite = self.ci_.get_defs().find_suite(test)
            assert suite != None, "Expected to find suite " + tests + ":\n" + str(self.ci_.get_defs())
            if suite.get_state() == State.complete:
                break;
            time.sleep(3)
            if count > 20:
                assert False, test + " aborted after " + str(count) + " loops:\n" + str(self.ci_.get_defs())
            
        self.ci_.log_msg("Looped " + str(count) + " times")
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this   
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
    
    def test_client_requeue(self):
        test = "test_client_requeue"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_defstatus(DState.suspended)
         
        defs.generate_scripts();
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
     
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local()  
        self.ci_.begin_suite(test)
        
        self.ci_.force_state_recursive("/" + test,State.unknown)
        self.sync_local();
        suite = self.ci_.get_defs().find_suite(test)
        assert suite.get_state() == State.unknown, "Expected to find suite with state unknown"
    
        self.ci_.requeue("/" + test)
        self.sync_local();
        suite = self.ci_.get_defs().find_suite(test)
        assert suite.get_state() == State.queued, "Expected to find suite with state queued"
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
    
    def test_client_requeue_with_multiple_paths(self):
        test = "test_client_requeue_with_multiple_paths"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_defstatus(DState.suspended)
         
        defs.generate_scripts();
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
     
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local()  
        self.ci_.begin_suite(test)
        
        self.ci_.force_state_recursive("/" + test,State.unknown)
        self.sync_local();
        task1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task1.get_state() == State.unknown, "Expected to find t1 with state unknown"
        assert task2.get_state() == State.unknown, "Expected to find t2 with state unknown"
    
        path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
        self.ci_.requeue( path_list)
        self.sync_local();
        task1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task1.get_state() == State.queued, "Expected to find task t1 with state queued"
        assert task2.get_state() == State.queued, "Expected to find task t2 with state queued"
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
    
    def test_client_free_dep(self):
        test = "test_client_free_dep"
        self.log_msg(test)
           
        # add a real clock, since we are adding date dependencies
        # Note: adding a future time dependency on a task, will cause it to requeue, when complete
        # Hence even when we free these dependency they get requeued.
        # So we use todays date.
        ltime = time.localtime();
        day = ltime.tm_mday
        month = ltime.tm_mon
        year = ltime.tm_year
        
        defs = Defs()
        suite = defs.add_suite(test);
        suite.add_clock(Clock(False)) # true means hybrid, False means real
        ecfhome = self.ecf_home();
        suite.add_variable("ECF_HOME", ecfhome);
        suite.add_variable("ECF_CLIENT_EXE_PATH", self.path_to_ecflow_client());
        suite.add_variable("SLEEPTIME", "1");
        suite.add_variable("ECF_INCLUDE", self.ecf_includes());
        family = suite.add_family("f1")
        family.add_task("t1").add_time("00:01")
        family.add_task("t2").add_date(day,month,year)
        family.add_task("t3").add_trigger("1 == 0")
        t4 = family.add_task("t4")
        t4.add_time("00:01")
        t4.add_date(day,month,year)
        t4.add_trigger("1 == 0")
    
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
     
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local()  
        self.ci_.begin_suite(test)
        
        t1_path = "/" + test + "/f1/t1"
        t2_path = "/" + test + "/f1/t2"
        t3_path = "/" + test + "/f1/t3"
        t4_path = "/" + test + "/f1/t4"
        while 1:
            self.sync_local(0)
            t1 = self.ci_.get_defs().find_abs_node(t1_path)
            t2 = self.ci_.get_defs().find_abs_node(t2_path)
            t3 = self.ci_.get_defs().find_abs_node(t3_path)
            t4 = self.ci_.get_defs().find_abs_node(t4_path)
     
            if t1.get_state() == State.queued: self.ci_.free_time_dep(t1_path)
            if t2.get_state() == State.queued: self.ci_.free_date_dep(t2_path)
            if t3.get_state() == State.queued: self.ci_.free_trigger_dep(t3_path)
            if t4.get_state() == State.queued: self.ci_.free_all_dep(t4_path)
    
            suite = self.ci_.get_defs().find_suite(test)
            if suite.get_state() == State.complete:
                break;
            time.sleep(3)       
                
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)             
    
    
    def test_client_check(self):
        test = "test_client_check"
        self.log_msg(test)
        
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
        suite = defs.add_suite(test)
        family_f1 = suite.add_family("f1")
        family_f1.add_task("p").add_trigger("/a/b/c/d == complete")      # extern path
        family_f1.add_task("q").add_trigger("/a/b/c/d/e:event == set")   # extern event path
        family_f1.add_task("r").add_trigger("/a/b/c/d/e:meter le 30")    # extern meter path
    
        suite.add_inlimit("c1a","/limits")
        suite.add_inlimit("fred")
        
        family_anon = suite.add_family("anon")
        family_anon.add_inlimit("hpcd","limits")
        family_anon.add_task("t1").add_inlimit("sg1","/suiteName")
        family_anon.add_task("t2").add_inlimit("hpcd","/obs/limits")
        family_anon.add_task("t3").add_inlimit("c1a","/limits")
     
        # CLIENT side check
        client_check = defs.check()
        assert len(client_check) == 0, "Expected clean defs check due to externs but found:\n" + client_check + "\n" + str(defs)
    
        # SERVER side check
        self.ci_.load(defs)
        self.sync_local()  
        server_check = self.ci_.check("") # empty string means check the whole defs, otherwise a node path can be specified.
        server_check = self.ci_.check("/" + test)  
        # print server_check
        assert len(server_check) > 0, "Expected defs to fail, since no externs in server "
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        
    def test_client_suites(self):
        test = "test_client_suites"
        self.log_msg(test)
        no_of_suites = len(self.ci_.suites())
    
        defs = self.create_defs("test_client_suites")
        self.ci_.load(defs)  
        assert len(self.ci_.suites()) == no_of_suites + 1 ,"expected " + str(no_of_suites + 1) + " suites"
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        
    def test_client_ch_suites(self):
        test = "test_client_ch_suites"
        self.log_msg(test)
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        suite_names = [ test + '1', test + '2', test + '3' ]
        self.ci_.ch_register(True,suite_names)      # register interest in suites s1,s2,s3 and any new suites
        self.ci_.ch_register(False,[ test + "1"])   # register interest in suites s1 
        self.sync_local();
        
        self.ci_.ch_suites()  # writes to standard out, list of suites and handles
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
        self.sync_local();    
    
    def test_client_ch_register(self):
        test = "test_client_ch_register"
        self.log_msg("test_client_ch_register")
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite( test + str(i))
        self.ci_.load(defs)
        self.sync_local();
        
        suite_names = [ test + '1', test + '2', test + '3' ]
        self.ci_.ch_register(True, suite_names)    # register interest in suites s1,s2,s3 and any new suites
        self.ci_.ch_register(False,suite_names)    # register interest in suites s1,s2,s3 only
        self.sync_local();
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
                
    def test_client_ch_drop(self):
        test = "test_client_ch_drop"
        self.log_msg(test)
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        try:
            # register interest in suites s1,s2,s3 and any new suites
            suite_names = [ test + '1', test + '2', test + '3' ]
            self.ci_.ch_register(True, suite_names)    
            self.sync_local();
        finally:  
            self.ci_.ch_drop()  # drop using handle stored in self.ci_., from last register
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
              
    def test_client_ch_drop_user(self):
        test = "test_client_ch_drop_user"
        self.log_msg(test)
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        try:
            # register interest in suites s1,s2,s3 and any new suites
            suite_names = [ test + '1', test + '2', test + '3' ]
            self.ci_.ch_register(True, suite_names)
            self.sync_local();
        except RuntimeError as e:
            print(str(e))
        
        self.ci_.ch_drop_user("")  # drop all handle associated with current user
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
                
    def test_client_ch_add(self):
        test = "test_client_ch_add"
        self.log_msg(test)
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        try:
            suite_names = []
            self.ci_.ch_register(True,suite_names)        # register interest in any new suites
            suite_names = [ test + '1', test + '2' ]
            self.ci_.ch_add(suite_names)                  # add suites s1,s2 to the last added handle
            suite_names = [ test + '3', test + '4' ]
            self.ci_.ch_add( self.ci_.ch_handle(),suite_names)  # add suites s3,s4 using last handle
            self.sync_local();
        except RuntimeError as e:
            print(str(e))
            
        self.ci_.ch_drop_user("")  # drop all handle associated with current user
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
                
    def test_client_ch_auto_add(self):
        test = "test_client_ch_auto_add"
        self.log_msg(test)
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        try:
            suite_names = [ test + '1', test + '2', test + '3' ]
            self.ci_.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
            self.ci_.ch_auto_add( False )                 # disable adding newly created suites to last registered handle\n"
            self.ci_.ch_auto_add( True )                  # enable adding newly created suites to last registered handle\n"
            self.ci_.ch_auto_add( self.ci_.ch_handle(), False ) # disable adding newly created suites to handle\n"
            self.sync_local();
        except RuntimeError as e:
            print(str(e))
            
        self.ci_.ch_drop_user("")  # drop all handle associated with current user
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
               
    def test_client_ch_remove(self):
        test = "test_client_ch_remove"
        self.log_msg(test)
        try: self.ci_.ch_drop_user("")  # drop all handle associated with current user
        except: pass              # Drop throws if no handle registered
        
        defs = Defs()
        for i in range(1,7): defs.add_suite(test + str(i))
        self.ci_.load(defs)
        self.sync_local()  
        
        try:
            suite_names = [ test + '1', test + '2', test + '3' ]
            self.ci_.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
            suite_names = [ test + '1' ]
            self.ci_.ch_remove( suite_names )          # remove suites s1 from the last added handle\n"
            suite_names = [  test + '2'  ]
            self.ci_.ch_remove( self.ci_.ch_handle(), suite_names )  # remove suites s2 from the last added handle\n"
            self.sync_local();
        except RuntimeError as e:
            print(str(e))
            
        self.ci_.ch_drop_user("")  # drop all handle associated with current user
        for i in range(1,7):  self.ci_.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
               
    def test_client_get_file(self):
        test = "test_client_get_file"
        self.log_msg(test)
        defs = self.create_defs(test)  
          
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
     
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.sync_local()  
        self.ci_.begin_suite(test)
        
        while 1:
            if self.ci_.news_local():
                self.sync_local(0) # get the changes, synced with local defs
                suite = self.ci_.get_defs().find_suite(test)
                assert suite != None, "Expected to find suite"
                if suite.get_state() == State.complete:
                    break;
                time.sleep(3)
        try:
            for file_t in [ 'script', 'job', 'jobout', 'manual' ]:
                the_returned_file = self.ci_.get_file('/' + test +'/f1/t1',file_t)  # make a request to the server
                assert len(the_returned_file) > 0,"Expected self.ci_.get_file(/" + test + "/f1/t1," + file_t + ") to return something"
        except RuntimeError as e:
            print(str(e))
    
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove,True)   # True means ignore errors   
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
        
    def test_client_alter_add(self):
        test = "test_client_alter_add"
        self.log_msg(test)
        self.ci_.load(self.create_defs(test))   
        self.sync_local()  
    
        t1 = "/" + test + "/f1/t1"
        self.ci_.alter(t1,"add","variable","var","var_name")
        self.ci_.alter(t1,"add","time","+00:30")
        self.ci_.alter(t1,"add","time","01:30")
        self.ci_.alter(t1,"add","time","01:30 20:00 00:30")
        self.ci_.alter(t1,"add","today","+00:30")
        self.ci_.alter(t1,"add","today","01:30")
        self.ci_.alter(t1,"add","today","01:30 20:00 00:30")
        self.ci_.alter(t1,"add","date","01.01.2001")
        self.ci_.alter(t1,"add","date","*.01.2001")
        self.ci_.alter(t1,"add","date","*.*.2001")
        self.ci_.alter(t1,"add","date","*.*.*")
        self.ci_.alter(t1,"add","day","sunday")
        self.ci_.alter(t1,"add","day","monday")
        self.ci_.alter(t1,"add","day","tuesday")
        self.ci_.alter(t1,"add","day","wednesday")
        self.ci_.alter(t1,"add","day","thursday")
        self.ci_.alter(t1,"add","day","friday")
        self.ci_.alter(t1,"add","day","saturday")
        self.ci_.alter(t1,"add","late","late -s +00:15 -a 20:00 -c +02:00")
    
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(self.ci_.get_defs())
        assert( len(list(task_t1.times))) == 3 ,"Expected 3 time :\n" + str(self.ci_.get_defs())
        assert( len(list(task_t1.todays))) == 3 ,"Expected 3 today's :\n" + str(self.ci_.get_defs())
        assert( len(list(task_t1.dates))) == 4 ,"Expected 4 dates :\n" + str(self.ci_.get_defs())
        assert( len(list(task_t1.days))) == 7 ,"Expected 7 days :\n" + str(self.ci_.get_defs())
        assert str(task_t1.get_late()) == "late -s +00:15 -a 20:00 -c +02:00", "Expected late 'late -s +00:15 -a 20:00 -c +02:00'" + str(self.ci_.get_defs())
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
               
    def test_client_alter_delete(self):
        test = "test_client_alter_delete"
        self.log_msg(test)
        defs = self.create_defs(test)  
         
        t1 = "/" + test + "/f1/t1"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("var","value")
        task_t1.add_variable("var1","value")
        task_t1.add_time("00:30")
        task_t1.add_time("00:31")
        task_t1.add_today("00:30")
        task_t1.add_today("00:31")
        task_t1.add_date(1,1,2001)
        task_t1.add_date(1,1,2002)
        task_t1.add_day("sunday")
        task_t1.add_day("monday")
        cron = Cron()
        cron.set_week_days( [0,1,2,3,4,5,6] )
        cron.set_time_series( "+00:30" )
        task_t1.add_cron(cron)
        task_t1.add_cron(cron)
        task_t1.add_event("event")
        task_t1.add_event("event1")
        task_t1.add_meter("meter",0,100,100)
        task_t1.add_meter("meter1",0,100,100)
        task_t1.add_label("label","name")
        task_t1.add_label("label1","name")
        task_t1.add_limit("limit",10)
        task_t1.add_limit("limit1",10)
        task_t1.add_inlimit( "limit",t1,2)
        task_t1.add_inlimit( "limit1",t1,2)
        task_t1.add_trigger( "t2 == active" )
        task_t1.add_complete( "t2 == complete" )
        
        assert task_t1.get_late() == None, "expected no late" 
        late = Late()
        late.submitted(20, 10)
        late.active(20, 10)
        late.complete(20, 10, True)
        task_t1.add_late(late)
        assert task_t1.get_late() != None, "expected late" 
        
        t2 = "/" + test + "/f1/t2"
        task_t2 = defs.find_abs_node(t2)
        task_t2.add_repeat( RepeatDate("date",20100111,20100115,2) )  # can't add cron and repeat at the same level
        
        self.ci_.load(defs)   
        self.sync_local()  
    
        self.ci_.alter(t1,"delete","variable","var")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","variable")  # delete all variables
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.variables))) == 0 ,"Expected 0 variable :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","time","00:30")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.times))) == 1 ,"Expected 1 time :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","time")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.times))) == 0 ,"Expected 0 time :\n" + str(self.ci_.get_defs())
        
        self.ci_.alter(t1,"delete","today","00:30")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.todays))) == 1 ,"Expected 1 today :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","today")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.todays))) == 0 ,"Expected 0 today :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","date","01.01.2001")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.dates))) == 1 ,"Expected 1 date :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","date")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.dates))) == 0 ,"Expected 0 date :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","day","sunday")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.days))) == 1 ,"Expected 1 day :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","day")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.days))) == 0 ,"Expected 0 day :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","event","event")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.events))) == 1 ,"Expected 1 event :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","event")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.events))) == 0 ,"Expected 0 event :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","meter","meter")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.meters))) == 1 ,"Expected 1 meter :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","meter")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.meters))) == 0 ,"Expected 0 meter :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","label","label")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.labels))) == 1 ,"Expected 1 label :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","label")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.labels))) == 0 ,"Expected 0 label :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","limit","limit")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.limits))) == 1 ,"Expected 1 limit :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","limit")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.limits))) == 0 ,"Expected 0 limit :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","inlimit","limit")
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.inlimits))) == 1 ,"Expected 1 inlimit :\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","inlimit")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.inlimits))) == 0 ,"Expected 0 inlimit :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","cron")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert( len(list(task_t1.crons))) == 0 ,"Expected 0 crons :\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t1,"delete","late")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert task_t1.get_late() == None, "expected no late after delete" 
    
    
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert task_t1.get_trigger() != None, "Expected trigger:\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","trigger")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert task_t1.get_trigger() == None, "Expected trigger to be deleted:\n" + str(self.ci_.get_defs())
    
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert task_t1.get_complete() != None, "Expected complete:\n" + str(self.ci_.get_defs())
        self.ci_.alter(t1,"delete","complete")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        assert task_t1.get_complete() == None, "Expected complete to be deleted:\n" + str(self.ci_.get_defs())
    
        self.ci_.alter(t2,"delete","repeat")   
        self.sync_local()
        task_t2 = self.ci_.get_defs().find_abs_node(t2)
        repeat = task_t2.get_repeat()
        assert repeat.empty(), "Expected repeat to be deleted:\n" + str(self.ci_.get_defs())
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
     
    def test_client_alter_change(self):
        test = "test_client_alter_change"
        self.log_msg(test)
        defs = self.create_defs(test)   
        t1 = "/" + test + "/f1/t1"
        repeat_date_path = "/" + test + "/f1/repeat_date"
        
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_variable("var","value")
        task_t1.add_variable("var1","value")
        task_t1.add_event("event")
        task_t1.add_event("event1")
        task_t1.add_meter("meter",0,100,100)
        task_t1.add_meter("meter1",0,100,100)
        task_t1.add_label("label","name")
        task_t1.add_label("label1","name1")
        task_t1.add_limit("limit",10)
        task_t1.add_limit("limit1",10)
        task_t1.add_inlimit( "limit",t1,2)
        task_t1.add_inlimit( "limit1",t1,2)
        task_t1.add_trigger( "t2 == active" )
        task_t1.add_complete( "t2 == complete" )
        late = Late()
        late.submitted(20, 10)
        late.active(20, 10)
        late.complete(20, 10, True)
        task_t1.add_late(late)
                
        f1 = defs.find_abs_node("/" + test + "/f1")
        repeat_date = f1.add_task("repeat_date")
        repeat_date.add_repeat( RepeatDate("date",20100111,20100115,2) )  # can't add cron and repeat at the same level
               
        self.ci_.load(defs)   
        self.sync_local()  
    
        self.ci_.alter(t1,"change","late","-s +10:00")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        variable = task_t1.get_late()
        assert str(task_t1.get_late()) == "late -s +10:00", "Expected alter of late to be 'late -s +10:00' but found " + str(task_t1.get_late())
    
        self.ci_.alter(t1,"change","variable","var","changed_var")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        variable = task_t1.find_variable("var")
        assert variable.value() == "changed_var", "Expected alter of variable to be 'change_var' but found " + variable.value()
    
        self.ci_.alter(t1,"change","meter","meter","10")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        meter = task_t1.find_meter("meter")
        assert meter.value() == 10, "Expected alter of meter to be 10 but found " + str(meter.value())
    
        self.ci_.alter(t1,"change","event","event","set")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        event = task_t1.find_event("event")
        assert event.value() == True, "Expected alter of event to be set but found " + str(event.value())
    
        self.ci_.alter(t1,"change","trigger","t2 == aborted")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        trigger = task_t1.get_trigger()
        assert trigger.get_expression() == "t2 == aborted", "Expected alter of trigger to be 't2 == aborted' but found " + trigger.get_expression()
    
        self.ci_.alter(t1,"change","trigger","/test_client_alter_change/f1/t2 == complete")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        trigger = task_t1.get_trigger()
        assert trigger.get_expression() == "/test_client_alter_change/f1/t2 == complete", "Expected alter of trigger to be '/test_client_alter_change/f1/t2 == complete' but found " + trigger.get_expression()
    
        self.ci_.alter(t1,"change","complete","t2 == aborted")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        complete = task_t1.get_complete()
        assert complete.get_expression() == "t2 == aborted", "Expected alter of complete to be 't2 == aborted' but found " + complete.get_expression()
    
        self.ci_.alter(t1,"change","complete","/test_client_alter_change/f1/t2 == active")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        complete = task_t1.get_complete()
        assert complete.get_expression() == "/test_client_alter_change/f1/t2 == active", "Expected alter of complete to be '/test_client_alter_change/f1/t2 == active' but found " + complete.get_expression()
    
        self.ci_.alter(t1,"change","limit_max","limit", "2")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        limit = task_t1.find_limit("limit")
        assert limit != None, "Expected to find limit"
        assert limit.limit() == 2, "Expected alter of limit_max to be 2 but found " + str(limit.limit())
    
        self.ci_.alter(t1,"change","limit_value","limit", "2")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        limit = task_t1.find_limit("limit")
        assert limit != None, "Expected to find limit"
        assert limit.value() == 2, "Expected alter of limit_value to be 2 but found " + str(limit.value())
    
        self.ci_.alter(t1,"change","label","label","new-value")   
        self.sync_local()
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        label = task_t1.find_label("label")
        assert label.new_value() == "new-value", "Expected alter of label to be 'new-value' but found " + label.new_value()
        
        self.ci_.alter(repeat_date_path,"change","repeat","20100113")   
        self.sync_local()
        task = self.ci_.get_defs().find_abs_node(repeat_date_path)
        repeat = task.get_repeat()
        assert repeat.value() == 20100113, "Expected alter of repeat to be 20100113 but found " + str(repeat.value())
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
     
    def test_client_alter_flag(self):
        test = "test_client_alter_flag"
        self.log_msg(test)
        defs = self.create_defs(test)   
        t1 = "/" + test + "/f1/t1"
         
        task_t1 = defs.find_abs_node(t1)
               
        self.ci_.load(defs)   
        self.sync_local()  
    
        flag = Flag()
        flag_list = flag.list() # flag_list is of type FlagTypeVec
        for flg in flag_list: 
            self.ci_.alter(t1,"set_flag",flag.type_to_string(flg) )   
            self.sync_local()
            task_t1 = self.ci_.get_defs().find_abs_node(t1)
            task_flag = task_t1.get_flag()
            assert task_flag.is_set( flg ),"expected flag %r to be set" % task_flag.type_to_string(flg)
    
            # alter itself causes the flag message to be set, and preserved
            if flg == FlagType.message: continue 
            
            self.ci_.alter(t1,"clear_flag",flag.type_to_string(flg) )   
            self.sync_local()
            task_t1 = self.ci_.get_defs().find_abs_node(t1)
            task_flag = task_t1.get_flag()
            assert not task_flag.is_set( flg ),"expected flag %r NOT to be set" % task_flag.type_to_string(flg)
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
    
    def test_client_flag_migrated(self):
        # ENABLE for ecflow 4.5.0
        return
        test = "test_client_flag_migrated"
        self.log_msg(test)
        
        defs = self.create_defs(test)   
        s1 = "/" + test  
       
        self.ci_.load(defs)   
        self.sync_local()
     
        suite = defs.find_suite(test)
        node_vec = suite.get_all_nodes()
        assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))
     
        self.ci_.alter(s1,"set_flag","migrated")   
        self.sync_local()
        suite = defs.find_suite(test)
        node_vec = suite.get_all_nodes()
        assert len(node_vec) == 1, "Expected 1 nodes, but found " + str(len(node_vec))
     
        self.ci_.checkpt()  # checkpoint after setting flag migrated, need to prove nodes still persisted
         
        self.ci_.alter(s1,"clear_flag","migrated")   
        self.sync_local()
        suite = defs.find_suite(test)
        node_vec = suite.get_all_nodes()
        assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
     
        # ISSUES:
        # o Currently we can only change clock attr if we have one.
        # o Even when we have a clock attr, it only makes sense to apply clock attr changes
        #   before begin(), i.e how do we apply change in gain after begin ??
        
        #" change clock-type name        # The name must be one of 'hybrid' or 'real'.\n"
        #       " change clock-gain name        # The gain must be convertible to an integer.\n"
        #        " change label name value       # sets the label\n"
        #        " change repeat value           # If the repeat is a date, then the value must be a valid YMD ( ie. yyyymmdd)\n"
        #        "                               # and be convertible to an integer, additionally the value must be in range\n"
        #        "                               # of the repeat start and end dates. Like wise for repeat integer. For repeat\n"
        #         "                               # string and enum,  the name must either be an integer, that is a valid index or\n"
        #         "                               # if it is a string, it must correspond to one of enum's or strings list\n"
    
    def test_client_force(self):
        test = "test_client_force"
        self.log_msg(test)
        defs = self.create_defs(test) 
         
        path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]       
        t1 = path_list[0]      
        for path in path_list:
            task = defs.find_abs_node(path)
            assert task != None, "Expected to find task at path " + path
            task.add_event("event")
    
        self.ci_.load(defs)   
        self.sync_local()  
        
        state_list = [ State.unknown, State.active, State.complete, State.submitted, State.aborted, State.queued ]
        for state in state_list:
            self.ci_.force_state(t1,state) 
            self.sync_local()
            task = self.ci_.get_defs().find_abs_node(t1)
            assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())      
        for state in state_list:
            self.ci_.force_state( path_list,state) 
            self.sync_local()
            for path in path_list:
                task = self.ci_.get_defs().find_abs_node(path)
                assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())     
     
        for state in state_list:
            self.ci_.force_state_recursive("/" + test,state) 
            self.sync_local()
            task = self.ci_.get_defs().find_abs_node(t1)
            assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())
        suite_paths = [ "/" + test ]
        for state in state_list:
            self.ci_.force_state_recursive( suite_paths,state) 
            self.sync_local()
            task = self.ci_.get_defs().find_abs_node(t1)
            assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())           
        
        event_states = [ "set", "clear" ]
        for ev_state in event_states:
            for path in path_list:
                self.ci_.force_event(path + ":event" , ev_state)
                self.sync_local()
                task = self.ci_.get_defs().find_abs_node(path)
                event_fnd = False
                for event in task.events:
                    event_fnd = True
                    if ev_state == "set" :  assert event.value() == True  ," Expected event value to be set"
                    else:                   assert event.value() == False ," Expected event value to be clear"
                assert event_fnd == True," Expected event to be found"
    
        event_path_list = [ "/" + test + "/f1/t1:event", "/" + test + "/f1/t2:event" ]       
        event_states = [ "set", "clear" ]
        for ev_state in event_states:
            self.ci_.force_event( event_path_list , ev_state)
            self.sync_local()
            for path in path_list:
                task = self.ci_.get_defs().find_abs_node(path)
                event_fnd = False
                for event in task.events:
                    event_fnd = True
                    if ev_state == "set" :  assert event.value() == True  ," Expected event value to be set"
                    else:                   assert event.value() == False ," Expected event value to be clear"
                assert event_fnd == True," Expected event to be found"
    
        self.ci_.suspend("/" + test  ) # stop  downstream test from re-starting this
          
    def test_client_replace(self,on_disk):
        test = "test_client_replace"
        if on_disk:
            test = "test_client_replace_on_disk"
        self.log_msg(test)
        self.ci_.log_msg(str(on_disk))
         
        # Create and load the following defs
        # s1
        #   f1
        #     t1
        #     t2
        self.ci_.load(self.create_defs(test))  
        self.sync_local()  
        
        #===============================================================================
        # Example of using replace to ADD a *NEW* node hierarchy to an existing suite
        # we should end up with:
        # s1
        #   f1
        #     t1
        #     t2
        #   f2
        #     t1
        client_def = self.create_defs(test)
        client_def.find_suite(test).add_family("f2").add_task("t1")
        if on_disk:
            client_def.save_as_defs(test + ".def")
            client_def = test + ".def"
        
        self.ci_.replace("/" + test + "/f2",client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
        self.sync_local()  
        assert self.ci_.get_defs().find_abs_node("/" + test + "/f2/t1") != None, "Expected to find task /" + test + "/f2/t1\n" + str(self.ci_.get_defs())
    
        # Example of using replace to *REMOVE* node hierarchy to an existing suite, could have used delete
        assert self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1") != None, "Expected to find task /" + test + "/f1/t1\n" + str(self.ci_.get_defs())
        assert self.ci_.get_defs().find_abs_node("/" + test + "/f2/t1") != None, "Expected to find task /" + test + "/f2/t1\n" + str(self.ci_.get_defs())
        client_def = Defs()
        client_def.add_suite(test)    # should only have the suite
        if on_disk:
            client_def.save_as_defs(test + ".def")
            client_def = test + ".def"
    
        self.ci_.replace("/" + test,client_def)   
        self.sync_local()  
        assert self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1") == None, "Expected NOT to find task /" + test + "/f1/t1\n" + str(self.ci_.get_defs())
        assert self.ci_.get_defs().find_abs_node("/" + test + "/f2/t1") == None, "Expected NOT to find task /" + test + "/f2/t1\n" + str(self.ci_.get_defs())
    
        #===============================================================================
        # Example of using replace to add a *NEW* suite
        test2 = test + "2";
        client_def = Defs();
        client_def.add_suite(test2)
        if on_disk:
            client_def.save_as_defs(test + ".def")
            client_def = test + ".def"
    
        self.ci_.replace("/" + test2,client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
        self.sync_local()  
        suite = self.ci_.get_defs().find_suite(test2)
        assert suite != None ,"Expected to find suite :" + test2 + "\n" + str(self.ci_.get_defs())
               
        # replace added suite s2 with a new s2 which has a task, 
        # s2 must exist on the client defs
        client_def = Defs();
        client_def.add_suite(test2).add_task("t1")
        if on_disk:
            client_def.save_as_defs(test + ".def")
            client_def = test + ".def"
    
        self.ci_.replace("/" + test2,client_def) 
        self.sync_local()  
            
        assert self.ci_.get_defs().find_abs_node("/" + test2 + "/t1") != None, "Expected to find task /" + test2 + "/t1\n" + str(self.ci_.get_defs())
        if on_disk:
            os.remove(client_def)
    
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
        self.ci_.suspend("/" + test2  ) # stop  downstream test from re-starting this
        self.sync_local()  
    
    def test_client_suspend(self):
        test = "test_client_suspend"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_variable("ECF_DUMMY_TASK","")
            
        self.ci_.load(defs)  
        self.sync_local()  
        self.ci_.begin_suite(test)  
        
        self.ci_.suspend("/" + test )  
               
        self.sync_local();
        suite = self.ci_.get_defs().find_suite(test)
        assert suite.is_suspended(), "Expected to find suite suspended"
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
        
    def test_client_suspend_multiple_paths(self):
        test = "test_client_suspend_multiple_paths"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_variable("ECF_DUMMY_TASK","")
        
        self.ci_.load(defs)  
        self.ci_.begin_suite(test)  
        self.sync_local()  
        
        path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
        self.ci_.suspend( path_list )  
               
        self.sync_local();
        task_t1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task_t2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
        assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
                
    def test_client_resume(self):
        test = "test_client_resume"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_variable("ECF_DUMMY_TASK","")
            
        self.ci_.load(defs)  
        self.ci_.begin_suite(test)  
        self.sync_local()  
        
        self.ci_.suspend("/" + test )  
        self.sync_local();
        suite = self.ci_.get_defs().find_suite(test)
        assert suite.is_suspended(), "Expected to find suite suspended"
        
        self.ci_.resume("/" + test )  
        self.sync_local();
        suite = self.ci_.get_defs().find_suite(test)
        assert suite.is_suspended() == False, "Expected to find suite resumed"
    
        # suspend to stop down stream tests from restarting this test
        self.ci_.suspend("/" + test )  
    
    def test_client_resume_multiple_paths(self):
        test = "test_client_resume_multiple_paths"
        self.log_msg(test)
        defs = self.create_defs(test)  
        suite = defs.find_suite(test)
        suite.add_variable("ECF_DUMMY_TASK","")
            
        self.ci_.load(defs)  
        self.ci_.begin_suite(test)  
        self.sync_local()  
        
        path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
        self.ci_.suspend( path_list )  
      
        self.sync_local();
        task_t1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task_t2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
        assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"
    
        self.ci_.resume( path_list )  
        self.sync_local();
        task_t1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task_t2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task_t1.is_suspended() == False, "Expected to find task t1 to be resumed"
        assert task_t2.is_suspended() == False, "Expected to find task t2 to be resumed"
        
         # suspend to stop down stream tests from restarting this test
        self.ci_.suspend("/" + test )  
      
    def test_client_delete_node(self): 
        test = "test_client_delete_node"
        self.log_msg(test)
        defs = self.create_defs(test)
        
        task_vec = defs.get_all_tasks();
        assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)
    
        self.ci_.load(defs)  
        self.sync_local();
        for task in task_vec:
            node = self.ci_.get_defs().find_abs_node(task.get_abs_node_path()) 
            assert node != None , "Expected to find task " + task.get_abs_node_path()  + ":\n"  + str(self.ci_.get_defs()) 
    
        for task in task_vec:
            self.ci_.delete(task.get_abs_node_path())
    
        self.sync_local();
        for task in task_vec:
            node = self.ci_.get_defs().find_abs_node(task.get_abs_node_path()) 
            assert node == None , "Expected not to find task " + task.get_abs_node_path()  + " as it should have been deleted:\n" + str(self.ci_.get_defs())   
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
        
    def test_client_delete_node_multiple_paths(self): 
        test = "test_client_delete_node_multiple_paths"
        self.log_msg(test)
        defs = self.create_defs(test)
        
        task_vec = defs.get_all_tasks();
        assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)
    
        paths = []
        for task in task_vec:
            paths.append(task.get_abs_node_path()) 
     
        self.ci_.load(defs)  
        
        self.sync_local();
        for task in task_vec:
            node = self.ci_.get_defs().find_abs_node(task.get_abs_node_path()) 
            assert node != None , "Expected to find task " + task.get_abs_node_path()  + ":\n"  + str(self.ci_.get_defs()) 
    
        self.ci_.delete(paths)
    
        self.sync_local();
        for task in task_vec:
            node = self.ci_.get_defs().find_abs_node(task.get_abs_node_path()) 
            assert node == None , "Expected not to find task " + task.get_abs_node_path()  + " as it should have been deleted:\n" + str(self.ci_.get_defs())   
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this    
    
    def test_client_check_defstatus(self):            
        test = "test_client_check_defstatus"
        self.log_msg(test)
        defs = self.create_defs(test)  
        
        # stop defs form running when begin is called.
        suite = defs.find_suite(test)
        suite.add_defstatus(DState.suspended)
    
        t1 = "/" + test + "/f1/t1"
        t2 = "/" + test + "/f1/t2"
        task_t1 = defs.find_abs_node(t1)
        task_t1.add_defstatus(DState.suspended)
        
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        job_ctrl.set_node_path("/" + test)
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
        
        self.ci_.restart_server()
        self.ci_.load(defs)           
        self.ci_.begin_suite(test)
         
        self.sync_local() # get the changes, synced with local defs
        #print self.ci_.get_defs();
        task_t1 = self.ci_.get_defs().find_abs_node(t1)
        task_t2 = self.ci_.get_defs().find_abs_node(t2)
        assert task_t1 != None,"Could not find t1"
        assert task_t2 != None,"Could not find t2"
      
        assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
        assert task_t2.get_state() == State.queued, "Expected state queued " + str(task_t2.get_state())
    
        assert task_t1.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t1.get_state())
        assert task_t2.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t2.get_state())
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
       
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
        
    def test_ECFLOW_189(self):
        # Bug, when a node is resumed it ignored holding dependencies higher up the tree.
        # i.e Previously when we resumed a node, it ignored trigger/time/node state, dependencies higher up the tree
        test = "test_ECFLOW_189"
        self.log_msg(test)
        defs = self.create_defs(test)  
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
        
        self.ci_.restart_server()
        self.ci_.load(defs)   
        self.sync_local()  
        
        self.ci_.suspend("/" + test  )
        self.ci_.suspend("/" + test + "/f1/t1")
        self.ci_.suspend("/" + test + "/f1/t2")
            
        self.ci_.begin_suite(test)
        
        self.sync_local() # get the changes, synced with local defs
        #print self.ci_.get_defs();
        task_t1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task_t2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task_t1 != None,"Could not find /" + test + "/f1/t1"
        assert task_t2 != None,"Could not find /" + test + "/f1/t2"
      
        assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
        assert task_t2.get_state() == State.queued, "Expected state queued but found " + str(task_t2.get_state())
        assert task_t1.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t1.get_dstate())
        assert task_t2.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t2.get_dstate())
    
        # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
        self.ci_.resume("/" + test + "/f1/t1")
        self.ci_.resume("/" + test + "/f1/t2")
         
        self.sync_local() # get the changes, synced with local defs
        #print self.ci_.get_defs();
        task_t1 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t1")
        task_t2 = self.ci_.get_defs().find_abs_node("/" + test + "/f1/t2")
        assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
        assert task_t2.get_state() == State.queued, "Expected state queued but found " + str(task_t2.get_state())
        assert task_t1.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t1.get_dstate())
        assert task_t2.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t2.get_dstate())
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
    
        dir_to_remove = self.ecf_home() + "/" + "test_ECFLOW_189"
        shutil.rmtree(dir_to_remove)      
    
    
    def test_ECFLOW_199(self):
        # Test ClientInvoker::changed_node_paths
        test = "test_ECFLOW_199"
        self.log_msg(test)
        defs = self.create_defs(test)  
        defs.generate_scripts();
        
        job_ctrl = JobCreationCtrl()
        defs.check_job_creation(job_ctrl)       
        assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
        
        self.ci_.restart_server()
        self.ci_.load(defs)   
        self.sync_local() # get the changes, synced with local defs
        
        self.ci_.suspend("/" + test  )
        self.ci_.suspend("/" + test + "/f1/t1")
        self.ci_.suspend("/" + test + "/f1/t2")
        self.sync_local() # get the changes, synced with local defs
    
        self.ci_.begin_suite(test)
        
        self.sync_local() # get the changes, synced with local defs
        #print self.ci_.get_defs();
        assert len(list(self.ci_.changed_node_paths)) == 0, "Expected first call to sync_local, to have no changed paths but found " + str(len(list(self.ci_.changed_node_paths)))
        
        # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
        self.ci_.resume("/" + test + "/f1/t1")
        self.sync_local() 
        if len(list(self.ci_.changed_node_paths)) != 1:
            for path in self.ci_.changed_node_paths:
                print("   changed node path " + path);
            assert False, "Expected 1 changed path but found " + str(len(list(self.ci_.changed_node_paths)))
    
        self.ci_.resume("/" + test + "/f1/t2")
        self.sync_local() 
        if len(list(self.ci_.changed_node_paths)) != 1:
            for path in self.ci_.changed_node_paths:
                print("   changed node path " + path);
            assert False, "Expected 1 changed path but found " + str(len(list(self.ci_.changed_node_paths)))
            
        self.ci_.suspend("/" + test  )  # stop  downstream test from re-starting this
        dir_to_remove = self.ecf_home() + "/" + test
        shutil.rmtree(dir_to_remove)      
    
    def test_gui(self):   
        self.ci_.delete_all() # start fresh
        self.test_version()
        PrintStyle.set_style( Style.STATE ) # show node state 
        self.test_client_get_server_defs()             
                 
        self.test_client_restart_server()             
        self.test_client_halt_server()             
        self.test_client_shutdown_server()   
             
        self.test_client_load_in_memory_defs()             
        self.test_client_load_from_disk()             
        self.test_client_checkpt()             
        self.test_client_restore_from_checkpt()             
                  
        self.test_client_reload_wl_file()             
          
        self.test_client_run()  
        self.test_client_run_with_multiple_paths()     
        self.test_client_requeue()             
        self.test_client_requeue_with_multiple_paths()             
        self.test_client_free_dep()              
         
        self.test_client_suites()
        self.test_client_ch_suites()  
        self.test_client_ch_register()             
        self.test_client_ch_drop()             
        self.test_client_ch_drop_user()             
        self.test_client_ch_add()             
        self.test_client_ch_auto_add()             
        self.test_client_ch_remove()             
                    
        self.test_client_get_file()             
        self.test_client_alter_add() 
        self.test_client_alter_delete() 
        self.test_client_alter_change() 
        self.test_client_alter_flag() 
        self.test_client_flag_migrated() 
       
        self.test_client_force()             
        self.test_client_replace(False)             
        self.test_client_replace(True)             
         
        self.test_client_suspend()             
        self.test_client_suspend_multiple_paths()             
        self.test_client_resume()             
        self.test_client_resume_multiple_paths()             
        self.test_client_delete_node()             
        self.test_client_delete_node_multiple_paths()             
         
        self.test_client_check()  
        self.test_client_check_defstatus()  
         
        self.test_ECFLOW_189()         
        self.test_ECFLOW_199()         

if __name__ == "__main__":
    
    DESC = """Will run various tests on the server. These will be used to test the GUI
              Usage:
                Example1: List all the server variables
                   TestGui.py --host cca --port 4141 --time <sec> /
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default="localhost",   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default="3141",   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--time', default=0,   
                        help="How long to run the tests in seconds. default is 0, which is one test loop")
    PARSER.add_argument('--sync_sleep', type=int,default=4,   
                        help="Time to wait after sync_local.Allow GUI to refresh. Set to 0 for debug.")
    ARGS = PARSER.parse_args()
    print ARGS   
     
    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    tester = Tester(CL,ARGS)
    try:
        CL.ping() 
        
        count = 1
        start_time = time.time()
        while True:
            tester = Tester(CL,ARGS)
            tester.test_gui()
            elapsed = int(time.time() - start_time)
            print "======================================================"
            print "elapsed time :",elapsed, "s loop:",count
            print "======================================================"
            count += 1
            if elapsed > int(ARGS.time):
                break
                
    except RuntimeError, ex:
        print "Error: " + str(ex)
        print "Check host and port number are correct."

    tester.clean_up_server()
    tester.clean_up_data()
