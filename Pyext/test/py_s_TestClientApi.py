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
from ecflow import Defs, Clock, DState,  Style, State, RepeatDate, PrintStyle, File, Client, SState, \
                   JobCreationCtrl, CheckPt, Cron, debug_build

def ecf_home(): 
    # the_port is global
    # debug_build() is defined for ecflow. Used in test to distinguish debug/release ecflow
    # Vary ECF_HOME based on debug/release/port allowing multiple invocations of these tests
    if debug_build():
        return os.getcwd() + "/test/data/ecf_home_debug_" + str(the_port)
    return os.getcwd() + "/test/data/ecf_home_release_" + str(the_port)

def ecf_includes() :  return os.getcwd() + "/test/data/includes"

def create_defs(name=""):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    suite = defs.add_suite(suite_name);
    
    ecfhome = ecf_home();
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("ECF_CLIENT_EXE_PATH", File.find_client());
    suite.add_variable("SLEEP", "1");  # not strictly required since default is 1 second
    suite.add_variable("ECF_INCLUDE", ecf_includes());

    family = suite.add_family("f1")
    family.add_task("t1")
    family.add_task("t2")
    return defs;
    

def get_username(): return pwd.getpwuid(os.getuid())[ 0 ]

def log_file_path(port): return "./" + gethostname() + "." + port + ".ecf.log"
def checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check"
def backup_checkpt_file_path(port): return "./" + gethostname() + "." + port + ".ecf.check.b"
def white_list_file_path(port): return "./" + gethostname() + "." + port + ".ecf.lists"
 
def test_host_port(ci,host,port):
    try :
        ci.set_host_port(host,port)
        return True
    except RuntimeError:
        return False 

def test_host_port_(ci,host_port):
    try :
        ci.set_host_port(host_port)
        return True
    except RuntimeError:
        return False 
    
def test_client_host_port(host,port):
    try :
        Client(host,port)
        return True
    except RuntimeError:
        return False 

def test_client_host_port_(host_port):
    try :
        Client(host_port)
        return True
    except RuntimeError:
        return False 
        
def test_set_host_port():
    print "test_set_host_port"
    ci = Client();
    assert test_host_port(ci,"host","3141") ,  "Expected no errors"
    assert test_host_port(ci,"host",4444) ,    "Expected no errors"
    assert test_host_port_(ci,"host:4444") ,    "Expected no errors"
    assert test_host_port(ci,"","") == False , "Expected errors"
    assert test_host_port(ci,"host","") == False , "Expected errors"
    assert test_host_port(ci,"host","host") == False , "Expected errors"
    assert test_host_port_(ci,"host:host") == False , "Expected errors"
    assert test_host_port_(ci,"3141:host") == False , "Expected errors"
    
    assert test_client_host_port("host","3141") ,  "Expected no errors"
    assert test_client_host_port("host",4444) ,    "Expected no errors"
    assert test_client_host_port_("host:4444") ,    "Expected no errors"
    assert test_client_host_port("","") == False , "Expected errors"
    assert test_client_host_port("host","") == False , "Expected errors"
    assert test_client_host_port("host","host") == False , "Expected errors"
    assert test_client_host_port_("host:host") == False , "Expected errors"
    assert test_client_host_port_("3141:host") == False , "Expected errors"

def test_version(ci):
    client_version = ci.version();
    server_version = ci.server_version();
    assert client_version == server_version, "Expected client version(" + client_version +") and server version(" +  server_version + ") to match\n";
    
def test_client_get_server_defs(ci):
    print "Client version is " + ci.version();
    print "test_client_get_server_defs"
    ci.delete_all() # start fresh
    ci.load(create_defs())  
    ci.get_server_defs() 
    assert ci.get_defs().find_suite("s1") != None, "Expected to find suite of name s1:\n" + str(ci.get_defs())

    ci.delete_all() # start fresh
    ci.load(create_defs())  
    ci.sync_local()
    assert ci.get_defs().find_suite("s1") != None, "Expected to find suite of name s1:\n" + str(ci.get_defs())


