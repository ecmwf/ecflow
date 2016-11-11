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

def ecf_includes() :  return os.getcwd() + "Pyext/test/data/includes"

def ecf_home(port): 
    # debug_build() is defined for ecflow. Used in test to distinguish debug/release ecflow
    # Vary ECF_HOME based on debug/release/port allowing multiple invocations of these tests
    if debug_build():
        return os.getcwd() + "/test_gui/data/ecf_home_debug_" + str(port)
    return os.getcwd() + "/test_gui/data/ecf_home_release_" + str(port)

def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"
def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"
def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"
def white_list_file_path(port): return "./" + gethostname() + "." + port + ".ecf.lists"

def clean_up_server(ci):
    log_msg(ci,"   clean_up " + ci.get_port())
    #try: os.remove(log_file_path(port))
    #except: pass
    try: os.remove(checkpt_file_path(ci.get_port()))
    except: pass
    try: os.remove(backup_checkpt_file_path(ci.get_port()))
    except: pass
    try: os.remove(white_list_file_path(ci.get_port()))  
    except: pass
    
def clean_up_data(ci):
    log_msg(ci,"   Attempting to Removing ECF_HOME " + ecf_home(ci.get_port()))
    try: 
        shutil.rmtree(ecf_home(ci.get_port()),True)   # True means ignore errors
        shutil.rmtree("test_gui",True)   # True means ignore errors 
        print("   Remove OK") 
    except: 
        print("   Remove Failed") 
        pass

def path_to_ecflow_client(ci):
    # to use the build, in preference, to release build
    path_to_client = File.find_client()
    if os.path.exists(path_to_client):
        return path_to_client
    return "/usr/local/apps/ecflow/" + ci.version() + "/bin/ecflow_client"


def create_defs(ci,name=""):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    suite = defs.add_suite(suite_name);
    
    ecfhome = ecf_home(the_port);
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("ECF_CLIENT_EXE_PATH", path_to_ecflow_client(ci));
    suite.add_variable("SLEEP", "1");  # not strictly required since default is 1 second
    suite.add_variable("ECF_INCLUDE", ecf_includes());

    family = suite.add_family("f1")
    family.add_task("t1")
    family.add_task("t2")
    return defs;
    
def log_msg(ci,msg):
    print(msg)
    ci.log_msg("======================== " + msg + " ========================")

def test_version(ci):
    log_msg(ci,"test_version")
    client_version = ci.version();
    server_version = ci.server_version();
    assert client_version == server_version, "Expected client version(" + client_version +") and server version(" +  server_version + ") to match\n";
    
def test_client_get_server_defs(ci):
    test = "test_client_get_server_defs"
    log_msg(ci,test)
    ci.load(create_defs(ci,test))  
    ci.get_server_defs() 
    assert ci.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(ci.get_defs())
    
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
    ci.sync_local() # clear changed_node_paths 

def test_client_restart_server(ci):
    log_msg(ci,"test_client_restart_server")
    ci.restart_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.RUNNING, "Expected server to be running"
    
    paths = list(ci.changed_node_paths)
    #for path in ci.changed_node_paths:
    #    print("   changed node path " + path);
    assert len(paths) == 1, "expected changed node to be the root node"
    assert paths[0] == "/", "Expected root path but found " + str(paths[0])

def test_client_halt_server(ci):
    log_msg(ci,"test_client_halt_server")
    ci.halt_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.HALTED, "Expected server to be halted"
    
    paths = list(ci.changed_node_paths)
    #for path in ci.changed_node_paths:
    #    print("   changed node path " + path);
    assert len(paths) == 1, "expected changed node to be the root node"
    assert paths[0] == "/", "Expected root path but found " + str(paths[0])
    ci.restart_server()   

def test_client_shutdown_server(ci):
    log_msg(ci,"test_client_shutdown_server")
    ci.shutdown_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.SHUTDOWN, "Expected server to be shutdown"
    
    paths = list(ci.changed_node_paths)
    #for path in ci.changed_node_paths:
    #    print("   changed node path " + path);
    assert len(paths) == 1, "expected changed node to be the root node"
    assert paths[0] == "/", "Expected root path but found " + str(paths[0])

def test_client_load_in_memory_defs(ci):
    test = "test_client_load_in_memory_defs"
    log_msg(ci,test)
    ci.load(create_defs(ci,test))  
    ci.sync_local() 
    assert ci.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(ci.get_defs())              
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this


def test_client_load_from_disk(ci):            
    test = "test_client_load_from_disk"
    log_msg(ci,test)
    defs = create_defs(ci,test);
    defs_file = test + ".def"
    defs.save_as_defs(defs_file)     
    assert os.path.exists(defs_file), "Expected file " + defs_file + " to exist after defs.save_as_defs()"
    ci.load(defs_file) # open and parse defs file, and load into server.\n"
        
    # check load worked
    ci.sync_local() 
    assert ci.get_defs().find_suite(test) != None, "Expected to find suite of name " + test + ":\n" + str(ci.get_defs())
    os.remove(defs_file)
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

