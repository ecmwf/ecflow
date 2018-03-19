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

import ecflow
import os       # for getenv
import sys
import shutil   # used to remove directory tree
import argparse # for argument parsing     

if __name__ == "__main__":
    DESC = """Will list variables for any node"""
    
    # ===========================================================================
    CL = ecflow.Client("localhost", 4141)
    try:
        CL.ping() 
        print "ping worked";
        
        alter = "alter add variable X XXX /ecflow/test_local_actions; alter add variable xx y /ecflow/test_local_actions;"
        alter += "alter add inlimit limit /ecflow/test_local_actions"
        CL.group(alter)
 
    except RuntimeError, ex:
        print "Error: " + str(ex)
