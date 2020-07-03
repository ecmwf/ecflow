#!/usr/bin/env python2.7
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2020 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

import sys
sys.path.append("../../../admin/2.0")
import rest
import argparse

PARSER = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
PARSER.add_argument('--file', help="Tar file to upload")
ARGS = PARSER.parse_args()
    
conflunce_base_url = "https://software-test.ecmwf.int/wiki"
confluence = rest.Confluence(conflunce_base_url,"deploy","deploy2013")
page_id = confluence.get_page_id("ECFLOW","Releases") 
if page_id is None:
    sys.exit(1)

attachment = confluence.get_attachment_id(page_id,ARGS.file)
if attachment is not None:
    print("space key:",space_key," allready has an attachment for file ",ARGS.file)
else:
    confluence.create_attachment(page_id,"%TARBALL_COMMENT%",ARGS.file)
    
#     # ===========================================================================
#     CL = ecflow.Client("localhost", 4141)
#     try:
#         CL.ping() 
#         print "ping worked";
#         
#         alter = 'alter add variable X "X XX" /ecflow/test_local_actions; alter add variable xx "y a" /ecflow/test_local_actions;'
#         CL.group(alter)
#  
#     except RuntimeError, ex:
#         print "Error: " + str(ex)
