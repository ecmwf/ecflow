#!/usr/bin/env python2.7
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
import ecflow
import argparse # for argument parsing     

if __name__ == "__main__":
    
    DESC = """Will resume any suspended 'node' who name matches input
              Usage:
                Example1: resume all suspended node whose name matches 'fred' for suite grib_api
                   resume.py --host cca --port 4141 --suite grib_api --name fred
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default="localhost",   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default="3141",   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--suite',   
                        help="The name of the suite")
    PARSER.add_argument('--name', default="install",   
                        help="The name of the node")
    ARGS = PARSER.parse_args()
    print ARGS    
     
    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        CL.ping() 

        # get the incremental changes, and merge with defs stored on the Client 
        CL.sync_local()
        
        # check to see if definition exists in the server
        defs = CL.get_defs()
        if defs == None :
            print "No definition found, exiting..."
            exit(0) 
         
        paths_list = []
        node_vec = defs.get_all_nodes()
        for node in node_vec:
            if node.name() != ARGS.name: continue
            if node.get_dstate() != ecflow.DState.suspended: continue

            path = node.get_abs_node_path()
            paths = path.split('/')
            if paths[1] == ARGS.suite:
                paths_list.append(path) 
                
        if len(paths_list) > 0:
            CL.resume(paths_list)

#        # requires ecflow >= 4.0.8
#         suite = defs.find_suite(ARGS.suite)
#         if suite != None:
#             paths_list = []
#             node_vec = suite.get_all_nodes()
#             for node in node_vec:
#                 if node.name() == ARGS.name and node.get_dstate() == DState.SUSPENDED:
#                     paths_list.append(node.get_abs_node_path())
# 
#             print paths_list
#             if len(paths_list) > 0:
#                  CL.resume(paths_list)
        
    except RuntimeError, ex:
        print "Error: " + str(ex)
        print "Check host and port number are correct."