def test_client_new_log(ci, port):
    print "test_client_new_log"
    try : os.remove("./test_client_new_log.log") # delete file if it exists
    except: pass
    
    ci.new_log("./test_client_new_log.log") 
    ci.flush_log() # close log file and force write to disk
    assert os.path.exists("./test_client_new_log.log"), "New log does not exist"
    
    # reset new log to original
    ci.new_log(log_file_path(port)) 
    ci.ping()
    ci.flush_log() # close log file and force write to disk    
    log_file = open(log_file_path(port))
    try:     log_text = log_file.read();     # assume log file not to big
    finally: log_file.close();
    assert log_text.find("--ping") != -1, "Expected to find --ping in log file"
    try: os.remove("./test_client_new_log.log")
    except: pass


def test_client_clear_log(ci, port):
    print "test_client_clear_log"
    # populate log
    ci.ping();
    ci.ping();
    ci.flush_log() # close log file and force write to disk    
    log_file = open(log_file_path(port))
    try:     log_text = log_file.read();     # assume log file not to big
    finally: log_file.close();
    assert log_text.find("--ping") != -1, "Expected to find --ping in log file"
    
    ci.clear_log()    
    log_file = open(log_file_path(port))
    try:     log_text = log_file.read();     # assume log file not to big
    finally: log_file.close();
    assert len(log_text) == 0, "Expected log file to be empty but found " + log_text


def test_client_log_msg(ci, port):
    print "test_client_log_msg"
    # Send a message to the log file, then make sure it was written
    ci.log_msg("Humpty dumpty sat on a wall!")
    ci.flush_log(); # flush and close log file, so we can open it
    log_file = open(log_file_path(port))
    try:     log_text = log_file.read();     # assume log file not to big
    finally: log_file.close();
    assert log_text.find("Humpty dumpty sat on a wall!") != -1, "Expected to find Humpty dumpty in the log file"                
        
         
def test_client_restart_server(ci):
    print "test_client_restart_server"
    ci.restart_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.RUNNING, "Expected server to be running"


def test_client_halt_server(ci):
    print "test_client_halt_server"
    ci.halt_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.HALTED, "Expected server to be halted"


def test_client_shutdown_server(ci):
    print "test_client_shutdown_server"
    ci.shutdown_server()
    ci.sync_local()
    assert ci.get_defs().get_server_state() == SState.SHUTDOWN, "Expected server to be shutdown"
    

def test_client_load_in_memory_defs(ci):
    print "test_client_load_in_memory_defs"
    ci.delete_all() # start fresh
    ci.load(create_defs())  
    ci.sync_local() 
    assert ci.get_defs().find_suite("s1") != None, "Expected to find suite of name s1:\n" + str(ci.get_defs())              


def test_client_load_from_disk(ci):            
    print "test_client_load_from_disk"
    ci.delete_all() # start fresh
    defs = create_defs();
    defs_file = "test_client_load_from_disk.def"
    defs.save_as_defs(defs_file)     
    assert os.path.exists(defs_file), "Expected file " + defs_file + " to exist after defs.save_as_defs()"
    ci.load(defs_file) # open and parse defs file, and load into server.\n"
        
    # check load worked
    ci.sync_local() 
    assert ci.get_defs().find_suite("s1") != None, "Expected to find suite of name s1:\n" + str(ci.get_defs())
    os.remove(defs_file)


def test_client_checkpt(ci, port):
    print "test_client_checkpt"
    # start fresh
    ci.delete_all() 
    try:    
        os.remove(checkpt_file_path(port))
        os.remove(backup_checkpt_file_path(port))
    except: pass
    
    ci.load(create_defs())  
    ci.checkpt()
    assert os.path.exists(checkpt_file_path(port)), "Expected check pt file to exist after ci.checkpt()"
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


