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
import argparse # for argument parsing  
import sys  
import os
    
if __name__ == "__main__":
    DESC = """query node state
              Usage:
                   query.py --host cca --port 4141 --path /ecflow/regenerate
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default=os.getenv("ECF_HOST","localhost"),   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default=os.getenv("ECF_PORT","3141"),   
                        help="The port on the host, defaults to 3141")
    PARSER.add_argument('--path', default="/",   
                        help="The path to the node. i.e /suite/family/task")
    ARGS = PARSER.parse_args()
    
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        CL.sync_local()
        defs = CL.get_defs()        
        if len(defs) == 0 :
            print("No suites found, exiting...")
            sys.exit(0) 
        if ARGS.path == "/":
            print(defs.get_state())
        else:
            node = defs.find_abs_node(ARGS.path)
            if node is None:  
                print("No node found at path " + ARGS.path)
            else:
                print(node.get_state())
    except RuntimeError as ex:
        print("Error: " + str(ex))
        print("Check host and port number are correct.")
