#!/usr/bin/env python3.6
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

import ecflow
import os       # for getenv
import sys
import shutil   # used to remove directory tree
import argparse # for argument parsing     

def sync_local(ci):
    ci.sync_local()
    return ci.get_defs() 

def get_tasks_to_run(defs,paths):
    print("Collating tasks to run in batch")
    node_path_list = []
    for path in paths:
        node = defs.find_abs_node(path)
        if not node:
            print("Could not find node",path,"in the definition")
            continue
        if isinstance(node,ecflow.Task):
            node_path_list.append(node.get_abs_node_path())
        elif isinstance(node,ecflow.NodeContainer):
            node_vec = node.get_all_nodes()
            for n in node_vec:
                if isinstance(n,ecflow.Task):
                    node_path_list.append(n.get_abs_node_path())
             
    uniqueList = []
    for path in node_path_list:
        if path not in uniqueList:
            uniqueList.append(path)
            print(" found task",path)
           
    return uniqueList 
    
def chunks(list_to_break, chunk_size):
    for i in range(0, len(list_to_break), chunk_size):
        yield list_to_break[i:i + chunk_size]
        
def run_chunk(ci,list_of_task_paths):
    defs = sync_local(ci)
    CL.suspend(list_of_task_paths);


if __name__ == "__main__":
    
    DESC = """ 
            example:
                python3 libs/pyext/samples/run_batches_with_delay.py -h polonius -p 4141 -b 4 -s 4 -n "/ecflow/cron /limit_basic"
                The list of paths can be obtained from the GUI, using:
                sh $WK/libs/pyext/samples/run_batches_with_delay.sh -h %ECF_HOST% -p %ECF_PORT% -b 10 -s 2 -n '<full_name>'
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
    PARSER.add_argument('--host', default=default_host,   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default=default_port,   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--batch_size', default=10, type=int,
                        help="The number of jobs to submit in batch")
    PARSER.add_argument('--sleep_between_batch', default=5, type=int,  
                        help="The number of seconds to sleep between the next batch")
    PARSER.add_argument('--paths',
                        help="This list of node path to execute. These could be suites/families.")
    ARGS = PARSER.parse_args()
    print(ARGS )  
    
    paths = ARGS.paths.split()
    print(paths)
    
    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        print("check server " + ARGS.host + ":" + ARGS.port + " is running")
        CL.ping() 
        print("Server is running.")
         
        defs = sync_local(CL)
        node_path_list = get_tasks_to_run(defs,paths)
        
        list_of_chunks = (list(chunks(node_path_list,ARGS.batch_size)))
        for chunk in list_of_chunks:
            print(chunk)
            run_chunk(CL,chunk)
        
    except RuntimeError as ex:
        print("Error: " + str(ex))