def test_client_checkpt(ci, port):
    test = "test_client_checkpt"
    log_msg(ci,test)
    try:    
        os.remove(checkpt_file_path(port))
        os.remove(backup_checkpt_file_path(port))
    except: pass
    
    ci.load(create_defs(ci,test))  
    ci.checkpt()
    assert os.path.exists(checkpt_file_path(port)), "Expected check pt file " + checkpt_file_path(port) + " to exist after ci.checkpt()"
    assert os.path.exists(backup_checkpt_file_path(port)) == False, "Expected back up check pt file to *NOT* exist"
    
    ci.checkpt()   # second check pt should cause backup check pt to be written
    assert os.path.exists(backup_checkpt_file_path(port)), "Expected back up check pt file to exist after second ci.checkpt()"

    ci.checkpt(CheckPt.NEVER)         # switch of check pointing
    ci.checkpt(CheckPt.ALWAYS)        # always check point, at any state change
    ci.checkpt(CheckPt.ON_TIME)       # Check point periodically, by interval set in server
    ci.checkpt(CheckPt.ON_TIME, 200)  # Check point periodically, by interval set in server
    ci.checkpt(CheckPt.UNDEFINED, 0, 35)  # Change check point save time alarm

    os.remove(checkpt_file_path(port))
    os.remove(backup_checkpt_file_path(port))
    
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

def test_client_restore_from_checkpt(ci, port):          
    test = "test_client_restore_from_checkpt"
    log_msg(ci,test)
    try:    
        os.remove(checkpt_file_path(port))
        os.remove(backup_checkpt_file_path(port))
    except: pass
    
    ci.load(create_defs(ci,test))  
    ci.checkpt()
    ci.delete_all() 
    
    ci.sync_local() 
    assert len(list(ci.get_defs().suites)) == 0, "Expected all suites to be delete:\n"
    
    ci.halt_server()  # server must be halted, otherwise restore_from_checkpt will throw
    ci.restore_from_checkpt()
    
    ci.sync_local() 
    assert ci.get_defs().find_suite(test) != None, "Expected to find suite " + test + " after restore from checkpt:\n" + str(ci.get_defs())

    os.remove(checkpt_file_path(port))
    ci.restart_server()   
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

def get_username(): return pwd.getpwuid(os.getuid())[ 0 ]

def test_client_reload_wl_file(ci, port):
    test = "test_client_reload_wl_file"
    log_msg(ci,test)
    
    expected = False
    try:    ci.reload_wl_file();            
    except: expected = True
    assert expected, "Expected reload to fail when no white list specified"
    
    # create a white list file
    wl_file = open(white_list_file_path(port), 'w')
    wl_file.write("#\n")
    wl_file.write("4.4.14   #   comment\n\n")
    wl_file.write("# These user have read and write access to the server\n")
    wl_file.write(get_username() + "\n")  # add current user otherwise remaining test's, wont have access from server anymore
    wl_file.write("axel  # admin\n")
    wl_file.write("john  # admin\n\n")
    wl_file.write("# Read only users\n")
    wl_file.write("-fred   # needs read access only\n")
    wl_file.write("-joe90  # needs read access only\n")
    wl_file.close();
    
    ci.reload_wl_file();  
    os.remove(white_list_file_path(port))          


def test_client_run(ci):            
    test = "test_client_run"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_defstatus(DState.suspended)

    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    ci.run("/" + test, False)
    
    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        suite = ci.get_defs().find_suite(test)
        assert suite != None, "Expected to find suite " + test + ":\n" + str(ci.get_defs())
        if suite.get_state() == State.complete:
            break;
        time.sleep(3)
        if count > 20:
            assert False, "test_client_run aborted after " + str(count) + " loops:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      

def test_client_run_with_multiple_paths(ci):            
    test = "test_client_run_with_multiple_paths"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_defstatus(DState.suspended)

    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2"]
    ci.run( path_list, False)

    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        suite = ci.get_defs().find_suite(test)
        assert suite != None, "Expected to find suite " + tests + ":\n" + str(ci.get_defs())
        if suite.get_state() == State.complete:
            break;
        time.sleep(3)
        if count > 20:
            assert False, test + " aborted after " + str(count) + " loops:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
   
    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      

def test_client_requeue(ci):
    test = "test_client_requeue"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_defstatus(DState.suspended)
     
    defs.generate_scripts();
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    ci.force_state_recursive("/" + test,State.unknown)
    ci.sync_local();
    suite = ci.get_defs().find_suite(test)
    assert suite.get_state() == State.unknown, "Expected to find suite with state unknown"

    ci.requeue("/" + test)
    ci.sync_local();
    suite = ci.get_defs().find_suite(test)
    assert suite.get_state() == State.queued, "Expected to find suite with state queued"
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      

def test_client_requeue_with_multiple_paths(ci):
    test = "test_client_requeue_with_multiple_paths"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_defstatus(DState.suspended)
     
    defs.generate_scripts();
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    ci.force_state_recursive("/" + test,State.unknown)
    ci.sync_local();
    task1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task1.get_state() == State.unknown, "Expected to find t1 with state unknown"
    assert task2.get_state() == State.unknown, "Expected to find t2 with state unknown"

    path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
    ci.requeue( path_list)
    ci.sync_local();
    task1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task1.get_state() == State.queued, "Expected to find task t1 with state queued"
    assert task2.get_state() == State.queued, "Expected to find task t2 with state queued"
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      

