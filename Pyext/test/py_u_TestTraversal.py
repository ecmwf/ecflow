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

# This file is used to test the python traversal routines
# This is done by loading from disk a defs file.
# traversing this defs file, and creating another defs file
# then comparing the two. If traversal was correct. the file should be the same

import ecflow
import os,fnmatch
            
class Indentor:
    """This class manages indentation, for use with context manager
    It is used to correctly indent the definition node tree hierarchy
    """
    _index = 0
    def __init__(self):
        Indentor._index += 1
    def __del__(self):
        Indentor._index -= 1
    @classmethod
    def indent(cls,the_file):
        for i in range(Indentor._index):
            the_file.write(' ')
            
class DefsTraverser:
    """Traverse the ecflow.Defs definition and write to file.
    
    This demonstrates that all nodes in the node tree and all attributes are accessible.
    Additionally the state data is also accessible. This class will write state data as
    comments. If the definition was returned from the server, it allows access to latest 
    snapshot of the state data held in the server. 
    """
    def __init__(self,defs,file_name):
        assert (isinstance(defs,ecflow.Defs)),"Expected ecflow.Defs as first argument"
        assert (isinstance(file_name,str)),"Expected a string argument. Representing a file name"
        self.__defs = defs
        self.__file = open(file_name, 'w')
        
    def write_to_file(self):
        for extern in self.__defs.externs:
            self.__writeln("extern " + extern)
        for suite in self.__defs.suites:
            self.__write("suite ")
            self.__print_node(suite)
            clock = suite.get_clock()
            if clock:
                indent = Indentor()
                self.__writeln(str(clock))
                del indent
            self.__print_nc(suite)
            self.__writeln("endsuite")  
        self.__file.close()

    def __print_nc(self,node_container):
        indent = Indentor()
        for node in node_container.nodes:
            if isinstance(node, ecflow.Task):
                self.__write("task ")
                self.__print_node(node)
                self.__print_alias(node)
            else: 
                self.__write("family ")
                self.__print_node(node)
                self.__print_nc(node)
                self.__writeln("endfamily")
        del indent

    def __print_alias(self,task):
       indent = Indentor()
       for alias in task.nodes:
           self.__write("alias ")
           self.__print_node(alias)
           self.__writeln("endalias")
       del indent

    def __print_node(self,node):
        self.__file.write(node.name() + " # state:" + str(node.get_state()) + "\n")
        
        indent = Indentor()
        defStatus = node.get_defstatus()
        if defStatus != ecflow.DState.queued: 
            self.__writeln("defstatus " + str(defStatus))
            
        autocancel = node.get_autocancel()
        if autocancel: self.__writeln(str(autocancel))
        
        repeat = node.get_repeat()
        if not repeat.empty(): self.__writeln(str(repeat)  + " # value: " + str(repeat.value()))
    
        late = node.get_late()
        if late: self.__writeln(str(late) + " # is_late: " + str(late.is_late()))

        complete_expr = node.get_complete()
        if complete_expr:
            for part_expr in complete_expr.parts:
                trig = "complete "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                self.__write(trig)
                self.__file.write( part_expr.get_expression() + "\n")
        trigger_expr = node.get_trigger()
        if trigger_expr:
            for part_expr in trigger_expr.parts:
                trig = "trigger "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                self.__write(trig)
                self.__file.write( part_expr.get_expression() + "\n")
                
        for var in node.variables:    self.__writeln("edit " + var.name() + " '" + var.value() + "'")
        for meter in node.meters:     self.__writeln(str(meter) + " # value: " + str(meter.value()))
        for event in node.events:     self.__writeln(str(event) + " # value: " + str(event.value()))
        for label in node.labels:     self.__writeln(str(label) + " # value: " + label.new_value())
        for limit in node.limits:     self.__writeln(str(limit) + " # value: " + str(limit.value()))
        for inlimit in node.inlimits: self.__writeln(str(inlimit))
        for the_time in node.times:   self.__writeln(str(the_time))
        for today in node.todays:     self.__writeln(str(today))   
        for date in node.dates:       self.__writeln(str(date))  
        for day in node.days:         self.__writeln(str(day))   
        for cron in node.crons:       self.__writeln(str(cron))    
        for verify in node.verifies:  self.__writeln(str(verify))
        for zombie in node.zombies:   self.__writeln(str(zombie))
        
        del indent

    def __write(self,the_string):
        Indentor.indent(self.__file)
        self.__file.write(the_string)

    def __writeln(self,the_string):
        Indentor.indent(self.__file)
        self.__file.write(the_string + "\n")
                                   
def check_traversal(path_to_def):
    # Open a def on disk *and* load into memory
    reference_def = ecflow.Defs( path_to_def )
         
    # traverse the opened def and write it out again
    file_name = "copy.def"
    traverser = DefsTraverser(reference_def,file_name)
    traverser.write_to_file()
         
    # restore the defs we create via traversal. If traversal was good it should
    # be the same as def on disk.
    try:
        traversed_def = ecflow.Defs( file_name )
    except RuntimeError, e: 
        print "Could not parse file " + file_name + "\n" + str(e)
        exit(1)
         
    # compare the two defs
    ecflow.Ecf.set_debug_equality(True)
    defs_equal = (traversed_def == reference_def)
    if not defs_equal:
        print str(path_to_def) + " FAILED "
        print "The traversed defs=========\n" + str(traversed_def) + "\nnot the same as reference def============\n" + str(reference_def)
        print "===================== " + file_name + " ===================================="
        the_traversed_def_on_disk = open(file_name)
        for line in the_traversed_def_on_disk:
            print line,
            
        assert defs_equal, "Failed: ---"
      
        # Notice: this path does not delete file_name, left for analysis of failure
    else:
        print str(path_to_def) + " PASSED "
        os.remove(file_name)        
        

def all_files(root, patterns='*', single_level=False, yield_folders=False):
    """Expand patterns from semi-colon separated string to list"""
    patterns = patterns.split(';')
    for path, subdirs, files in os.walk(root):
        if yield_folders:
            files.extend(subdirs)
        files.sort()
        for name in files:
            for pattern in patterns:
                if fnmatch.fnmatch(name,pattern):
                    yield os.path.join(path, name)
                    break
        if single_level:
            break    
        
if __name__ == "__main__":
    print "####################################################################"
    print "Running ecflow version " + ecflow.Client().version() + " debug build(" + str(ecflow.debug_build()) +")"
    print "####################################################################"
 
    cwd = os.getcwd()
    #print cwd
    #print "split = " + str(os.path.split(cwd))
    #print "basename = " + os.path.basename(cwd)
    #print "dirname = " + os.path.dirname(cwd)
    #print "splitext = " + str( os.path.splitext(cwd) )
    #print "isdir = " + str(os.path.isdir(cwd))
    
    # Traverse all the good defs in the Parser directory
    newpath = ""
    if os.path.basename(cwd) == "Pyext":
        newpath = os.path.join( os.path.dirname(cwd),"AParser/test/data/good_defs" )
    
    #print newpath
    for path_to_defs_file in all_files(newpath, '*.def;*.txt'):
        check_traversal(path_to_defs_file)
        
    # try the mega_def. Commented out since it takes to long
    #mega_def = os.path.join( os.path.dirname(cwd), "AParser/test/data/single_defs/mega.def")
    #check_traversal(mega_def)
    print "All Tests pass"    