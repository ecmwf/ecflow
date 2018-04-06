#!/usr/bin/env python2.7

import sys
sys.path.append("../../../admin/2.0")
import rest

import argparse
import sys

PARSER = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
PARSER.add_argument('--file', help="Tar file to upload")
ARGS = PARSER.parse_args()
    
conflunce_base_url = "https://software-test.ecmwf.int/wiki"
confluence = rest.Confluence(conflunce_base_url,"deploy","deploy2013")
page_id = confluence.get_page_id("ECFLOW","Releases") 
if page_id == None:
    exit(1)

attachment = confluence.get_attachment_id(page_id,ARGS.file)
if attachment != None:
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