def test_client_free_dep(ci):
    test = "test_client_free_dep"
    log_msg(ci,test)
       
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
    ecfhome = ecf_home(the_port);
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("ECF_CLIENT_EXE_PATH", path_to_ecflow_client(ci));
    suite.add_variable("SLEEPTIME", "1");
    suite.add_variable("ECF_INCLUDE", ecf_includes());
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
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    t1_path = "/" + test + "/f1/t1"
    t2_path = "/" + test + "/f1/t2"
    t3_path = "/" + test + "/f1/t3"
    t4_path = "/" + test + "/f1/t4"
    while 1:
        ci.sync_local()
        t1 = ci.get_defs().find_abs_node(t1_path)
        t2 = ci.get_defs().find_abs_node(t2_path)
        t3 = ci.get_defs().find_abs_node(t3_path)
        t4 = ci.get_defs().find_abs_node(t4_path)
 
        if t1.get_state() == State.queued: ci.free_time_dep(t1_path)
        if t2.get_state() == State.queued: ci.free_date_dep(t2_path)
        if t3.get_state() == State.queued: ci.free_trigger_dep(t3_path)
        if t4.get_state() == State.queued: ci.free_all_dep(t4_path)

        suite = ci.get_defs().find_suite(test)
        if suite.get_state() == State.complete:
            break;
        time.sleep(3)       
            
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)             


def test_client_check(ci):
    test = "test_client_check"
    log_msg(ci,test)
    
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
    ci.load(defs)
    server_check = ci.check("") # empty string means check the whole defs, otherwise a node path can be specified.
    server_check = ci.check("/" + test)  
    # print server_check
    assert len(server_check) > 0, "Expected defs to fail, since no externs in server "
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
    
def test_client_suites(ci):
    test = "test_client_suites"
    log_msg(ci,test)
    no_of_suites = len(ci.suites())

    defs = create_defs(ci,"test_client_suites")
    ci.load(defs)  
    assert len(ci.suites()) == no_of_suites + 1 ,"expected " + str(no_of_suites + 1) + " suites"
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
    
def test_client_ch_suites(ci):
    test = "test_client_ch_suites"
    log_msg(ci,test)
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    suite_names = [ test + '1', test + '2', test + '3' ]
    ci.ch_register(True,suite_names)      # register interest in suites s1,s2,s3 and any new suites
    ci.ch_register(False,[ test + "1"])   # register interest in suites s1 
    ci.sync_local();
    
    ci.ch_suites()  # writes to standard out, list of suites and handles
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();    

def test_client_ch_register(ci):
    test = "test_client_ch_register"
    log_msg(ci,"test_client_ch_register")
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite( test + str(i))
    ci.load(defs)
    ci.sync_local();
    
    suite_names = [ test + '1', test + '2', test + '3' ]
    ci.ch_register(True, suite_names)    # register interest in suites s1,s2,s3 and any new suites
    ci.ch_register(False,suite_names)    # register interest in suites s1,s2,s3 only
    ci.sync_local();
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
            
def test_client_ch_drop(ci):
    test = "test_client_ch_drop"
    log_msg(ci,test)
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    try:
        # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ test + '1', test + '2', test + '3' ]
        ci.ch_register(True, suite_names)    
        ci.sync_local();
    finally:  
        ci.ch_drop()  # drop using handle stored in ci., from last register
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();
          
def test_client_ch_drop_user(ci):
    test = "test_client_ch_drop_user"
    log_msg(ci,test)
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    try:
        # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ test + '1', test + '2', test + '3' ]
        ci.ch_register(True, suite_names)
        ci.sync_local();
    except RuntimeError as e:
        print(str(e))
    
    ci.ch_drop_user("")  # drop all handle associated with current user
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();
            
def test_client_ch_add(ci):
    test = "test_client_ch_add"
    log_msg(ci,test)
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    try:
        suite_names = []
        ci.ch_register(True,suite_names)        # register interest in any new suites
        suite_names = [ test + '1', test + '2' ]
        ci.ch_add(suite_names)                  # add suites s1,s2 to the last added handle
        suite_names = [ test + '3', test + '4' ]
        ci.ch_add( ci.ch_handle(),suite_names)  # add suites s3,s4 using last handle
    except RuntimeError as e:
        print(str(e))
        
    ci.ch_drop_user("")  # drop all handle associated with current user
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();
            
def test_client_ch_auto_add(ci):
    test = "test_client_ch_auto_add"
    log_msg(ci,test)
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    try:
        suite_names = [ test + '1', test + '2', test + '3' ]
        ci.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
        ci.ch_auto_add( False )                 # disable adding newly created suites to last registered handle\n"
        ci.ch_auto_add( True )                  # enable adding newly created suites to last registered handle\n"
        ci.ch_auto_add( ci.ch_handle(), False ) # disable adding newly created suites to handle\n"
    except RuntimeError as e:
        print(str(e))
        
    ci.ch_drop_user("")  # drop all handle associated with current user
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();
           
def test_client_ch_remove(ci):
    test = "test_client_ch_remove"
    log_msg(ci,test)
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite(test + str(i))
    ci.load(defs)
    
    try:
        suite_names = [ test + '1', test + '2', test + '3' ]
        ci.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ test + '1' ]
        ci.ch_remove( suite_names )          # remove suites s1 from the last added handle\n"
        suite_names = [  test + '2'  ]
        ci.ch_remove( ci.ch_handle(), suite_names )  # remove suites s2 from the last added handle\n"
    except RuntimeError as e:
        print(str(e))
        
    ci.ch_drop_user("")  # drop all handle associated with current user
    for i in range(1,7):  ci.delete("/" + test + str(i)  ) # stop  downstream test from re-starting this
    ci.sync_local();
           