def test_client_restore_from_checkpt(ci, port):          
    print "test_client_restore_from_checkpt"
    # start fresh
    ci.delete_all() 
    try:    
        os.remove(checkpt_file_path(port))
        os.remove(backup_checkpt_file_path(port))
    except: pass
    
    ci.load(create_defs())  
    ci.checkpt()
    ci.delete_all() 
    
    ci.sync_local() 
    assert ci.get_defs().find_suite("s1") == None, "Expected all suites to be delete:\n"
    
    ci.halt_server()  # server must be halted, otherwise restore_from_checkpt will throw
    ci.restore_from_checkpt()
    
    ci.sync_local() 
    assert ci.get_defs().find_suite("s1") != None, "Expected to find suite s1 after restore from checkpt:\n" + str(ci.get_defs())

    os.remove(checkpt_file_path(port))


def test_client_reload_wl_file(ci, port):
    print "test_client_reload_wl_file"
    
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
    print "test_client_run"
    ci.delete_all()     
    defs = create_defs("test_client_run")  
    suite = defs.find_suite("test_client_run")
    suite.add_defstatus(DState.suspended)

    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
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
        time.sleep(2)
        if count > 20:
            assert False, "test_client_run aborted after " + str(count) + " loops:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    
    dir_to_remove = ecf_home() + "/" + "test_client_run"
    shutil.rmtree(dir_to_remove)      

def test_client_run_with_multiple_paths(ci):            
    print "test_client_run_with_multiple_paths"
    ci.delete_all()     
    defs = create_defs("test_client_run_with_multiple_paths")  
    suite = defs.find_suite("test_client_run_with_multiple_paths")
    suite.add_defstatus(DState.suspended)

    defs.generate_scripts();
    
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
    
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    path_list = [ "/test_client_run_with_multiple_paths/f1/t1", "/test_client_run_with_multiple_paths/f1/t2"]
    ci.run( path_list, False)

    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        suite = ci.get_defs().find_suite("test_client_run_with_multiple_paths")
        assert suite != None, "Expected to find suite test_client_run_with_multiple_paths:\n" + str(ci.get_defs())
        if suite.get_state() == State.complete:
            break;
        time.sleep(2)
        if count > 12:
            assert False, "test_client_run_with_multiple_paths aborted after " + str(count) + " loops:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    
    dir_to_remove = ecf_home() + "/" + "test_client_run_with_multiple_paths"
    shutil.rmtree(dir_to_remove)      

    
def test_client_requeue(ci):
    print "test_client_requeue"
    ci.delete_all()     
    defs = create_defs("test_client_requeue")  
    suite = defs.find_suite("test_client_requeue")
    suite.add_defstatus(DState.suspended)
     
    defs.generate_scripts();
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    ci.force_state_recursive("/test_client_requeue",State.unknown)
    ci.sync_local();
    suite = ci.get_defs().find_suite("test_client_requeue")
    assert suite.get_state() == State.unknown, "Expected to find suite with state unknown"

    ci.requeue("/test_client_requeue")
    ci.sync_local();
    suite = ci.get_defs().find_suite("test_client_requeue")
    assert suite.get_state() == State.queued, "Expected to find suite with state queued"

    dir_to_remove = ecf_home() + "/" + "test_client_requeue"
    shutil.rmtree(dir_to_remove)      

