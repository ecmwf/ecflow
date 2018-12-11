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
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#  code for testing client code in python
import time
import os
import pwd
import sys  # determine python version
from datetime import datetime
import shutil   # used to remove directory tree

from ecflow import Defs, Clock, DState,  Style, State, PrintStyle, File, Client, SState, File, debug_build
import ecflow_test_util as Test


def ecf_includes() : return File.source_dir() + "/Pyext" + "/test/data/python_includes"

def create_defs(name,the_port):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    suite = defs.add_suite(suite_name);
    
    ecfhome = Test.ecf_home(the_port);
    suite.add_variable("ECF_HOME", ecfhome);
    suite.add_variable("ECF_INCLUDE", ecf_includes());
    
    prefix_job_cmd = ""
    if 'PYTHONPATH' in os.environ:
        prefix_job_cmd = "export PYTHONPATH=" + os.environ['PYTHONPATH'] +";" 
    if 'LD_LIBRARY_PATH' in os.environ:
        prefix_job_cmd += "export LD_LIBRARY_PATH=" + os.environ['LD_LIBRARY_PATH'] +";"
   
    # sys.version_info is a tuple containing (major,minor,micro,releaselevel,serial)
    # releaselevel = alpha beta candidate final
    if (sys.version_info > (3, 0)):
        suite.add_variable("ECF_JOB_CMD",prefix_job_cmd + "python3 %ECF_JOB% 1> %ECF_JOBOUT% 2>&1")
    else:
        suite.add_variable("ECF_JOB_CMD",prefix_job_cmd + "python %ECF_JOB% 1> %ECF_JOBOUT% 2>&1")

    family = suite.add_family("f1")
    task = family.add_task("t1")
    task.add_event("event_fred")
    task.add_meter("meter", 0, 100)
    task.add_label("label_name", "value")
    task.add_queue("q1",["1","2","3"])

    family.add_task("t2")  # test wait
    family.add_task("t3").add_trigger(task.get_abs_node_path() + ":q1 >= 3") # wait on queue q1
 
    return defs;
    
def wait_for_suite_to_complete(ci,suite_name):
    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        suite = ci.get_defs().find_suite(suite_name)
        assert suite != None, " Expected to find suite " + suite_name + ":\n" + str(ci.get_defs())
        if suite.get_state() == State.complete:
            break;
        if suite.get_state() == State.aborted:
            print(ci.get_defs());
            assert False," Suite aborted \n"  
        time.sleep(2)
        if count > 14:
            assert False, suite_name + " aborted after " + str(count) + " loops, printing defs:\n" + str(ci.get_defs())
        
    ci.log_msg("Looped " + str(count) + " times")
    
def test_client_run(ci):            
    print("\ntest_client_run " + ci.get_host() + ":" + str(ci.get_port()))
    print(" ECF_HOME(" + Test.ecf_home(ci.get_port()) + ")")
    print(" ECF_INCLUDES(" + ecf_includes() + ")")
    ci.delete_all(True)     
    defs = create_defs("test_client_run",ci.get_port())  
    suite = defs.find_suite("test_client_run")
    suite.add_defstatus(DState.suspended)

    # create the ecf file /test_client_run/f1/t1
    ecf_home = Test.ecf_home(ci.get_port())
    dir = ecf_home + "/test_client_run/f1"
    
    # on cray creating recursive directories can fail, try again. yuk
    try:
        if not os.path.exists(dir): os.makedirs(dir)
    except:
        try:
            if not os.path.exists(dir): os.makedirs(dir)
        except:
            # try breaking down
            if not os.path.exists(ecf_home): os.makedirs(ecf_home)
            new_dir = ecf_home + "/test_client_run"
            if not os.path.exists(new_dir): os.makedirs(new_dir)
            new_dir = ecf_home + "/test_client_run/f1"
            if not os.path.exists(new_dir): os.makedirs(new_dir)
    if not os.path.exists(dir): os.makedirs(dir)

    server_version = ci.server_version()
    file = dir + "/t1.ecf"
    contents = "%include <head.py>\n\n"
    contents += "with Client() as ci:\n"
    contents += "    print('doing some work: t1.ecf')\n"
    contents += "    if ci.version() != '" + server_version + "':\n"
    contents += "        assert False, 'Client and server versions different'\n"
    contents += "    ci.child_event('event_fred')\n"
    contents += "    ci.child_meter('meter',100)\n"
    contents += "    ci.child_label('label_name','100')\n"
    contents += "    step = ci.child_queue('q1','active')\n"
    contents += "    assert step == '1','expected first step to be 1'\n"
    contents += "    step = ci.child_queue('q1','complete',step)\n"
    contents += "    step = ci.child_queue('q1','active')\n"
    contents += "    assert step == '2','expected second step to be 2'\n"
    contents += "    step = ci.child_queue('q1','complete',step)\n"
    contents += "    step = ci.child_queue('q1','active')\n"
    contents += "    assert step == '3','expected third step to be 3'\n"
    contents += "    step = ci.child_queue('q1','complete',step)\n"
    contents += "    step = ci.child_queue('q1','active')\n"
    contents += "    assert step == '<NULL>','expected <NULL? for end of queue'\n"
    contents += "    print('Finished event,meter,label and queue child commands')\n"
    open(file,'w').write(contents)
    print(" Created file " + file)
      
    # create the ecf file /test_client_run/f1/t2
    file = dir + "/t2.ecf"
    contents = "%include <head.py>\n\n"
    contents += "with Client() as ci:\n"
    contents += "    print('Waiting for /test_client_run/f1/t1 == complete')\n"
    contents += "    ci.child_wait('/test_client_run/f1/t1 == complete')\n"
    contents += "    print('Finished waiting')\n"
    open(file,'w').write(contents)
    print(" Created file " + file)
    
    # create the ecf file /test_client_run/f1/t3
    file = dir + "/t3.ecf"
    contents = "%include <head.py>\n\n"
    contents += "with Client() as ci:\n"
    contents += "    print('Running t3.ecf')\n"
    open(file,'w').write(contents)
    print(" Created file " + file)
        
    ci.restart_server()
    ci.load(defs)          
    ci.begin_all_suites()
    print(" Running the test, wait for suite to complete ...")  
    ci.run("/test_client_run", False)

    wait_for_suite_to_complete(ci,"test_client_run");
    
    if not Test.debugging():
        dir_to_remove = Test.ecf_home(ci.get_port()) + "/" + "test_client_run"
        print((" Test OK: removing directory " , dir_to_remove))
        shutil.rmtree(dir_to_remove)      
        
# //////////////////////////////////////////////////////////////////////////////////////        

if __name__ == "__main__":
    Test.print_test_start()

    with Test.Server() as ci:
    
        server_version = ci.server_version();
        print("Running ecflow server version " + server_version)
        assert ci.version() == server_version, "Client version not same as server version"

        PrintStyle.set_style( Style.STATE ) # show node state 
        test_client_run(ci)  
        print("\nAll Tests pass ======================================================================")    
