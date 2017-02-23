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
import argparse # for argument parsing     

class Indentor:
    """This class manages indentation, 
    It is used to correctly indent the definition node tree hierarchy
    """
    _index = 0
    def __init__(self):
        Indentor._index += 1
    def __del__(self):
        Indentor._index -= 1
    @classmethod
    def indent(cls):
        for i in range(Indentor._index):
            print ' ',
     
class DefsTraverser:
    """Traverse the ecflow.Defs definition and writes to standard out.
    
    Ecflow has the following hierarchy::
       Task   --> Submittable   -->Node
       Family --> NodeContainer -->Node
       Suite  --> NodeContainer -->Node
    
    This demonstrates that all nodes in the node tree and all attributes are accessible.
    Additionally the state data is also accessible. This class will write state data as
    comments. If the definition was returned from the server, it allows access to latest 
    snapshot of the state data held in the server. 
    """
    def __init__(self,defs):
        assert (isinstance(defs,ecflow.Defs)),"Expected ecflow.Defs as first argument"
        self.__defs = defs
        
    def do_print(self):
        for extern in self.__defs.externs:
            self.__println("extern " + extern)
        for suite in self.__defs.suites:
            self.__print("suite ")
            self.__print_node(suite)
            clock = suite.get_clock()
            if clock:
                indent = Indentor()
                self.__println(str(clock))
                del indent
            self.__print_nc(suite)
            self.__println("endsuite")  
 
    def __print_nc(self,node_container):
        indent = Indentor()
        for node in node_container.nodes:
            if isinstance(node, ecflow.Task):
                self.__print("task ")
                self.__print_node(node)
                self.__print_alias(node)
            else: 
                self.__print("family ")
                self.__print_node(node)
                self.__print_nc(node)
                self.__println("endfamily")
        del indent

    def __print_alias(self,task):
       indent = Indentor()
       for alias in task.nodes:
           self.__print("alias ")
           self.__print_node(alias)
           self.__println("endalias")
       del indent

    def __print_node(self,node):
        print node.name() + " # state:" + str(node.get_state()) 
        
        indent = Indentor()
        defStatus = node.get_defstatus()
        if defStatus != ecflow.DState.queued: 
            self.__println("defstatus " + str(defStatus))
            
        autocancel = node.get_autocancel()
        if autocancel: self.__println(str(autocancel))
        
        repeat = node.get_repeat()
        if not repeat.empty(): self.__println(str(repeat)  + " # value: " + str(repeat.value()))
    
        late = node.get_late()
        if late: self.__println(str(late) + " # is_late: " + str(late.is_late()))

        complete_expr = node.get_complete()
        if complete_expr:
            for part_expr in complete_expr.parts:
                trig = "complete "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                self.__print(trig)
                print part_expr.get_expression()
        trigger_expr = node.get_trigger()
        if trigger_expr:
            for part_expr in trigger_expr.parts:
                trig = "trigger "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                self.__print(trig)
                print part_expr.get_expression() 
                
        for var in node.variables:    self.__println("edit " + var.name() + " '" + var.value() + "'")
        for meter in node.meters:     self.__println(str(meter) + " # value: " + str(meter.value()))
        for event in node.events:     self.__println(str(event) + " # value: " + str(event.value()))
        for label in node.labels:     self.__println(str(label) + " # value: " + label.new_value())
        for limit in node.limits:     self.__println(str(limit) + " # value: " + str(limit.value()))
        for inlimit in node.inlimits: self.__println(str(inlimit))
        for the_time in node.times:   self.__println(str(the_time))
        for today in node.todays:     self.__println(str(today))   
        for date in node.dates:       self.__println(str(date))  
        for day in node.days:         self.__println(str(day))   
        for cron in node.crons:       self.__println(str(cron))    
        for verify in node.verifies:  self.__println(str(verify))
        for zombie in node.zombies:   self.__println(str(zombie))
        for queue in node.queues:     self.__println(str(queue))
        
        del indent

    def __print(self,the_string):
        Indentor.indent()
        print the_string,

    def __println(self,the_string):
        Indentor.indent()
        print the_string
 
if __name__ == "__main__":
    
    DESC = """Will list all the nodes and attributes in the definition
              Usage:
                Example1: List all the states
                   printdefs.py --host cca --port 4141  
            """    
    PARSER = argparse.ArgumentParser(description=DESC,  
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    PARSER.add_argument('--host', default="localhost",   
                        help="The name of the host machine, defaults to 'localhost'")
    PARSER.add_argument('--port', default="3141",   
                        help="The port on the host, defaults to 3141")
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
        defs_traverser = DefsTraverser(defs)
        defs_traverser.do_print()

    except RuntimeError, ex:
        print "Error: " + str(ex)
        print "Check host and port number are correct."
