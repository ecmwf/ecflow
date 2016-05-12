#!/usr/bin/env python2.7
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2016 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

import os          # for getenv
import shutil      # used to remove directory tree
#import argparse   # requires python 2.7
import getopt, sys # for argument parsing  

class Migrator(object):
    def __init__(self, list_of_input_lines):
        self.list_of_input_lines =  list_of_input_lines
        self.list_of_output_lines = []
        
    def _migration_hook(self,default_version_number):
        """Derived suite should override this function to perform the migration
        """
        return False;
    
    def output_lines(self):
        return self.list_of_output_lines
    
    def migrate(self, default_version_number):
        """Template/skeleton function, which will call the hooks
        """
        version = self.version_number_as_integer(default_version_number) 
        if version >= default_version_number:
            self.list_of_output_lines = self.list_of_input_lines
            return False

        self.list_of_output_lines = [] 
        migrated = self._migration_hook(default_version_number)
        if migrated : 
            self.list_of_input_lines = self.list_of_output_lines   # preserve this migration
            print "Applying " + type(self).__name__  
        else:
            self.list_of_output_lines = self.list_of_input_lines   
        return migrated         

    def version_number_as_integer(self,default_version):
        """Determine the version number used in ecflow_client --migrate
           This should be in the very first line
           # 3.1.8
        """
        if len(self.list_of_input_lines) > 0:
            tokens = self.list_of_input_lines[0].split()
            if len(tokens) > 1:
                # print tokens
                version = (int)(tokens[1].replace(".",""))
                # print "******************* found version " + str(version)
                return version;
        return default_version
    
    
class MigrateForTaskAbort(Migrator):
    """Fix bug where the task abort states(i.e a string) will contain new lines and hence
       spill over the next line, and affecting the parser. This will move the abort string
       into the same line, and remove the interleaving new lines
    """
    def __init__(self, list_of_input_lines):
        Migrator.__init__(self, list_of_input_lines)
 
    def _migration_hook(self,default_version_number):
        """
        The correct format is: where abort state is on the same line
        task task # passwd:jxX0gIbR abort<: reason for abort >abort:
        ensure_start_abort_and_end_abort_on_sameline
        """
        migrated = False
        start_line_append = ""
        for line in self.list_of_input_lines:
            # process line
            end_abort_pos = line.find(">abort")
            task_pos = line.find("task")
            if task_pos != -1:
                start_abort_pos = line.find("abort<:")
                if start_abort_pos != -1 and end_abort_pos != -1:
                    # start and end found on same line. This is the correct format
                    start_line_append = ""
                
                if start_abort_pos != -1 and end_abort_pos == -1:
                    # print "start abort found with no end abort"
                    start_line_append = line
                    start_line_append = start_line_append.rstrip('\n')
                    start_line_append += " "
                    migrated = True
                    continue
    
            if len(start_line_append) > 0:
                start_line_append += line
                if end_abort_pos != -1:
                    self.list_of_output_lines.append(start_line_append) # preserve'\n' for this case                   
                    # print start_line_append
                    start_line_append = "";
                else:
                    start_line_append = start_line_append.rstrip('\n')
                    start_line_append += " "
    
                continue;
                  
            self.list_of_output_lines.append(line)
           
        return migrated  
    
    
class MigrateForLabel(Migrator):
    """Fix bug where label value has new lines, and hence spills over to next line
       This will remove place newline back onto the same line
    """
    def __init__(self, list_of_input_lines):
        Migrator.__init__(self, list_of_input_lines)
 
    def _migration_hook(self,default_version_number):
        """
        The correct format is where label in on the same line
            label name "value" # "new value"
        However if the value or new value has new line it can mess up the parsing
            label info "have you? set START_LOBSVR=1 in the task, once, to launch the logserver,
    setup ssh login
    created remote /tmp/map/cray"
        """
        #count = 0;
        no_of_label_quotes = 0
        migrated = False
        start_line_append = ""
        for line in self.list_of_input_lines:
            #count =  count + 1
            #print str(count) + ": " + line
            #if count == 383119:
            #    print "debug mee"
            if len(start_line_append) > 0:
                no_of_label_quotes += line.count('"')
                start_line_append += line
                if no_of_label_quotes % 2 == 0:
                    # even number of quotes
                    self.list_of_output_lines.append(start_line_append) # preserve last '\n'  
                    start_line_append = ""
                    no_of_label_quotes = 0 
                else:
                    start_line_append = start_line_append.rstrip('\n')
                continue            
            else:
            
                # process line
                tokens = line.split()
                if len(tokens) >0 and tokens[0] == "label":
                    no_of_label_quotes = line.count('"')
                    if no_of_label_quotes % 2 != 0 : 
                        start_line_append = line
                        start_line_append = start_line_append.rstrip('\n')
                        migrated = True
                        continue
                  
            self.list_of_output_lines.append(line)
           
        return migrated
     