def test_client_get_file(ci):
    test = "test_client_get_file"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
      
    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    while 1:
        if ci.news_local():
            ci.sync_local() # get the changes, synced with local defs
            suite = ci.get_defs().find_suite(test)
            assert suite != None, "Expected to find suite"
            if suite.get_state() == State.complete:
                break;
            time.sleep(3)

    try:
        for file_t in [ 'script', 'job', 'jobout', 'manual' ]:
            the_returned_file = ci.get_file('/' + test +'/f1/t1',file_t)  # make a request to the server
            assert len(the_returned_file) > 0,"Expected ci.get_file(/" + test + "/f1/t1," + file_t + ") to return something"
    except RuntimeError as e:
        print(str(e))

    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove,True)   # True means ignore errors   
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
    
def test_client_alter_add(ci):
    test = "test_client_alter_add"
    log_msg(ci,test)
    ci.load(create_defs(ci,test))   

    t1 = "/" + test + "/f1/t1"
    ci.alter(t1,"add","variable","var","var_name")
    ci.alter(t1,"add","time","+00:30")
    ci.alter(t1,"add","time","01:30")
    ci.alter(t1,"add","time","01:30 20:00 00:30")
    ci.alter(t1,"add","today","+00:30")
    ci.alter(t1,"add","today","01:30")
    ci.alter(t1,"add","today","01:30 20:00 00:30")
    ci.alter(t1,"add","date","01.01.2001")
    ci.alter(t1,"add","date","*.01.2001")
    ci.alter(t1,"add","date","*.*.2001")
    ci.alter(t1,"add","date","*.*.*")
    ci.alter(t1,"add","day","sunday")
    ci.alter(t1,"add","day","monday")
    ci.alter(t1,"add","day","tuesday")
    ci.alter(t1,"add","day","wednesday")
    ci.alter(t1,"add","day","thursday")
    ci.alter(t1,"add","day","friday")
    ci.alter(t1,"add","day","saturday")
    ci.alter(t1,"add","late","late -s +00:15 -a 20:00 -c +02:00")

    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(ci.get_defs())
    assert( len(list(task_t1.times))) == 3 ,"Expected 3 time :\n" + str(ci.get_defs())
    assert( len(list(task_t1.todays))) == 3 ,"Expected 3 today's :\n" + str(ci.get_defs())
    assert( len(list(task_t1.dates))) == 4 ,"Expected 4 dates :\n" + str(ci.get_defs())
    assert( len(list(task_t1.days))) == 7 ,"Expected 7 days :\n" + str(ci.get_defs())
    assert str(task_t1.get_late()) == "late -s +00:15 -a 20:00 -c +02:00", "Expected late 'late -s +00:15 -a 20:00 -c +02:00'" + str(ci.get_defs())
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
           

def test_client_alter_delete(ci):
    test = "test_client_alter_delete"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
     
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
    
    ci.load(defs)   

    ci.alter(t1,"delete","variable","var")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","variable")  # delete all variables
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.variables))) == 0 ,"Expected 0 variable :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","time","00:30")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.times))) == 1 ,"Expected 1 time :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","time")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.times))) == 0 ,"Expected 0 time :\n" + str(ci.get_defs())
    
    ci.alter(t1,"delete","today","00:30")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.todays))) == 1 ,"Expected 1 today :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","today")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.todays))) == 0 ,"Expected 0 today :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","date","01.01.2001")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.dates))) == 1 ,"Expected 1 date :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","date")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.dates))) == 0 ,"Expected 0 date :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","day","sunday")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.days))) == 1 ,"Expected 1 day :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","day")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.days))) == 0 ,"Expected 0 day :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","event","event")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.events))) == 1 ,"Expected 1 event :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","event")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.events))) == 0 ,"Expected 0 event :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","meter","meter")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.meters))) == 1 ,"Expected 1 meter :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","meter")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.meters))) == 0 ,"Expected 0 meter :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","label","label")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.labels))) == 1 ,"Expected 1 label :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","label")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.labels))) == 0 ,"Expected 0 label :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","limit","limit")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.limits))) == 1 ,"Expected 1 limit :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","limit")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.limits))) == 0 ,"Expected 0 limit :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","inlimit","limit")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.inlimits))) == 1 ,"Expected 1 inlimit :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","inlimit")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.inlimits))) == 0 ,"Expected 0 inlimit :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","cron")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.crons))) == 0 ,"Expected 0 crons :\n" + str(ci.get_defs())

    ci.alter(t1,"delete","late")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert task_t1.get_late() == None, "expected no late after delete" 


    task_t1 = ci.get_defs().find_abs_node(t1)
    assert task_t1.get_trigger() != None, "Expected trigger:\n" + str(ci.get_defs())
    ci.alter(t1,"delete","trigger")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert task_t1.get_trigger() == None, "Expected trigger to be deleted:\n" + str(ci.get_defs())

    task_t1 = ci.get_defs().find_abs_node(t1)
    assert task_t1.get_complete() != None, "Expected complete:\n" + str(ci.get_defs())
    ci.alter(t1,"delete","complete")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert task_t1.get_complete() == None, "Expected complete to be deleted:\n" + str(ci.get_defs())

    ci.alter(t2,"delete","repeat")   
    ci.sync_local()
    task_t2 = ci.get_defs().find_abs_node(t2)
    repeat = task_t2.get_repeat()
    assert repeat.empty(), "Expected repeat to be deleted:\n" + str(ci.get_defs())
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
 
