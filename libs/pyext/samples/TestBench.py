#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009- ECMWF.
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
#   This script is re-runnable, and hence will delete suites in the server
#   matching those in the input definition. Hence it is best to use this 
#   script with a *test* server to avoid accidentally deleting existing suites 
#   of the same name.
# =============================================================================
import ecflow
import os       # for getenv
import sys
import shutil   # used to remove directory tree
import argparse # for argument parsing     

def get_root_source_dir():
    cwd = os.getcwd()
    #print "get_root_source_dir from: " + cwd
    while (1):
        # Get to directory that has ecflow
        head, tail = os.path.split(cwd)
        #print "   head:" + head
        #print "   tail:" + tail
        if tail.find("ecflow") != -1 :
            
            # bjam, already at the source directory
            if os.path.exists(cwd + "/Jamroot.jam"): 
                print("   Found Jamroot.jam in " + cwd)
                return cwd
        
        if tail != "pyext" and tail != "migrate":
            # in cmake, we may be in the build directory, hence we need to determine source directory
            file = cwd + "/CTestTestfile.cmake"
            #print "   searching for " + file
            if os.path.exists(file):
                # determine path by looking into this file:
                with open(file) as fp:
                    for line in fp:
                        ## Source directory: /tmp/ma0/workspace/ecflow/Acore
                        if line.find("Source directory"):
                            tokens = line.split()
                            if len(tokens) == 4:
                                #print "   returning root_source_dir:", tokens[3]
                                return tokens[3]
                raise RuntimeError("ERROR could not find Source directory in CTestTestfile.cmake")
            else:
                raise RuntimeError("ERROR could not find file CTestTestfile.cmake in " + cwd)
                
        cwd = head
    return cwd

    
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
    var = node.find_variable("ECF_MICRO")
    if not var.empty() :  
        node.delete_variable("ECF_MICRO")   # generation depend on % 
         
      
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
            o All suites are put into a suspended state. This allows the GUI to resume them
            o The server is restarted and suites are begun
            This programs assumes that ecflow module is accessible

            example:
                python libs/pyext/samples/TestBench.py --port=3141 --verbose=True libs/node/parser/test/data/good_defs/trigger/late.def
            """    
            
    print("####################################################################")
    print("Running ecflow version " + ecflow.Client().version()  + " debug build(" + str(ecflow.debug_build()) +")")
    if 'PYTHONPATH' in os.environ:
        print("PYTHONPATH: " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    print("sys.path:   " + str(sys.path))
    print("####################################################################")
 
    default_port = "3141"
    if "ECF_PORT" in os.environ:
         default_port = os.environ["ECF_PORT"]
 
    default_host = "localhost"
    if "ECF_HOST" in os.environ:
        default_host  = os.environ["ECF_HOST"]

    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('defs_file', 
                        help="The definition file")
    PARSER.add_argument('--host', default=default_host,   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default=default_port,   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--path', default="/",   
                        help="replace only the node path in the suite")
    PARSER.add_argument('--ecf_home', default=os.getcwd() + "/CUSTOMER/ECF_HOME",
                        help="Directory to be used for generated scripts(ECF_HOME), defaults to ./CUSTOMER/ECF_HOME")
    PARSER.add_argument('--verbose', nargs='?', default=False, const=True, type=bool,
                        help="Show verbose output")
    ARGS = PARSER.parse_args()
    ARGS.defs_file = os.path.expandvars(ARGS.defs_file) # expand references to any environment variables
    print(ARGS  )  
    
    # If running on local work space, use /libs/pyext/test/data/CUSTOMER/ECF_HOME as ecf_home
    using_workspace = False;
    ecflow_source_dir = ""
    try:
        ecflow_source_dir = get_root_source_dir();
        ARGS.ecf_home = ecflow_source_dir + "/libs/pyext/test/data/CUSTOMER/ECF_HOME"
        using_workspace = True
        if ARGS.verbose:
            print("Workspace is defined ecflow_source_dir: ",ecflow_source_dir)
                
    except:
        pass
    
    if ARGS.verbose:
        print("Using ECF_HOME=" + ARGS.ecf_home)
         
    try:
        if ARGS.verbose: 
            print("\nloading the definition from the input arguments(" + ARGS.defs_file + ")\n")
        DEFS = ecflow.Defs(ARGS.defs_file)
    except RuntimeError as ex:
        print("   ecflow.Defs(" + ARGS.defs_file + ") failed:\n" + str(ex))
        sys.exit(1)
    
    if ARGS.verbose: 
        print("remove test data associated with the DEFS, so we start fresh, Allows rerun")
    for suite in DEFS.suites:
        dir_to_remove = ARGS.ecf_home + suite.get_abs_node_path()
        if ARGS.verbose: 
            print("   Deleting directory: " + dir_to_remove + "\n")
        shutil.rmtree(dir_to_remove, True)  
        
    if ARGS.verbose: 
        print("remove remote reference to ECF_HOME and ECF_INCLUDE, since we inject or own\n")
    for suite in DEFS.suites:
        traverse_container(suite)
  
    DEFS.add_variable("ECF_HOME", ARGS.ecf_home)
    
    if using_workspace: 
        path_to_ecflow_client = ecflow.File.find_client()
        if os.path.exists( path_to_ecflow_client ):
            DEFS.add_variable("ECF_CLIENT_EXE_PATH", path_to_ecflow_client )
            if ARGS.verbose: print("Adding ECF_CLIENT_EXE_PATH:",path_to_ecflow_client)

    DEFS.add_variable("SLEEP", "10")  # not strictly required since default is 1 second
    DEFS.add_variable("ECF_INCLUDE", ARGS.ecf_home + "/includes")

    for suite in DEFS.suites:
        suite.add_defstatus(ecflow.DState.suspended)
    
    if ARGS.verbose: 
        #ecflow.PrintStyle.set_style(ecflow.Style.STATE)
        print(DEFS)

    if ARGS.verbose: 
        print("Generating script files(.ecf) from the definition")
    DEFS.generate_scripts()

    if ARGS.verbose: 
        print("\nchecking script file generation, pre-processing & variable substitution\n")
    msg = DEFS.check_job_creation()       
    assert len(msg) == 0, msg
    
    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        if ARGS.verbose: 
            print("check server " + ARGS.host + ":" + ARGS.port + " is running")
        CL.ping() 

        if ARGS.verbose: 
            print("Server is already running. re-start the server")
        CL.restart_server() 

        if ARGS.verbose: 
            print("Remove suites associated with this DEFS, allows rerun *******************************************")
        for suite in DEFS.suites:
            try:
                CL.delete(suite.get_abs_node_path(), True)
            except RuntimeError as ex:
                pass # For first run this will fail, hence ignore
        
        if ARGS.verbose: 
            print("Load the definition into " + ARGS.host + ":" + ARGS.port)
        if ARGS.path == "/":
            CL.load(DEFS) 
        else:
            CL.replace(ARGS.path, DEFS)

        if ARGS.verbose: 
            print("Begin all suites. They should be suspended.")
        print("Loaded suites:")
        for suite in DEFS.suites:
            CL.begin_suite(suite.name())
            print("   " + suite.name())
        print("into server " + ARGS.host  + ":" + ARGS.port + ", please view the playable suites in the GUI")
    except RuntimeError as ex:
        print("Error: " + str(ex))
