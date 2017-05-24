#!/usr/bin/env python2.7
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

# =============================================================================
# Code for testing *any* definition
#   Since any ad hoc definition will reference local directories in the
#   ECF_ variables, we need to remove them and inject our own.
#
#   This test was created aid the performance testing of job generation
#   It is tied to ANode/parser/test/TestJobGenPerf.cpp
# =============================================================================
import ecflow
import os       # for getenv
import shutil   # used to remove directory tree
import argparse # for argument parsing     

def delete_variables_affecting_job_generation(node): 
    """delete customer related ECF variables, these will point to directories
       that don't exist. Its ok we will regenerate our own local ones"""
    var = node.find_variable("ECF_HOME")
    if not var.empty() :  
        node.delete_variable("ECF_HOME")    
    var = node.find_variable("ECF_FILES")
    if not var.empty() :  
        node.delete_variable("ECF_FILES")    
    var = node.find_variable("ECF_INCLUDE")
    if not var.empty() :  
        node.delete_variable("ECF_INCLUDE")    
    var = node.find_variable("ECF_JOB_CMD")
    if not var.empty() :  
        node.delete_variable("ECF_JOB_CMD")    
    var = node.find_variable("ECF_KILL_CMD")
    if not var.empty() :  
        node.delete_variable("ECF_KILL_CMD")    
    var = node.find_variable("ECF_STATUS_CMD")
    if not var.empty() :  
        node.delete_variable("ECF_STATUS_CMD")    
    var = node.find_variable("ECF_OUT")
    if not var.empty() :  
        node.delete_variable("ECF_OUT")    
      
def traverse_container(node_container):
    """Recursively traverse definition node hierarchy and delete
       the variables that affect job generation.
    """
    delete_variables_affecting_job_generation(node_container)
    for node in node_container.nodes:
        delete_variables_affecting_job_generation(node)
        if not isinstance(node, ecflow.Task):
            traverse_container(node)  
    
if __name__ == "__main__":
    
    DESC = """Will allow any definition to be loaded and played on the server
            This is done by:
            o Remove existing ECF_ variables that affect job generation. 
              i.e variables that refer to customer specific directories are removed
            o Allows ECF_HOME to specified, defaults to ./CUSTOMER/ECF_HOME
            o Generates the scripts(.ecf files) automatically based on the definition.
              i.e if a task has events,meters,labels then the client request for these are
              automatically injected in the generated .ecf script files
            o Will clear out existing data both on disk and on the server to allow 
              multiple re-runs of this script. ** If this is an issue please use
              a test server **
            This programs assumes that ecflow module is accessible.
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('defs_file', 
                        help="The definition file")
    PARSER.add_argument('--ecf_home', default="/var/tmp/ma0/ECFLOW_TEST/TestJobGenPerf/ECF_HOME",
                        help="Directory to be used for generated scripts(ECF_HOME), defaults to /var/tmp/ma0/ECFLOW_TEST/TestJobGenPerf/ECF_HOME")
    PARSER.add_argument('--verbose', nargs='?', default=False, const=True, type=bool,
                        help="Show verbose output")
    ARGS = PARSER.parse_args()
    ARGS.defs_file = os.path.expandvars(ARGS.defs_file) # expand references to any environment variables
    print ARGS    
    
    # If running on local work space, use /Pyext/test/data/CUSTOMER/ECF_HOME as ecf_home
    if not ARGS.ecf_home:
        if os.getenv("WK") == None:
            print "No ecf_home specified. Please specify a writable directory"
            exit(1)
        ARGS.ecf_home = "/var/tmp/ma0/ECFLOW_TEST/TestJobGenPerf/ECF_HOME"
        if ARGS.verbose:
            print "Workspace is defined" 
            print "using /Client/bin/gcc\-4.5/debug/ecflow_client"

    print "Using ECF_HOME=" + ARGS.ecf_home
    print "removing directory " + ARGS.ecf_home
    try:
        shutil.rmtree(ARGS.ecf_home)   
    except:
        pass   
        
    if not os.path.exists(ARGS.ecf_home): 
        print ARGS.ecf_home + " directory does not exist, creating"
        os.makedirs(ARGS.ecf_home)

    print "\nloading the definition from the input arguments(" + ARGS.defs_file + ")\n"
    
    try:
        DEFS = ecflow.Defs(ARGS.defs_file)
    except RuntimeError, ex:
        print "   ecflow.Defs(" + ARGS.defs_file + ") failed:\n" + str(ex)
        exit(1)
    
    if ARGS.verbose: 
        print "remove test data associated with the DEFS, so we start fresh, Allows rerun"
    for suite in DEFS.suites:
        dir_to_remove = ARGS.ecf_home + suite.get_abs_node_path()
        if ARGS.verbose: 
            print "   Deleting directory: " + dir_to_remove + "\n"
        shutil.rmtree(dir_to_remove, True)  
        
    if ARGS.verbose: 
        print "remove remote reference to ECF_HOME and ECF_INCLUDE, since we inject or own\n"
    for suite in DEFS.suites:
        print "add variables required for script generation, for all suites\n"
        traverse_container(suite)
  
        suite.add_variable("ECF_HOME", ARGS.ecf_home)
        suite.add_variable("ECF_INCLUDE", ARGS.ecf_home + "/includes")
        suite.add_variable("ECF_CLIENT_EXE_PATH", os.getenv("WK") + "/Client/bin/gcc\-4.8/debug/ecflow_client")
        suite.add_variable("SLEEP", "10")  # not strictly required since default is 1 second


    
    # ecflow.PrintStyle.set_style(ecflow.Style.STATE)
    #if ARGS.verbose: 
    #print DEFS
    DEFS.generate_scripts()

    if ARGS.verbose: 
        print "\nchecking script file generation, pre-processing & variable substitution\n"
    JOB_CTRL = ecflow.JobCreationCtrl()
    DEFS.check_job_creation(JOB_CTRL)       
    assert len(JOB_CTRL.get_error_msg()) == 0, JOB_CTRL.get_error_msg()
    
    
    newDefs = ARGS.ecf_home + "/../" + os.path.basename(ARGS.defs_file)
    print "Saving modified defs as " + newDefs 
    DEFS.save_as_defs(newDefs)