def test_client_alter_change(ci):
    test = "test_client_alter_change"
    log_msg(ci,test)
    defs = create_defs(ci,test)   
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
           
    ci.load(defs)   

    ci.alter(t1,"change","late","-s +10:00")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    variable = task_t1.get_late()
    assert str(task_t1.get_late()) == "late -s +10:00", "Expected alter of late to be 'late -s +10:00' but found " + str(task_t1.get_late())


    ci.alter(t1,"change","variable","var","changed_var")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    variable = task_t1.find_variable("var")
    assert variable.value() == "changed_var", "Expected alter of variable to be 'change_var' but found " + variable.value()

    ci.alter(t1,"change","meter","meter","10")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    meter = task_t1.find_meter("meter")
    assert meter.value() == 10, "Expected alter of meter to be 10 but found " + str(meter.value())

    ci.alter(t1,"change","event","event","set")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    event = task_t1.find_event("event")
    assert event.value() == True, "Expected alter of event to be set but found " + str(event.value())

    ci.alter(t1,"change","trigger","t2 == aborted")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    trigger = task_t1.get_trigger()
    assert trigger.get_expression() == "t2 == aborted", "Expected alter of trigger to be 't2 == aborted' but found " + trigger.get_expression()

    ci.alter(t1,"change","trigger","/test_client_alter_change/f1/t2 == complete")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    trigger = task_t1.get_trigger()
    assert trigger.get_expression() == "/test_client_alter_change/f1/t2 == complete", "Expected alter of trigger to be '/test_client_alter_change/f1/t2 == complete' but found " + trigger.get_expression()

    ci.alter(t1,"change","complete","t2 == aborted")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    complete = task_t1.get_complete()
    assert complete.get_expression() == "t2 == aborted", "Expected alter of complete to be 't2 == aborted' but found " + complete.get_expression()

    ci.alter(t1,"change","complete","/test_client_alter_change/f1/t2 == active")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    complete = task_t1.get_complete()
    assert complete.get_expression() == "/test_client_alter_change/f1/t2 == active", "Expected alter of complete to be '/test_client_alter_change/f1/t2 == active' but found " + complete.get_expression()

    ci.alter(t1,"change","limit_max","limit", "2")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    limit = task_t1.find_limit("limit")
    assert limit != None, "Expected to find limit"
    assert limit.limit() == 2, "Expected alter of limit_max to be 2 but found " + str(limit.limit())

    ci.alter(t1,"change","limit_value","limit", "2")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    limit = task_t1.find_limit("limit")
    assert limit != None, "Expected to find limit"
    assert limit.value() == 2, "Expected alter of limit_value to be 2 but found " + str(limit.value())

    ci.alter(t1,"change","label","label","new-value")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    label = task_t1.find_label("label")
    assert label.new_value() == "new-value", "Expected alter of label to be 'new-value' but found " + label.new_value()
    
    ci.alter(repeat_date_path,"change","repeat","20100113")   
    ci.sync_local()
    task = ci.get_defs().find_abs_node(repeat_date_path)
    repeat = task.get_repeat()
    assert repeat.value() == 20100113, "Expected alter of repeat to be 20100113 but found " + str(repeat.value())
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
 
def test_client_alter_flag(ci):
    test = "test_client_alter_flag"
    log_msg(ci,test)
    defs = create_defs(ci,test)   
    t1 = "/" + test + "/f1/t1"
     
    task_t1 = defs.find_abs_node(t1)
           
    ci.load(defs)   

    flag = Flag()
    flag_list = flag.list() # flag_list is of type FlagTypeVec
    for flg in flag_list: 
        ci.alter(t1,"set_flag",flag.type_to_string(flg) )   
        ci.sync_local()
        task_t1 = ci.get_defs().find_abs_node(t1)
        task_flag = task_t1.get_flag()
        assert task_flag.is_set( flg ),"expected flag %r to be set" % task_flag.type_to_string(flg)

        # alter itself causes the flag message to be set, and preserved
        if flg == FlagType.message: continue 
        
        ci.alter(t1,"clear_flag",flag.type_to_string(flg) )   
        ci.sync_local()
        task_t1 = ci.get_defs().find_abs_node(t1)
        task_flag = task_t1.get_flag()
        assert not task_flag.is_set( flg ),"expected flag %r NOT to be set" % task_flag.type_to_string(flg)
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this

def test_client_flag_migrated(ci):
    # ENABLE for ecflow 4.5.0
    return
    test = "test_client_flag_migrated"
    log_msg(ci,test)
    
    defs = create_defs(ci,test)   
    s1 = "/" + test  
   
    ci.load(defs)   
    ci.sync_local()
 
    suite = defs.find_suite(test)
    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))
 
    ci.alter(s1,"set_flag","migrated")   
    ci.sync_local()
    suite = defs.find_suite(test)
    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 1, "Expected 1 nodes, but found " + str(len(node_vec))
 
    ci.checkpt()  # checkpoint after setting flag migrated, need to prove nodes still persisted
     
    ci.alter(s1,"clear_flag","migrated")   
    ci.sync_local()
    suite = defs.find_suite(test)
    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 4, "Expected 4 nodes, but found " + str(len(node_vec))
    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
 
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

