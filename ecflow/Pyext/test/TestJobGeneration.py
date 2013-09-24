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

# This code is used check job generation
#
import os
import shutil   # used to remove directory tree

from ecflow import Defs, JobCreationCtrl, TaskVec, File


def create_defs(ecf_home,task_vec):
    defs = Defs();
    suite = defs.add_suite("suite_job_gen");
    suite.add_variable("ECF_HOME",ecf_home);
    suite.add_variable("ECF_CLIENT_EXE_PATH",File.find_client());
    suite.add_variable("SLEEPTIME","1");
    suite.add_variable("ECF_INCLUDE",ecf_home + "/includes");
    
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
    
    return defs


def delete_jobs(task_vec, ecf_home):
    print "delete jobs"
    for task in task_vec:
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
        if os.path.exists(the_job_file) :
            print "removing file " + the_job_file
            os.remove(the_job_file)
        man_file = ecf_home + task.get_abs_node_path() + ".man"  
        if os.path.exists(man_file) :
            print "removing man_file " + man_file
            os.remove(man_file)
   
def check_jobs(task_vec, ecf_home):
    print "Check job file exists"
    for task in task_vec:
        variable = task.find_variable("ECF_DUMMY_TASK")
        if not variable.empty(): continue;
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
      
        if os.path.exists(the_job_file) :
            print "Found job file " + the_job_file
        else:
            assert False, "Could not find job file " + the_job_file
   
if __name__ == "__main__":
 
    cwd = os.getcwd()
    #print cwd
    ecf_home = cwd + "/test/data/ECF_HOME"
    task_vec = TaskVec()
    defs = create_defs(ecf_home,task_vec)
    print str(defs)

    # Generate jobs for *ALL* tasks, to default locations ECF_HOME/ECF_NAME.job0 
    print defs.check_job_creation()   
    check_jobs(task_vec,ecf_home)
    delete_jobs(task_vec,ecf_home)
       
    # Generate jobs for *ALL* tasks, to default locations ECF_HOME/ECF_NAME.job0 
    job_ctrl = JobCreationCtrl()
    defs.check_job_creation( job_ctrl )       
    print job_ctrl.get_error_msg()
    check_jobs(task_vec,ecf_home)
    delete_jobs(task_vec,ecf_home)
    
    # Generate jobs for all nodes, under path, to default locations ECF_HOME/ECF_NAME.job0     
    job_ctrl = JobCreationCtrl()
    job_ctrl.set_node_path( task_vec[0].get_abs_node_path() )    
    defs.check_job_creation(job_ctrl)       
    print job_ctrl.get_error_msg();
    delete_jobs(task_vec,ecf_home)
   
   
    # Generate jobs for all tasks, to the specified directory
    # Directory will automatically created under the provided directory
    job_ctrl = JobCreationCtrl()
    job_ctrl.set_dir_for_job_creation(cwd + "/test/data")  # generate jobs file under this directory
    defs.check_job_creation(job_ctrl)
    print job_ctrl.get_error_msg()
    
    generated_dir = job_ctrl.get_dir_for_job_creation() + "/suite_job_gen"
    print "removing directory tree " + generated_dir
    shutil.rmtree(generated_dir)      
 
 
    # Create jobs for all task, to a TMP directory ($TMPDIR/ecf_check_job_creation/ECF_NAME.job0
    # When run via rsh then TMPDIR may not be defined, hence expect exception to be thrown
    try:
        job_ctrl = JobCreationCtrl()                    
        job_ctrl.generate_temp_dir()       
        defs.check_job_creation(job_ctrl)
        print job_ctrl.get_error_msg()
        print "removing directory tree " + job_ctrl.get_dir_for_job_creation()
        shutil.rmtree(job_ctrl.get_dir_for_job_creation())     
    
        delete_jobs(task_vec,ecf_home)
    except RuntimeError, e:
        print "failed: " + str(e)    
