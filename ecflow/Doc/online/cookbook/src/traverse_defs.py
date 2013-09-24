#!/usr/bin/env python2.7
import ecflow

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
    def indent(cls, the_file):
        for i in range(Indentor._index):
            the_file.write(' ')
            
class DefsTraverser:
    """Traverse the ecflow.Defs definition and write to file.
    
    This demonstrates that all nodes in the node tree and all attributes are accessible.
    Additionally the state data is also accessible. This class will write state data as
    comments. If the definition was returned from the server, it allows access to latest 
    snapshot of the state data held in the server. 
    """
    def __init__(self, defs):
        assert (isinstance(defs, ecflow.Defs)),"Expected ecflow.Defs as first argument"
        self.__defs = defs
        
    def write_to_file(self, file_name):
        assert (isinstance(file_name, str)),"Expected a string argument. Representing a file name"
        self.__file = open(file_name, 'w')
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

    def __print_nc(self, node_container):
        indent = Indentor()
        for node in node_container.nodes:
            if isinstance(node, ecflow.Task):
                self.__write("task ")
                self.__print_node(node)
            else: 
                self.__write("family ")
                self.__print_node(node)
                self.__print_nc(node)
                self.__writeln("endfamily")
        del indent

    def __print_node(self, node):
        self.__file.write(node.name() + " # state: " + str(node.get_state()) + "\n")
        
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
        for label in node.labels:     self.__writeln(str(label) + " # value: " + label.value())
        for limit in node.limits:     self.__writeln(str(limit) + " # value: " + str(limit.value()))
        for inlimit in node.inlimits: self.__writeln(str(inlimit))
        for the_time in node.times:   self.__writeln(str(the_time))
        for today in node.todays :    self.__writeln(str(today))   
        for date in node.dates:       self.__writeln(str(date))
        for day in node.days:         self.__writeln(str(day))  
        for cron in node.crons:       self.__writeln(str(cron))
        for verify in node.verifies:  self.__writeln(str(verify))
        for zombie in node.zombies:   self.__writeln(str(zombie))
        
        del indent

    def __write(self, the_string):
        Indentor.indent(self.__file)
        self.__file.write(the_string)

    def __writeln(self, the_string):
        Indentor.indent(self.__file)
        self.__file.write(the_string + "\n")

   
try:
    # Create the client. This will read the default environment variables
    ci = ecflow.Client("localhost", "4143")
    
    # Get the node tree suite definition as stored in the server
    # The definition is retrieved and stored on the variable 'ci'
    ci.sync_local()

    # access the definition retrieved from the server
    server_defs = ci.get_defs()
    
    if server_defs == None :
        print "The server has no definition"
        exit(1)
    
    # Traverse server definition writing all state as comments.
    traverser = DefsTraverser(server_defs)
    traverser.write_to_file("server.defs")
        
except RuntimeError, e:
    print "failed: " + str(e)
    