class MigrateForVariable(Migrator):
    """Fix bug where variable value has new lines, and hence spills over to next line
       This will remove place newline back onto the same line
    """
    def __init__(self, list_of_input_lines):
        Migrator.__init__(self, list_of_input_lines)
 
    def _migration_hook(self,default_version_number):
        """
        The correct format is where variable in on the same line
            edit EMOS_TYPE '  family prod_wparam
  endfamily
'         
        """
        #count = 0;
        no_of_label_quotes = 0
        migrated = False
        start_line_append = ""
        for line in self.list_of_input_lines:
            #count =  count + 1
            #print str(count) + ": " + line
            #if count == 383119:
            #    print "debug mee"
            if len(start_line_append) > 0:
                no_of_label_quotes += line.count("'")
                start_line_append += line
                if no_of_label_quotes % 2 == 0:
                    # even number of quotes
                    self.list_of_output_lines.append(start_line_append) # preserve last '\n'  
                    start_line_append = ""
                    no_of_label_quotes = 0 
                else:
                    start_line_append = start_line_append.rstrip('\n')
                continue            
            else:
            
                # process line, avoid ECF_URL_CMD as that could by error have uneven number of "'' i.e
                #   edit ECF_URL_CMD '${BROWSER:=firefox} -remote 'openURL(%URL%'
                # hence we use 
                #    if no_of_label_quotes == 1: 
                # instead of
                #    if no_of_label_quotes % 2 == 0:
                tokens = line.split()
                if len(tokens) >= 3 and tokens[0] == "edit":
                    no_of_label_quotes = line.count("'")
                    if no_of_label_quotes == 1: 
                        start_line_append = line
                        start_line_append = start_line_append.rstrip('\n')
                        migrated = True
                        continue
                  
            self.list_of_output_lines.append(line)
           
        return migrated
     
     
class MigrateForHistory(Migrator):
    """Fix bug where history spans multiple lines.  
       See File: history_bug.def
    """
    def __init__(self, list_of_input_lines):
        Migrator.__init__(self, list_of_input_lines)
 
    def _migration_hook(self,default_version_number):
        """
        The history should all be on one line
            defs_state MIGRATE state>:queued flag:message state_change:1500811 modify_change:48
            edit ECF_LOG '/vol/emos_nc/output/ecflow/vali.21801.ecf.log'
            edit SMSNAME '0'
            history /s2s_devel/ecmf/back MSG:[15:53:06 21.10.2014] --replace=/s2s_devel/ecmf/back s2s_devel.def parent  :emos
            history /s2s_devel/ecmf/enfh MSG:[13:45:53 12.11.2014] -alter change label last_run 20140929
            20141002
            20141016
            20141023 /s2s_devel/ecmf/enfh/  :emos    MSG:[14:05:53 14.11.2014] --requeue force /s2s_devel/ecmf/enfh  :emos
            history /s2s_devel/ecmf/enfh/back MSG:[13:38:30 11.11.2014] --alter change event doIt set /s2s_devel/ecmf/enfh/back  :emos
            suite limits #  begun:1 state:complete flag:message suspended:1
            endsuite
        """
        migrated = False
        history_started = False
        for line in self.list_of_input_lines:
            # process line
            tokens = line.split()
            if len(tokens) >= 2 and tokens[0] == "suite":
                history_started = False

            if not history_started and len(tokens) >= 2 and tokens[0] == "history":
                history_started = True
 
            if history_started:
                # append to previous line
                if len(tokens) >= 1 and tokens[0] != "history":
                    # get last line of output lines, string the new line and append
                    self.list_of_output_lines[-1] = self.list_of_output_lines[-1].rstrip()
                    self.list_of_output_lines[-1] = self.list_of_output_lines[-1] + line
                    migrated = True
                    continue;
                  
            self.list_of_output_lines.append(line)
           
        return migrated  
    
    
