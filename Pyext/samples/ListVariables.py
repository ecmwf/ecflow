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
    
    DESC = """Will list variables for any node
              Usage:
                Example1: List all the server variables
                   ListVariables.py --host cca --port 4141 --path /
                   
                Example2: List the variables for the given node
                   ListVariables.py --host cca --port 4141 --path /path/to/node
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default="localhost",   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default="3141",   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--path', default="/",   
                        help="The path to the node. i.e /suite/family/task")
    ARGS = PARSER.parse_args()
    #print ARGS    
    
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
            
        # print defs;
        if ARGS.path == "/":
            for var in defs.server_variables:  print "edit " + var.name() + " '" + var.value() + "'"
            for var in defs.user_variables:    print "edit " + var.name() + " '" + var.value() + "'"
        else:
            node = defs.find_abs_node(ARGS.path)
            if node == None:  
                print "No node found at path " + ARGS.path
            else:
                for var in node.variables:     print "edit " + var.name() + " '" + var.value() + "'"

    except RuntimeError, ex:
        print "Error: " + str(ex)
        print "Check host and port number are correct."
