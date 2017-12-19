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
# SCRATCH test for ecflow python api

from ecflow import Alias, AttrType, Autocancel, CheckPt, ChildCmdType, Client, Clock, Cron, DState, Date, Day, Days, \
                  Defs, Ecf, Event, Expression, Family, FamilyVec, File, Flag, FlagType, FlagTypeVec, InLimit, \
                  JobCreationCtrl, Label, Late, Limit, Meter, Node, NodeContainer, NodeVec, PartExpression, PrintStyle, \
                  Repeat, RepeatDate, RepeatDay, RepeatEnumerated, RepeatInteger, RepeatString, SState, State, Style, \
                  Submittable, Suite, SuiteVec, Task, TaskVec, Time, TimeSeries, TimeSlot, Today, UrlCmd, Variable, \
                  VariableList, Verify, WhyCmd, ZombieAttr, ZombieType, ZombieUserActionType, Trigger, Complete, Edit, Defstatus
import unittest 
import sys
import os
import ecflow_test_util as Test

class Test_replace(unittest.TestCase):
    def test_replace_on_server(self):
        print("test_replace_on_server +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        with Test.Server() as ci:
            PrintStyle.set_style( Style.MIGRATE ) # show node state 
            ci.delete_all()     
            defs = Defs() + (Suite("s1") + Family('f1').add(Task('t1'),Task('t2')))
            ci.load(defs)  
 
            # We should have 4 nodes
            ci.sync_local()
            ci_defs = ci.get_defs()
            node_vec = ci_defs.get_all_nodes()
            self.assertEqual(len(list(node_vec)) , 4,"Expected two 4 nodes: \n" + str(ci.get_defs()))
             
            # replace each node, add variable first, then check, it was added
            for node in node_vec:
                print(node.name(),"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
                node += Edit(var="XX", var2="xx")
                node.replace_on_server(ci.get_host(),ci.get_port())
                
                ci.sync_local()
                replace_node = ci.get_defs().find_abs_node(node.get_abs_node_path())
                self.assertEqual(len(list(replace_node.variables)) , 2,"Expected two 2 variable: \n" + str(replace_node))
                self.assertEqual(replace_node.get_dstate(), DState.suspended,"Expected node to be suspended:\n" +  str(replace_node))

            # resume nodes, test that when False passed in we do not suspend the replaced node
            for node in node_vec:
                print(node.name(),"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
                node += Meter("meter",0,100)
                ci.resume(node.get_abs_node_path())
                node.replace_on_server(ci.get_host(),ci.get_port(),suspend_node_first=False)
                
                ci.sync_local()
                replace_node = ci.get_defs().find_abs_node(node.get_abs_node_path())
                self.assertEqual(len(list(replace_node.meters)) , 1,"Expected 1 meter: \n" + str(replace_node))
                self.assertEqual(len(list(replace_node.variables)) , 2,"Expected two 2 variable: \n" + str(replace_node))
                self.assertNotEqual(replace_node.get_dstate(), DState.suspended,"Expected node not to suspended:\n" +  str(replace_node))

            # replace the suite
            defs = Defs() + Suite("s1")
            host_port = ci.get_host() + ':' + ci.get_port()
            defs.s1.replace_on_server(host_port)
  
            ci.get_server_defs()
            self.assertEqual(len(list(ci.get_defs().s1)) , 0,"Expected 0 family: \n" + str(ci.get_defs()))
            self.assertEqual(ci.get_defs().s1.get_dstate(), DState.suspended,"Expected node to be suspended:\n" +  str(ci.get_defs()))

    def test_replace_on_server_errors(self):
        print("test_replace_on_server_errors +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        # expect error since nodes not attached to a definition
        # The error should happen before we connect to the server,hence no need to start server
        # Avoid suspending node first( since that will require node exist in the server)
        node_vec = [ Suite('s1'), Family('f1'), Task('t1') ]
        for node in node_vec:
            print(node.name(), " +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
            try: 
                node.replace_on_server("locahost:3141",suspend_node_first=False)
                self.assertFalse(False, "Expected failure since client definition is empty")
            except RuntimeError as e:
                self.assertTrue("client definition is empty" in str(e), "expected 'client definition is empty' in exception message  but found:\n"+ str(e))

        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
