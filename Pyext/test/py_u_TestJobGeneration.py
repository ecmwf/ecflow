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

# This code is used check job generation
#
import os
import shutil   # used to remove directory tree

import ecflow_test_util as Test
from ecflow import Defs, JobCreationCtrl, TaskVec, File, Client, debug_build

def ecf_includes() : return File.source_dir() + "/Pyext/test/data/includes"

def suite_name(): return "suite_job_gen_" + str(os.getpid())  # allow python2 and python3 to run in parallel

def create_defs(ecf_home,task_vec):
    defs = Defs();
    suite = defs.add_suite( suite_name() );
    suite.add_variable("ECF_HOME",ecf_home);
    suite.add_variable("ECF_CLIENT_EXE_PATH",File.find_client());
    suite.add_variable("SLEEPTIME","1");
    suite.add_variable("ECF_INCLUDE",ecf_includes());
    
    fam =  suite.add_family("family")
    t1 = fam.add_task("t1")
    t1.add_event("eventname");
    t1.add_meter("meter",0,100,100)
    
    t2 = fam.add_task("t2")
    t3 = fam.add_task("t3")
    t4 = fam.add_task("dummy_task")
    t4.add_variable("ECF_DUMMY_TASK","any"); # stop job generation errors for tasks without an .ecf
    task_vec.append(t1);
    task_vec.append(t2);
    task_vec.append(t3);
    task_vec.append(t4);
    
    # Create dir. **NOTE**, if the second component of os.path.joind(..) is absolute, 
    # the first path is ignored, hence remove first char
    dir_to_create = os.path.join(str(ecf_home),fam.get_abs_node_path()[1:])
    print("Creating dir ",dir_to_create)
    os.makedirs( dir_to_create  )
     
    # copy ecf files from ecflow source, to create unique file per process
    src_t1  = os.path.join(ecf_home, "suite_job_gen" , "family" , "t1.ecf")
    dest_t1 = os.path.join(ecf_home,  t1.get_abs_node_path()[1:] + ".ecf" )
    src_t2  = os.path.join(ecf_home, "suite_job_gen" , "family" , "t2.ecf")
    dest_t2 = os.path.join(ecf_home,  t2.get_abs_node_path()[1:] + ".ecf" )
    src_t3  = os.path.join(ecf_home, "suite_job_gen" , "family" , "t3.ecf")
    dest_t3 = os.path.join(ecf_home,  t3.get_abs_node_path()[1:] + ".ecf" )
 
    shutil.copy(src_t1, dest_t1)
    shutil.copy(src_t2, dest_t2)
    shutil.copy(src_t3, dest_t3)
    
    return defs


def delete_jobs(task_vec, ecf_home):
    print("delete jobs")
    for task in task_vec:
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
        if os.path.exists(the_job_file) :
            print("removing file " + the_job_file)
            try: os.remove(the_job_file)
            except: pass
        man_file = ecf_home + task.get_abs_node_path() + ".man"  
        if os.path.exists(man_file) :
            print("removing man_file " + man_file)
            try: os.remove(man_file)
            except: pass
   
def check_jobs(task_vec, ecf_home):
    print("Check job file exists")
    for task in task_vec:
        variable = task.find_variable("ECF_DUMMY_TASK")
        if not variable.empty(): continue;
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
      
        if os.path.exists(the_job_file) :
            print("Found job file " + the_job_file)
        else:
            assert False, "Could not find job file " + the_job_file


if __name__ == "__main__":
    print("####################################################################")
    print("Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")")
    print("####################################################################")
    
    workspace = File.source_dir();
    print(workspace)

    ecf_home = workspace + "/Pyext/test/data/ECF_HOME"
    task_vec = TaskVec()
    defs = create_defs(ecf_home,task_vec)
    print(str(defs))

    print("Generate jobs for *ALL* tasks, to default locations ECF_HOME/ECF_NAME.job0") 
    print(defs.check_job_creation())   
    check_jobs(task_vec,ecf_home)
    delete_jobs(task_vec,ecf_home)
       
    print("\nGenerate jobs for *ALL* tasks, to default locations ECF_HOME/ECF_NAME.job0")
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation( job_ctrl )       
    print(job_ctrl.get_error_msg())
    check_jobs(task_vec,ecf_home)
    delete_jobs(task_vec,ecf_home)
    
    print("\nGenerate jobs for all nodes, under path, to default locations ECF_HOME/ECF_NAME.job0")    
    job_ctrl = JobCreationCtrl()
    job_ctrl.set_node_path( task_vec[0].get_abs_node_path() )    
    defs.check_job_creation(job_ctrl)       
    print(job_ctrl.get_error_msg());
    delete_jobs(task_vec,ecf_home)
   
    print("\nGenerate jobs for all tasks, to the specified directory")
    # Directory will automatically created under the provided directory
    job_ctrl = JobCreationCtrl()
    job_ctrl.set_dir_for_job_creation(workspace + "/Pyext/test/data")  # generate jobs file under this directory
    defs.check_job_creation(job_ctrl)
    print(job_ctrl.get_error_msg())
    
    generated_dir = job_ctrl.get_dir_for_job_creation() + "/" + suite_name()
    print("removing directory tree " + generated_dir)
    shutil.rmtree(generated_dir)      
 
    # Create jobs for all task, to a TMP directory ($TMPDIR/ecf_check_job_creation/ECF_NAME.job0
    # When run via rsh then TMPDIR may not be defined, hence expect exception to be thrown
    try:
        job_ctrl = JobCreationCtrl()                    
        job_ctrl.generate_temp_dir()       
        defs.check_job_creation(job_ctrl)
        print(job_ctrl.get_error_msg())
        print("removing directory tree " + job_ctrl.get_dir_for_job_creation())
        shutil.rmtree(job_ctrl.get_dir_for_job_creation())     
        delete_jobs(task_vec,ecf_home)
    except RuntimeError as e:
        print("failed: " + str(e))  
        
    try:
        dir_to_del = os.path.join(ecf_home,suite_name())
        print("removing directory tree " + dir_to_del )
        shutil.rmtree(dir_to_del)     
        print("All test pass")
    except RuntimeError as e:
        print("failed: " + str(e))    