def test_client_requeue_with_multiple_paths(ci):
    print "test_client_requeue_with_multiple_paths"
    ci.delete_all()     
    defs = create_defs("test_client_requeue_with_multiple_paths")  
    suite = defs.find_suite("test_client_requeue_with_multiple_paths")
    suite.add_defstatus(DState.suspended)
     
    defs.generate_scripts();
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)       
    assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()
 
    ci.restart_server()
    ci.load(defs)           
    ci.begin_all_suites()
    
    ci.force_state_recursive("/test_client_requeue_with_multiple_paths",State.unknown)
    ci.sync_local();
    task1 = ci.get_defs().find_abs_node("/test_client_requeue_with_multiple_paths/f1/t1")
    task2 = ci.get_defs().find_abs_node("/test_client_requeue_with_multiple_paths/f1/t2")
    assert task1.get_state() == State.unknown, "Expected to find t1 with state unknown"
    assert task2.get_state() == State.unknown, "Expected to find t2 with state unknown"

    path_list = [ "/test_client_requeue_with_multiple_paths/f1/t1", "/test_client_requeue_with_multiple_paths/f1/t2" ]
    ci.requeue( path_list)
    ci.sync_local();
    task1 = ci.get_defs().find_abs_node("/test_client_requeue_with_multiple_paths/f1/t1")
    task2 = ci.get_defs().find_abs_node("/test_client_requeue_with_multiple_paths/f1/t2")
    assert task1.get_state() == State.queued, "Expected to find task t1 with state queued"
    assert task2.get_state() == State.queued, "Expected to find task t2 with state queued"

    dir_to_remove = ecf_home() + "/" + "test_client_requeue_with_multiple_paths"
    shutil.rmtree(dir_to_remove)      


def test_client_free_dep(ci):
    print "test_client_free_dep"
    ci.delete_all()  
       
    # add a real clock, since we are adding date dependencies
    # Note: adding a future time dependency on a task, will cause it to requeue, when complete
    # Hence even when we free these dependency they get requeued.
    # So we use todays date.
    ltime = time.localtime();
    day = ltime.tm_mday
    month = ltime.tm_mon
    year = ltime.tm_year
    
    defs = Defs()
    suite = defs.add_suite("test_client_free_dep");
    suite.add_clock(Clock(False)) # true means hybrid, False means real
    ecfhome = ecf_home();
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("ECF_CLIENT_EXE_PATH", File.find_client());
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
    
    t1_path = "/test_client_free_dep/f1/t1"
    t2_path = "/test_client_free_dep/f1/t2"
    t3_path = "/test_client_free_dep/f1/t3"
    t4_path = "/test_client_free_dep/f1/t4"
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

        suite = ci.get_defs().find_suite("test_client_free_dep")
        if suite.get_state() == State.complete:
            break;
        time.sleep(2)       
            
    dir_to_remove = ecf_home() + "/" + "test_client_free_dep"
    shutil.rmtree(dir_to_remove)             


def test_client_stats(ci):
    print "test_client_stats"
    ci.stats()  # writes to standard out
            
def test_client_debug_server_on_off(ci):
    print "test_client_debug_server_on_off"
    ci.debug_server_on()  # writes to standard out
    ci.debug_server_off()  


def test_client_check(ci):
    print "test_client_check"
    ci.delete_all()     
    
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
    # print server_check
    assert len(server_check) > 0, "Expected defs to fail, since no externs in server "
    
def test_client_suites(ci):
    print "test_client_suites"
    ci.delete_all() 
    assert len(ci.suites()) == 0 ,"expected 0 suite "

    defs = create_defs("test_client_suites")
    ci.load(defs)  
    assert len(ci.suites()) == 1 ,"expected 1 suite "
    
    ci.delete_all() 
    defs.add_suite("s2")
    ci.load(defs)  
    assert len(ci.suites()) == 2 ,"expected 2 suite "
    
def test_client_ch_suites(ci):
    print "test_client_ch_suites"
    ci.delete_all()     
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    suite_names = [ 's1', 's2', 's3' ]
    ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites
    ci.ch_register(False,[ "s1"])       # register interest in suites s1 
 
    ci.ch_suites()  # writes to standard out, list of suites and handles

def test_client_ch_register(ci):
    print "test_client_ch_register"
    ci.delete_all()  
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    suite_names = [ 's1', 's2', 's3' ]
    ci.ch_register(True, suite_names)    # register interest in suites s1,s2,s3 and any new suites
    ci.ch_register(False,suite_names)    # register interest in suites s1,s2,s3 only
  
            