def test_client_force(ci):
    test = "test_client_force"
    log_msg(ci,test)
    defs = create_defs(ci,test) 
     
    path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]       
    t1 = path_list[0]      
    for path in path_list:
        task = defs.find_abs_node(path)
        assert task != None, "Expected to find task at path " + path
        task.add_event("event")

    ci.load(defs)   
    
    state_list = [ State.unknown, State.active, State.complete, State.queued, State.submitted, State.aborted ]
    for state in state_list:
        ci.force_state(t1,state) 
        ci.sync_local()
        task = ci.get_defs().find_abs_node(t1)
        assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())      
    for state in state_list:
        ci.force_state( path_list,state) 
        ci.sync_local()
        for path in path_list:
            task = ci.get_defs().find_abs_node(path)
            assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())     
 
    for state in state_list:
        ci.force_state_recursive("/" + test,state) 
        ci.sync_local()
        task = ci.get_defs().find_abs_node(t1)
        assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())
    suite_paths = [ "/" + test ]
    for state in state_list:
        ci.force_state_recursive( suite_paths,state) 
        ci.sync_local()
        task = ci.get_defs().find_abs_node(t1)
        assert task.get_state() == state, "Expected state " + state + " but found " + str(task.get_state())           
    
    event_states = [ "set", "clear" ]
    for ev_state in event_states:
        for path in path_list:
            ci.force_event(path + ":event" , ev_state)
            ci.sync_local()
            task = ci.get_defs().find_abs_node(path)
            event_fnd = False
            for event in task.events:
                event_fnd = True
                if ev_state == "set" :  assert event.value() == True  ," Expected event value to be set"
                else:                   assert event.value() == False ," Expected event value to be clear"
            assert event_fnd == True," Expected event to be found"

    event_path_list = [ "/" + test + "/f1/t1:event", "/" + test + "/f1/t2:event" ]       
    event_states = [ "set", "clear" ]
    for ev_state in event_states:
        ci.force_event( event_path_list , ev_state)
        ci.sync_local()
        for path in path_list:
            task = ci.get_defs().find_abs_node(path)
            event_fnd = False
            for event in task.events:
                event_fnd = True
                if ev_state == "set" :  assert event.value() == True  ," Expected event value to be set"
                else:                   assert event.value() == False ," Expected event value to be clear"
            assert event_fnd == True," Expected event to be found"

    ci.suspend("/" + test  ) # stop  downstream test from re-starting this
      
