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

class Test_dunder_lshift(unittest.TestCase):
    def test_node_dunder_lshift(self):
        # will ONLY work if we have starting NodeContainer
        suite = Suite('s') << Task('t1') << Task('t2') << Task('t3') << Task('t4')
        self.assertEqual(len(list(suite)),4,"expected 4 children but found " + str(len(list(suite))) )
        print "t1.get_trigger: " + str(suite.t1.get_trigger())
        print "t2.get_trigger: " + str(suite.t2.get_trigger())
        print "t3.get_trigger: " + str(suite.t3.get_trigger())
        print "t4.get_trigger: " + str(suite.t4.get_trigger())
        
        self.assertEqual(suite.t4.get_trigger(),None,"Trigger not as expected: " + str( suite.t4.get_trigger())) 
        self.assertEqual(str(suite.t3.get_trigger()),"t4 == complete","Trigger not as expected: " + str( suite.t3.get_trigger())) 
        self.assertEqual(str(suite.t2.get_trigger()),"t3 == complete","Trigger not as expected: " + str( suite.t2.get_trigger())) 
        self.assertEqual(str(suite.t1.get_trigger()),"t2 == complete","Trigger not as expected: " + str( suite.t1.get_trigger())) 
 
        fam = Family("f1") << Task('t1') << Task('t2') << Task('t3') << Task('t4')
        self.assertEqual(len(list(fam)),4,"expected 4 children but found " + str(len(list(fam))) )
 
        self.assertEqual(fam.t4.get_trigger(),None,"Trigger not as expected: " + str( fam.t4.get_trigger())) 
        self.assertEqual(str(fam.t3.get_trigger()),"t4 == complete","Trigger not as expected: " + str( fam.t3.get_trigger())) 
        self.assertEqual(str(fam.t2.get_trigger()),"t3 == complete","Trigger not as expected: " + str( fam.t2.get_trigger())) 
        self.assertEqual(str(fam.t1.get_trigger()),"t2 == complete","Trigger not as expected: " + str( fam.t1.get_trigger())) 

    def test_node_dunder_lshift_trigger_cat(self):
        suite = Suite('s')
        # will ONLY work if we have starting NodeContainer
        suite >> Task('t1') << (Task('t2') + Trigger("x == 1")) << (Task('t3') + Trigger("y==1")) << Task('t4')
        self.assertEqual(len(list(suite)),4,"expected 4 children but found " + str(len(list(suite))) )
         
        self.assertEqual(suite.t4.get_trigger(),None,"Trigger not as expected: " + str( suite.t4.get_trigger())) 
        self.assertEqual(str(suite.t3.get_trigger()),"y==1 AND t4 == complete","Trigger not as expected: " + str( suite.t3.get_trigger())) 
        self.assertEqual(str(suite.t2.get_trigger()),"x == 1 AND t3 == complete","Trigger not as expected: " + str( suite.t2.get_trigger())) 
        self.assertEqual(str(suite.t1.get_trigger()),"t2 == complete","Trigger not as expected: " + str( suite.t1.get_trigger())) 


if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