def do_migrate(defs_file):
    
    migration_count = 0
    fileName, fileExtension = os.path.splitext(defs_file)
    output_file_name = fileName + ".mig";
    
    input_file_object = open(defs_file,'r')
    output_file_object = open(output_file_name,'w')
    try:
        list_of_input_lines = input_file_object.readlines()  # preserves new lines, we want this
        
        list_of_output_lines = []
        abort_migrator = MigrateForTaskAbort(list_of_input_lines)
        if abort_migrator.migrate(401):
            migration_count += 1
            # did migration, update input lines for next migration
            list_of_input_lines = abort_migrator.output_lines()
        list_of_output_lines = abort_migrator.output_lines()
         
         
        label_migrator = MigrateForLabel(list_of_input_lines)
        if label_migrator.migrate(319):
            migration_count += 1
            # did migration, update input lines for next migration
            list_of_input_lines = label_migrator.output_lines()
        list_of_output_lines = label_migrator.output_lines()


        variable_migrator = MigrateForVariable(list_of_input_lines)
        if variable_migrator.migrate(401):
            migration_count += 1
            # did migration, update input lines for next migration
            list_of_input_lines = variable_migrator.output_lines()
        list_of_output_lines = variable_migrator.output_lines()
        
        
        variable_migrator = MigrateForHistory(list_of_input_lines)
        if variable_migrator.migrate(406):
            migration_count += 1
            # did migration, update input lines for next migration
            list_of_input_lines = variable_migrator.output_lines()
        list_of_output_lines = variable_migrator.output_lines()
            
        output_file_object.writelines(list_of_output_lines)
    finally:
        input_file_object.close()
        output_file_object.close()
        
    return migration_count 

def usage():
    DESC = """\nThis python file is used to *FIX* migration bugs, when migrating from one release of ecflow to another.

 Migration may be required when the release number changes:
 <release-number>.<major>.<minor>
    6.1.7   ->  7.0.0
 as the checkpoint files *may* not be compatible. 
        
 This can be *checked* before hand. By attempting to load the check pt into the new server. 
 On the old server run:
    ecflow_client --check_pt  # this will write out a checkpt file  
 Now copy this file to a separate directory, and start the *new* server using the same port number
 If no errors are reported, you do not need to migrate.  
 If loading the checkpt file into the new server fails for any reason then
 then please follow these steps.

 Steps for Old server:
   o shutdown
        # ecflow_client --shutdown
   o suspend all suites
        # CL="ecflow_client --port=32222 --host=vsms1"
        # for s in $($CL --suites); do $CL --suspend=/$s; done
   o wait for active/submitted tasks to complete
   o halt the server
        # ecflow_client --halt=yes
   o Use --migrate to dump state and structure to a file
        # ecflow_client --migrate > all_suites.def
   o terminate server *or* leave server running but start new server on different machine
     to avoid port number clash.
   o remove checkpt and backup checkpt files, to prevent new server from loading them
     *Only* applicable if starting new server on same machine
   o Run this script 
     python ecflow_migrate.py -d all_suites.def
     This will fix any issues associated with reloading this defs into the new server.
     It write a file called 'all_suites.mig', which we will need to load into the new server. 
     See below.

 Steps for New server:
   o start server
   o load the migration file
       # ecflow_client --load all_suites.mig
   o set server running:
       # ecflow_client --restart
   o resume suspended suites
       # CL="ecflow_client --port=32222 --host=vsms1"
       # for s in $($CL --suites); do $CL --resume=/$s; done


 There could be bugs with *old* "ecflow_client --migrate > all_suites.def"
 This file, will fix any bugs associated --migrate, so that the subsequent
 load with  *new* client (ecflow_client --load all_suites.mig) will work

 Usage:
    python ecflow_migrate.py --defs_file <filename>
    python ecflow_migrate.py --d <filename>
    python ecflow_migrate.py --defs_file all_suites.def 
    
    The fixed definition is written as filename.mig, i.e. all_suites.mig
    """
    return DESC;
    
    
if __name__ == "__main__":
    
#     PARSER = argparse.ArgumentParser(description=usage(),  
#                                      formatter_class=argparse.RawDescriptionHelpFormatter)
#     PARSER.add_argument('defs_file',  help="The definition file to *fix* for migration")
#     ARGS = PARSER.parse_args()
#     ARGS.defs_file = os.path.expandvars(ARGS.defs_file) # expand references to any environment variables
#     print ARGS    
#     do_migrate( ARGS.defs_file )
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd", ["help", "defs_file="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognised"
        print usage()
        sys.exit(2)
        
    defs_file = None
    for o, a in opts:
        if o in ("-h", "--help"):
            print usage()
            sys.exit()
        elif o in ("-d", "--defs_file"):
            defs_file = a
        else:
            assert False, "un-handled option"
    
    if defs_file == None:
        print "Please enter path to the defs file i.e ecflow_migrate --d ./defs_file.def"
        sys.exit(1)
        
    defs_file = os.path.expandvars(defs_file); # expand references to any environment variables
    do_migrate( defs_file )