def test_client_replace(ci,on_disk):
    test = "test_client_replace"
    if on_disk:
        test = "test_client_replace_on_disk"
    log_msg(ci,test)
    ci.log_msg(str(on_disk))
     
    # Create and load the following defs
    # s1
    #   f1
    #     t1
    #     t2
    ci.load(create_defs(ci,test))  
    
    #===============================================================================
    # Example of using replace to ADD a *NEW* node hierarchy to an existing suite
    # we should end up with:
    # s1
    #   f1
    #     t1
    #     t2
    #   f2
    #     t1
    client_def = create_defs(ci,test)
    client_def.find_suite(test).add_family("f2").add_task("t1")
    if on_disk:
        client_def.save_as_defs(test + ".def")
        client_def = test + ".def"
    
    ci.replace("/" + test + "/f2",client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
    ci.get_server_defs()
    assert ci.get_defs().find_abs_node("/" + test + "/f2/t1") != None, "Expected to find task /" + test + "/f2/t1\n" + str(ci.get_defs())


    # Example of using replace to *REMOVE* node hierarchy to an existing suite, could have used delete
    assert ci.get_defs().find_abs_node("/" + test + "/f1/t1") != None, "Expected to find task /" + test + "/f1/t1\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/" + test + "/f2/t1") != None, "Expected to find task /" + test + "/f2/t1\n" + str(ci.get_defs())
    client_def = Defs()
    client_def.add_suite(test)    # should only have the suite
    if on_disk:
        client_def.save_as_defs(test + ".def")
        client_def = test + ".def"

    ci.replace("/" + test,client_def)   
    ci.get_server_defs()
    assert ci.get_defs().find_abs_node("/" + test + "/f1/t1") == None, "Expected NOT to find task /" + test + "/f1/t1\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/" + test + "/f2/t1") == None, "Expected NOT to find task /" + test + "/f2/t1\n" + str(ci.get_defs())


    #===============================================================================
    # Example of using replace to add a *NEW* suite
    test2 = test + "2";
    client_def = Defs();
    client_def.add_suite(test2)
    if on_disk:
        client_def.save_as_defs(test + ".def")
        client_def = test + ".def"

    ci.replace("/" + test2,client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
    ci.get_server_defs()
    suite = ci.get_defs().find_suite(test2)
    assert suite != None ,"Expected to find suite :" + test2 + "\n" + str(ci.get_defs())
           
    # replace added suite s2 with a new s2 which has a task, 
    # s2 must exist on the client defs
    client_def = Defs();
    client_def.add_suite(test2).add_task("t1")
    if on_disk:
        client_def.save_as_defs(test + ".def")
        client_def = test + ".def"

    ci.replace("/" + test2,client_def) 
        
    ci.get_server_defs()
    assert ci.get_defs().find_abs_node("/" + test2 + "/t1") != None, "Expected to find task /" + test2 + "/t1\n" + str(ci.get_defs())
    if on_disk:
        os.remove(client_def)

    ci.suspend("/" + test  )  # stop  downstream test from re-starting this
    ci.suspend("/" + test2  ) # stop  downstream test from re-starting this

def test_client_suspend(ci):
    test = "test_client_suspend"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    ci.suspend("/" + test )  
           
    ci.sync_local();
    suite = ci.get_defs().find_suite(test)
    assert suite.is_suspended(), "Expected to find suite suspended"
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this
    
def test_client_suspend_multiple_paths(ci):
    test = "test_client_suspend_multiple_paths"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_variable("ECF_DUMMY_TASK","")
    
    ci.load(defs)  
    ci.begin_all_suites()  
    
    path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
    ci.suspend( path_list )  
           
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
    assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this
            
def test_client_resume(ci):
    test = "test_client_resume"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    ci.suspend("/" + test )  
    ci.sync_local();
    suite = ci.get_defs().find_suite(test)
    assert suite.is_suspended(), "Expected to find suite suspended"
    
    ci.resume("/" + test )  
    ci.sync_local();
    suite = ci.get_defs().find_suite(test)
    assert suite.is_suspended() == False, "Expected to find suite resumed"

    # suspend to stop down stream tests from restarting this test
    ci.suspend("/" + test )  

def test_client_resume_multiple_paths(ci):
    test = "test_client_resume_multiple_paths"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    suite = defs.find_suite(test)
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    path_list = [ "/" + test + "/f1/t1", "/" + test + "/f1/t2" ]
    ci.suspend( path_list )  
  
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
    assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"

    ci.resume( path_list )  
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task_t1.is_suspended() == False, "Expected to find task t1 to be resumed"
    assert task_t2.is_suspended() == False, "Expected to find task t2 to be resumed"
    
     # suspend to stop down stream tests from restarting this test
    ci.suspend("/" + test )  
  
def test_client_delete_node(ci): 
    test = "test_client_delete_node"
    log_msg(ci,test)
    defs = create_defs(ci,test)
    
    task_vec = defs.get_all_tasks();
    assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)

    ci.load(defs)  
    ci.sync_local();
    for task in task_vec:
        node = ci.get_defs().find_abs_node(task.get_abs_node_path()) 
        assert node != None , "Expected to find task " + task.get_abs_node_path()  + ":\n"  + str(ci.get_defs()) 

    for task in task_vec:
        ci.delete(task.get_abs_node_path())

    ci.sync_local();
    for task in task_vec:
        node = ci.get_defs().find_abs_node(task.get_abs_node_path()) 
        assert node == None , "Expected not to find task " + task.get_abs_node_path()  + " as it should have been deleted:\n" + str(ci.get_defs())   
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this
    
def test_client_delete_node_multiple_paths(ci): 
    test = "test_client_delete_node_multiple_paths"
    log_msg(ci,test)
    defs = create_defs(ci,test)
    
    task_vec = defs.get_all_tasks();
    assert len(task_vec) > 0, "Expected some tasks but found none:\n" + str(defs)

    paths = []
    for task in task_vec:
        paths.append(task.get_abs_node_path()) 
 
    ci.load(defs)  
    
    ci.sync_local();
    for task in task_vec:
        node = ci.get_defs().find_abs_node(task.get_abs_node_path()) 
        assert node != None , "Expected to find task " + task.get_abs_node_path()  + ":\n"  + str(ci.get_defs()) 

    ci.delete(paths)

    ci.sync_local();
    for task in task_vec:
        node = ci.get_defs().find_abs_node(task.get_abs_node_path()) 
        assert node == None , "Expected not to find task " + task.get_abs_node_path()  + " as it should have been deleted:\n" + str(ci.get_defs())   
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this    

def test_client_check_defstatus(ci):            
    test = "test_client_check_defstatus"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    
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
    
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
     
    ci.sync_local() # get the changes, synced with local defs
    #print ci.get_defs();
    task_t1 = ci.get_defs().find_abs_node(t1)
    task_t2 = ci.get_defs().find_abs_node(t2)
    assert task_t1 != None,"Could not find t1"
    assert task_t2 != None,"Could not find t2"
  
    assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
    assert task_t2.get_state() == State.queued, "Expected state queued " + str(task_t2.get_state())

    assert task_t1.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t1.get_state())
    assert task_t2.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t2.get_state())
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this
   
    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      
    