def test_client_ch_drop(ci):
    print "test_client_ch_drop"
    ci.delete_all()   
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    try:
        # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ 's1', 's2', 's3' ]
        ci.ch_register(True, suite_names)    
    finally:  
        ci.ch_drop()  # drop using handle stored in ci., from last register
          
          
def test_client_ch_drop_user(ci):
    print "test_client_ch_drop_user"
    ci.delete_all()   
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    try:
        # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ 's1', 's2', 's3' ]
        ci.ch_register(True, suite_names)
    except RuntimeError, e:
        print str(e)
    
    ci.ch_drop_user("")  # drop all handle associated with current user
            
            
def test_client_ch_add(ci):
    print "test_client_ch_add"
    ci.delete_all()  
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    try:
        suite_names = []
        ci.ch_register(True,suite_names)        # register interest in any new suites
        suite_names = [ 's1', 's2' ]
        ci.ch_add(suite_names)                  # add suites s1,s2 to the last added handle
        suite_names = [ 's3', 's4' ]
        ci.ch_add( ci.ch_handle(),suite_names)  # add suites s3,s4 using last handle
    except RuntimeError, e:
        print str(e)
        
    ci.ch_drop_user("")  # drop all handle associated with current user

            
def test_client_ch_auto_add(ci):
    print "test_client_ch_auto_add"
    ci.delete_all()  
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    try:
        suite_names = [ 's1', 's2' , 's3']
        ci.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
        ci.ch_auto_add( False )                 # disable adding newly created suites to last registered handle\n"
        ci.ch_auto_add( True )                  # enable adding newly created suites to last registered handle\n"
        ci.ch_auto_add( ci.ch_handle(), False ) # disable adding newly created suites to handle\n"
    except RuntimeError, e:
        print str(e)
        
    ci.ch_drop_user("")  # drop all handle associated with current user
        
           
def test_client_ch_remove(ci):
    print "test_client_ch_remove"
    ci.delete_all()  
    try: ci.ch_drop_user("")  # drop all handle associated with current user
    except: pass              # Drop throws if no handle registered
    
    defs = Defs()
    for i in range(1,7): defs.add_suite("s" + str(i))
    ci.load(defs)
    
    try:
        suite_names = [ 's1', 's2' , 's3']
        ci.ch_register(True,suite_names)     # register interest in suites s1,s2,s3 and any new suites
        suite_names = [ 's1' ]
        ci.ch_remove( suite_names )          # remove suites s1 from the last added handle\n"
        suite_names = [ 's2' ]
        ci.ch_remove( ci.ch_handle(), suite_names )  # remove suites s2 from the last added handle\n"
    except RuntimeError, e:
        print str(e)
        
    ci.ch_drop_user("")  # drop all handle associated with current user
           
           
def test_client_get_file(ci):
    print "test_client_get_file"
    ci.delete_all()     
    defs = create_defs("test_client_get_file")  
      
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
            suite = ci.get_defs().find_suite("test_client_get_file")
            assert suite != None, "Expected to find suite"
            if suite.get_state() == State.complete:
                break;

    try:
        for file_t in [ 'script', 'job', 'jobout', 'manual' ]:
            the_returned_file = ci.get_file('/test_client_get_file/f1/t1',file_t)  # make a request to the server
            assert len(the_returned_file) > 0,"Expected ci.get_file(/test_client_get_file/f1/t1," + file_t + ") to return something"
    except RuntimeError, e:
        print str(e)

    dir_to_remove = ecf_home() + "/" + "test_client_get_file"
    shutil.rmtree(dir_to_remove,True)   # True means ignore errors   

    
def test_client_plug(ci):
    pass
           
