#!/usr/bin/env python2.7

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import ecflow
import argparse # for argument parsing  
import sys   

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
        if len(defs) == 0 :
            print("No suites found, exiting...")
            sys.exit(0) 
            
        # print defs;
        if ARGS.path == "/":
            for var in defs.server_variables:  print("edit " + var.name() + " '" + var.value() + "'")
            for var in defs.user_variables:    print("edit " + var.name() + " '" + var.value() + "'")
        else:
            node = defs.find_abs_node(ARGS.path)
            if node is None:  
                print("No node found at path " + ARGS.path)
            else:
                for var in node.variables:     print("edit " + var.name() + " '" + var.value() + "'")

    except RuntimeError as ex:
        print("Error: " + str(ex))
        print("Check host and port number are correct.")
