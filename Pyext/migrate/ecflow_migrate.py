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

import os       # for getenv
import shutil   # used to remove directory tree
import argparse # for argument parsing     

def ensure_start_abort_and_end_abort_on_sameline(list_of_all_theLines, output_lines):
   migrated = False
   start_line_append = ""
   for line in list_of_all_theLines:
       # process line
       end_abort_pos = line.find(">abort")
       task_pos = line.find("task")
       if task_pos != -1:
           start_abort_pos = line.find("abort<:")
           if start_abort_pos != -1 and end_abort_pos != -1:
               # start and end found on same line. This is the correct format
               start_line_append = ""
            
           if start_abort_pos != -1 and end_abort_pos == -1:
               print "start abort found with no end abort"
               start_line_append = line
               start_line_append = start_line_append.rstrip('\n')
               start_line_append += " "
               migrated = True
               continue

       if len(start_line_append) > 0:
           start_line_append += line
           if end_abort_pos != -1:
               output_lines.append(start_line_append) # preserve'\n' for this case                   
               # print start_line_append
               start_line_append = "";
           else:
               start_line_append = start_line_append.rstrip('\n')
               start_line_append += " "

           continue;
              
       output_lines.append(line)
       
   return migrated
     
def version_number_as_integer(list_of_all_theLines, default_version):
    """Determine the version number used in ecflow_client --migrate
       This should be in the very first line
       # 3.1.8
    """
    if len(list_of_all_theLines) > 0:
        tokens = list_of_all_theLines[0].split()
        if len(tokens) > 1:
            version = (int)(tokens[1].replace(".",""))
            # print "******************* found version " + str(version)
            return version;
    return default_version
    
def do_migrate(defs_file):
    migration_count = 0
    fileName, fileExtension = os.path.splitext(defs_file)
    output_file_name = fileName + ".mig";
    
    input_file_object = open(defs_file,'r')
    output_file_object = open(output_file_name,'w')
    try:
        list_of_output_lines = [] 
        list_of_all_theLines = input_file_object.readlines()  # preserves new lines, we want this
        
        version = version_number_as_integer(list_of_all_theLines,318) 
        if version < 318:
            # This fix is only applicable for versions < 3.1.8
            migrated = ensure_start_abort_and_end_abort_on_sameline(list_of_all_theLines,list_of_output_lines)
            if migrated : migration_count = migration_count + 1
        else:
            print "***** NO migration required for " + ARGS.defs_file + " version " + str(version)
            list_of_output_lines = list_of_all_theLines
            
        # Add any future migration bug fixes here.
        # ... 
        
        output_file_object.writelines(list_of_output_lines)
    finally:
        input_file_object.close()
        output_file_object.close()
    return migration_count

if __name__ == "__main__":
    
    DESC = """This python file is used to *FIX* migration bugs, when migrating from one release of ecflow to another
  The typical migration path is:

    Migration is required when the release number changes:
        <release-number>.<major>.<minor>
            6.1.7   ->  7.0.0
        as the checkpoint files are not compatible.

        To actually migrate to a newer version of ecflow, follow these steps:

 Steps for Old server:
   o shutdown
        # ecflow_client --shutdown
   o suspend all suites
        # CL="ecflow_client --port 32222 --host vsms1"
       # for s in $($CL --suites); do $CL --suspend /$s; done
   o wait for active/submitted tasks to complete
   o halt the server
       # ecflow_client --halt
   o Use --migrate to dump state and structure to a file
       # ecflow_client --migrate > all_suites.def
   o terminate server *or* leave server running but start new server on different machine
     to avoid port number clash.
   o remove checkpt and backup checkpt files, to prevent new server from loading them
     *Only* applicable if starting new server on same machine

 Steps for New server:
   o start server
   o load the migration file
       # ecflow_client --load all_suites.def
   o set server running:
       # ecflow_client --restart
   o resume suspended suites
       # CL="ecflow_client --port 32222 --host vsms1"
       # for s in $($CL --suites); do $CL --resume /$s; done

 
 There could be bugs with *old* "ecflow_client --migrate > all_suites.def"
 This file, will fix any bugs associated --migrate, so that the subsequent
 load with  *new* client (ecflow_client --load all_suites.def) will work

 Issue 1: In release prior to 3.1.8, the --migrate in some cases
          would generate a task where the abort reason could span multiple lines
          This messed up the subsequent, --load.
          i.e
   task task # passwd:jxX0gIbR abort<:   Script for /suite/family/task can not be found:
   ECF_SCRIPT(/tmp/map/work/p4/merlin/workspace/MyProject/Pyext/test/data/CUSTOMER/ECF_HOME/suite/family/task.sms) does not exist:
   Variable ECF_FETCH not defined:
   Variable ECF_FILES not defined:
   Search of directory ECF_HOME(/tmp/map/work/p4/merlin/workspace/MyProject/Pyext/test/data/CUSTOMER/ECF_HOME) failed:
   >abort try:2 state:aborted dur:02:19:57 flag:task_aborted,no_script
      edit VARIABLE 'VALUE'

   The fix will ensure that:
     1/ abort<:    and >abort, are all placed on the same line.

 Usage:
    python migrate.py <filename>
    python migrate.py all_suites.def 
    
    The fixed definition is written as <filename.mig, i.e. all_suites.mig
    """

    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('defs_file',  help="The definition file to *fix* for migration")
    ARGS = PARSER.parse_args()
    ARGS.defs_file = os.path.expandvars(ARGS.defs_file) # expand references to any environment variables
    print ARGS    
    do_migrate( ARGS.defs_file )
    
