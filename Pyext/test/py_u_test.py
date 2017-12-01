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

class TestNewSuite(unittest.TestCase):
    def setUp(self):
        home = os.path.join(os.getenv("HOME"),"course")
        self.defs = Defs()
        suite = self.defs.add_suite("test")
        suite.add_variable("ECF_HOME", home)
        suite.add_task("t1")
         
        with Defs() as self.defs2: 
            with self.defs2.add_suite("test") as suite:
                suite.add_variable("ECF_HOME", home)
                suite.add_task("t1") 
         
        with Defs() as self.defs3:  
            self.defs3.add(Suite("test").add(
                Edit(ECF_HOME=home),
                Task("t1")))
             
        self.defs4 = Defs() + Suite("test").add(Edit(ECF_HOME=home))
        self.defs4.test += Task("t1")
        
        self.defs5 = Defs().add(
                Suite("test").add(
                    Edit(ECF_HOME= os.path.join(os.getenv("HOME"),  "course")),
                    Task("t1")))
        
     
    def test_defs_equal(self):
        self.assertEqual(self.defs, self.defs2, "defs not the same")
        self.assertEqual(self.defs, self.defs3, "defs not the same")
        self.assertEqual(self.defs, self.defs4, "defs not the same")
        self.assertEqual(self.defs, self.defs5, "defs not the same")
        
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