def test_client_alter_add(ci):
    print "test_client_alter_add"
    ci.delete_all()     
    ci.load(create_defs("test_client_alter_add"))   

    t1 = "/test_client_alter_add/f1/t1"
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

    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(ci.get_defs())
    assert( len(list(task_t1.times))) == 3 ,"Expected 3 time :\n" + str(ci.get_defs())
    assert( len(list(task_t1.todays))) == 3 ,"Expected 3 today's :\n" + str(ci.get_defs())
    assert( len(list(task_t1.dates))) == 4 ,"Expected 4 dates :\n" + str(ci.get_defs())
    assert( len(list(task_t1.days))) == 7 ,"Expected 7 days :\n" + str(ci.get_defs())
           

def test_client_alter_delete(ci):
    print "test_client_alter_delete"
    ci.delete_all() 
    defs =create_defs("test_client_alter_delete")   
    t1 = "/test_client_alter_delete/f1/t1"
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
            
    t2 = "/test_client_alter_delete/f1/t2"
    task_t2 = defs.find_abs_node(t2)
    task_t2.add_repeat( RepeatDate("date",20100111,20100115,2) )  # can't add cron and repeat at the same level
    
    ci.load(defs)   

    ci.alter(t1,"delete","variable","var")
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    assert( len(list(task_t1.variables))) == 1 ,"Expected 1 variable :\n" + str(ci.get_defs())
    ci.alter(t1,"delete","variable")  # delete all veriables
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
 
def test_client_alter_change(ci):
    print "test_client_alter_change"
    ci.delete_all() 
    defs =create_defs("test_client_alter_change")   
    t1 = "/test_client_alter_change/f1/t1"
    repeat_date_path = "/test_client_alter_change/f1/repeat_date"
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
            
    f1 = defs.find_abs_node("/test_client_alter_change/f1")
    repeat_date = f1.add_task("repeat_date")
    repeat_date.add_repeat( RepeatDate("date",20100111,20100115,2) )  # can't add cron and repeat at the same level
           
    ci.load(defs)   

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

    ci.alter(t1,"change","complete","t2 == aborted")   
    ci.sync_local()
    task_t1 = ci.get_defs().find_abs_node(t1)
    complete = task_t1.get_complete()
    assert complete.get_expression() == "t2 == aborted", "Expected alter of complete to be 't2 == aborted' but found " + complete.get_expression()
 
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
    
    ci.alter(repeat_date_path,"change","repeat","20100112")   
    ci.sync_local()
    task = ci.get_defs().find_abs_node(repeat_date_path)
    repeat = task.get_repeat()
    assert repeat.value() == 20100112, "Expected alter of repeat to be 20100112 but found " + str(repeat.value())
 

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
    print "test_client_force"
    ci.delete_all()     
    defs = create_defs("test_client_force") 
     
    path_list = [ "/test_client_force/f1/t1", "/test_client_force/f1/t2" ]       
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
        assert task.get_state() == state, "Expected state " + state + " but found " + task.get_state()       
    for state in state_list:
        ci.force_state( path_list,state) 
        ci.sync_local()
        for path in path_list:
            task = ci.get_defs().find_abs_node(path)
            assert task.get_state() == state, "Expected state " + state + " but found " + task.get_state()       
 
    for state in state_list:
        ci.force_state_recursive("/test_client_force",state) 
        ci.sync_local()
        task = ci.get_defs().find_abs_node(t1)
        assert task.get_state() == state, "Expected state " + state + " but found " + task.get_state()
    suite_paths = [ "/test_client_force"]
    for state in state_list:
        ci.force_state_recursive( suite_paths,state) 
        ci.sync_local()
        task = ci.get_defs().find_abs_node(t1)
        assert task.get_state() == state, "Expected state " + state + " but found " + task.get_state()            
    
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

    event_path_list = [ "/test_client_force/f1/t1:event", "/test_client_force/f1/t2:event" ]       
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

      