def test_ECFLOW_189(ci):
    # Bug, when a node is resumed it ignored holding dependencies higher up the tree.
    # i.e Previously when we resumed a node, it ignored trigger/time/node state, dependencies higher up the tree
    test = "test_ECFLOW_189"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
    ci.restart_server()
    ci.load(defs)   
    
    ci.suspend("/" + test  )
    ci.suspend("/" + test + "/f1/t1")
    ci.suspend("/" + test + "/f1/t2")
        
    ci.begin_all_suites()
    
    ci.sync_local() # get the changes, synced with local defs
    #print ci.get_defs();
    task_t1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task_t1 != None,"Could not find /" + test + "/f1/t1"
    assert task_t2 != None,"Could not find /" + test + "/f1/t2"
  
    assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
    assert task_t2.get_state() == State.queued, "Expected state queued but found " + str(task_t2.get_state())
    assert task_t1.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t1.get_dstate())
    assert task_t2.get_dstate() == DState.suspended, "Expected state suspended but found " + str(task_t2.get_dstate())

    # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
    ci.resume("/" + test + "/f1/t1")
    ci.resume("/" + test + "/f1/t2")
     
    time.sleep(3)
    ci.sync_local() # get the changes, synced with local defs
    #print ci.get_defs();
    task_t1 = ci.get_defs().find_abs_node("/" + test + "/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/" + test + "/f1/t2")
    assert task_t1.get_state() == State.queued, "Expected state queued but found " + str(task_t1.get_state())
    assert task_t2.get_state() == State.queued, "Expected state queued but found " + str(task_t2.get_state())
    assert task_t1.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t1.get_dstate())
    assert task_t2.get_dstate() == DState.queued, "Expected state queued but found " + str(task_t2.get_dstate())
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this

    dir_to_remove = ecf_home(the_port) + "/" + "test_ECFLOW_189"
    shutil.rmtree(dir_to_remove)      


def test_ECFLOW_199(ci):
    # Test ClientInvoker::changed_node_paths
    test = "test_ECFLOW_199"
    log_msg(ci,test)
    defs = create_defs(ci,test)  
    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
    ci.restart_server()
    ci.load(defs)   
    
    ci.suspend("/" + test  )
    ci.suspend("/" + test + "/f1/t1")
    ci.suspend("/" + test + "/f1/t2")

    ci.begin_all_suites()
    
    ci.sync_local() # get the changes, synced with local defs
    #print ci.get_defs();
    assert len(list(ci.changed_node_paths)) == 0, "Expected first call to sync_local, to have no changed paths but found " + str(len(list(ci.changed_node_paths)))
    
    # ok now resume t1/t2, they should remain queued, since the Suite is still suspended
    ci.resume("/" + test + "/f1/t1")
    ci.sync_local() 
    for path in ci.changed_node_paths:
        print("   changed node path " + path);
    assert len(list(ci.changed_node_paths)) == 1, "Expected 1 changed path but found " + str(len(list(ci.changed_node_paths)))

    ci.resume("/" + test + "/f1/t2")
    ci.sync_local() 
    for path in ci.changed_node_paths:
        print("   changed node path " + path);
    assert len(list(ci.changed_node_paths)) == 1, "Expected 1 changed path but found " + str(len(list(ci.changed_node_paths)))
    ci.suspend("/" + test  )  # stop  downstream test from re-starting this

    dir_to_remove = ecf_home(the_port) + "/" + test
    shutil.rmtree(dir_to_remove)      

def test_gui(ci):   
    ci.delete_all() # start fresh
    global the_port
    the_port = ci.get_port();
    test_version(ci)
    PrintStyle.set_style( Style.STATE ) # show node state 
    test_client_get_server_defs(ci)             
           
    test_client_restart_server(ci)             
    test_client_halt_server(ci)             
    test_client_shutdown_server(ci)   
       
    test_client_load_in_memory_defs(ci)             
    test_client_load_from_disk(ci)             
    test_client_checkpt(ci, the_port)             
    test_client_restore_from_checkpt(ci, the_port)             
            
    test_client_reload_wl_file(ci, the_port)             
    
    test_client_run(ci)  
    test_client_run_with_multiple_paths(ci)     
    test_client_requeue(ci)             
    test_client_requeue_with_multiple_paths(ci)             
    test_client_free_dep(ci)              
   
    test_client_suites(ci)
    test_client_ch_suites(ci)  
    test_client_ch_register(ci)             
    test_client_ch_drop(ci)             
    test_client_ch_drop_user(ci)             
    test_client_ch_add(ci)             
    test_client_ch_auto_add(ci)             
    test_client_ch_remove(ci)             
              
    test_client_get_file(ci)             
    test_client_alter_add(ci) 
    test_client_alter_delete(ci) 
    test_client_alter_change(ci) 
    test_client_alter_flag(ci) 
    test_client_flag_migrated(ci) 
 
    test_client_force(ci)             
    test_client_replace(ci,False)             
    test_client_replace(ci,True)             
   
    test_client_suspend(ci)             
    test_client_suspend_multiple_paths(ci)             
    test_client_resume(ci)             
    test_client_resume_multiple_paths(ci)             
    test_client_delete_node(ci)             
    test_client_delete_node_multiple_paths(ci)             
   
    test_client_check(ci)  
    test_client_check_defstatus(ci)  
    
    test_ECFLOW_189(ci)         
    test_ECFLOW_199(ci)         

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
                        help="How long to run the tests in seconds. default is 0, which one test loop")
    ARGS = PARSER.parse_args()
    #print ARGS   
    
    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        CL.ping() 
        
        start_time = time.time()
        while True:
            test_gui(CL)
            elapsed = int(time.time() - start_time)
            print "======================================================"
            print "elapsed time :",elapsed
            print "======================================================"
            if elapsed > int(ARGS.time):
                break
                
    except RuntimeError, ex:
        print "Error: " + str(ex)
        print "Check host and port number are correct."

    clean_up_server(CL)
    clean_up_data(CL)