def test_client_replace(ci,on_disk):
    print "test_client_replace client_defs on disk = " + str(on_disk)
    # Create and load the following defs
    # s1
    #   f1
    #     t1
    #     t2
    ci.delete_all()     
    ci.load(create_defs("s1"))  
    
    #===============================================================================
    # Example of using replace to ADD a *NEW* node hierarchy to an existing suite
    # we should end up with:
    # s1
    #   f1
    #     t1
    #     t2
    #   f2
    #     t1
    #     t2
    client_def = create_defs("s1")
    client_def.find_suite("s1").add_family("f2").add_task("t1")
    if on_disk:
        client_def.save_as_defs("test_client_replace.def")
        client_def = "test_client_replace.def"
    
    ci.replace("/s1/f2",client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
    ci.get_server_defs()
    assert len(list(ci.get_defs().suites)) == 1 ,"Expected 1 suites:\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/s1/f2/t1") != None, "Expected to find task /s1/f2/t1\n" + str(ci.get_defs())


    # Example of using replace to *REMOVE* node hierarchy to an existing suite, could have used delete
    assert ci.get_defs().find_abs_node("/s1/f1/t1") != None, "Expected to find task /s1/f1/t1\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/s1/f2/t1") != None, "Expected to find task /s1/f2/t1\n" + str(ci.get_defs())
    client_def = Defs()
    client_def.add_suite("s1")    # should only have the suite
    if on_disk:
        client_def.save_as_defs("test_client_replace.def")
        client_def = "test_client_replace.def"

    ci.replace("/s1",client_def)   
    ci.get_server_defs()
    assert len(list(ci.get_defs().suites)) == 1 ,"Expected 1 suites:\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/s1/f1/t1") == None, "Expected NOT to find task /s1/f1/t1\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/s1/f2/t1") == None, "Expected NOT to find task /s1/f2/t1\n" + str(ci.get_defs())


    #===============================================================================
    # Example of using replace to add a *NEW* suite
    client_def = Defs();
    client_def.add_suite("s2")
    if on_disk:
        client_def.save_as_defs("test_client_replace.def")
        client_def = "test_client_replace.def"

    ci.replace("/s2",client_def,True,False)  # True means create parents as needed, False means don't bypass checks/zombies
    ci.get_server_defs()
    assert len(list(ci.get_defs().suites)) == 2 ," Expected two suites:\n" + str(ci.get_defs())
           
    # replace added suite s2 with a new s2 which has a task, 
    # s2 must exist on the client defs
    client_def = Defs();
    client_def.add_suite("s2").add_task("t1")
    if on_disk:
        client_def.save_as_defs("test_client_replace.def")
        client_def = "test_client_replace.def"

    ci.replace("/s2",client_def) 
        
    ci.get_server_defs()
    assert len(list(ci.get_defs().suites)) == 2 ," Expected two suites:\n" + str(ci.get_defs())
    assert ci.get_defs().find_abs_node("/s2/t1") != None, "Expected to find task /s2/t1\n" + str(ci.get_defs())
    if on_disk:
        os.remove(client_def)

def test_client_kill(ci):
    pass
        
def test_client_status(ci):
    pass
           
def test_client_order(ci):
    pass
           
def test_client_group(ci):
    pass
           
def test_client_suspend(ci):
    print "test_client_suspend"
    ci.delete_all()     
    defs = create_defs("test_client_suspend")  
    suite = defs.find_suite("test_client_suspend")
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    ci.suspend("/test_client_suspend")  
           
    ci.sync_local();
    suite = ci.get_defs().find_suite("test_client_suspend")
    assert suite.is_suspended(), "Expected to find suite suspended"
    

def test_client_suspend_multiple_paths(ci):
    print "test_client_suspend_multiple_paths"
    ci.delete_all()     
    defs = create_defs("test_client_suspend_multiple_paths")  
    suite = defs.find_suite("test_client_suspend_multiple_paths")
    suite.add_variable("ECF_DUMMY_TASK","")
    
    ci.load(defs)  
    ci.begin_all_suites()  
    
    path_list = [ "/test_client_suspend_multiple_paths/f1/t1", "/test_client_suspend_multiple_paths/f1/t2" ]
    ci.suspend( path_list )  
           
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/test_client_suspend_multiple_paths/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/test_client_suspend_multiple_paths/f1/t2")
    assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
    assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"
            
def test_client_resume(ci):
    print "test_client_resume"
    ci.delete_all()     
    defs = create_defs("test_client_resume")  
    suite = defs.find_suite("test_client_resume")
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    ci.suspend("/test_client_resume")  
    ci.sync_local();
    suite = ci.get_defs().find_suite("test_client_resume")
    assert suite.is_suspended(), "Expected to find suite suspended"
    
    ci.resume("/test_client_resume")  
    ci.sync_local();
    suite = ci.get_defs().find_suite("test_client_resume")
    assert suite.is_suspended() == False, "Expected to find suite resumed"

def test_client_resume_multiple_paths(ci):
    print "test_client_resume_multiple_paths"
    ci.delete_all()     
    defs = create_defs("test_client_resume_multiple_paths")  
    suite = defs.find_suite("test_client_resume_multiple_paths")
    suite.add_variable("ECF_DUMMY_TASK","")
        
    ci.load(defs)  
    ci.begin_all_suites()  
    
    path_list = [ "/test_client_resume_multiple_paths/f1/t1", "/test_client_resume_multiple_paths/f1/t2" ]
    ci.suspend( path_list )  
  
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/test_client_resume_multiple_paths/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/test_client_resume_multiple_paths/f1/t2")
    assert task_t1.is_suspended(), "Expected to find task t1 to be suspended"
    assert task_t2.is_suspended(), "Expected to find task t2 to be suspended"

    ci.resume( path_list )  
    ci.sync_local();
    task_t1 = ci.get_defs().find_abs_node("/test_client_resume_multiple_paths/f1/t1")
    task_t2 = ci.get_defs().find_abs_node("/test_client_resume_multiple_paths/f1/t2")
    assert task_t1.is_suspended() == False, "Expected to find task t1 to be resumed"
    assert task_t2.is_suspended() == False, "Expected to find task t2 to be resumed"
 
          
def test_client_delete_node(ci): 
    print "test_client_delete_node"
    ci.delete_all() 
    defs = create_defs("test_client_delete_node")
    
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
    
def test_client_delete_node_multiple_paths(ci): 
    print "test_client_delete_node_multiple_paths"
    ci.delete_all() 
    defs = create_defs("test_client_delete_node_multiple_paths")
    
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

    # server independent tests
    test_set_host_port();
    
    # =========================================================================
    # server dependent tests
    seed_port = 3151
    if debug_build(): seed_port = 3150
    
    lock_file = EcfPortLock()
    global the_port
    the_port = lock_file.find_free_port(seed_port)   
    print "Creating client: on port " + the_port
     
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

        test_version(ci)
        PrintStyle.set_style( Style.STATE ) # show node state 
        test_client_get_server_defs(ci)             
        test_client_new_log(ci, the_port)             
        test_client_clear_log(ci, the_port)             
        test_client_log_msg(ci, the_port)             
        
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
        #test_client_plug(ci)             
        test_client_alter_add(ci) 
        test_client_alter_delete(ci) 
        test_client_alter_change(ci) 
                
        test_client_force(ci)             
        test_client_replace(ci,False)             
        test_client_replace(ci,True)             

        #test_client_kill(ci)             
        #test_client_status(ci)             
        #test_client_order(ci)             
        #test_client_group(ci)             
        test_client_suspend(ci)             
        test_client_suspend_multiple_paths(ci)             
        test_client_resume(ci)             
        test_client_resume_multiple_paths(ci)             
        test_client_delete_node(ci)             
        test_client_delete_node_multiple_paths(ci)             

        test_client_check(ci)  
 
        test_client_stats(ci)             
        test_client_debug_server_on_off(ci)             

        print "All Tests pass ======================================================================"    
    finally:
        print "Finally, Kill the server, clean up log file, check pt files and lock files used in test"
        ci.terminate_server()  
        clean_up(the_port)
        lock_file.remove(the_port)
