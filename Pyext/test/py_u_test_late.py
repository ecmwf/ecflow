#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2019 ECMWF.
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

class TestLate(unittest.TestCase):
    def setUp(self):
        late = Late()
        late.submitted(TimeSlot(20, 10))
        late.active(TimeSlot(20, 10))
        late.complete(TimeSlot(20, 10), True)
        self.late = late
        #print("setUp ",str(self.late))
         
        late = Late()
        late.submitted(10, 10)
        late.active(10, 10)
        late.complete(10, 10, False)
        self.late1 = late
          
        late = Late()
        late.submitted(10, 10)
        self.late2 = late
  
        late = Late()
        late.active(10, 10)
        self.late3 = late
  
        late = Late()
        late.complete(10, 12, False)
        self.late4 = late
     
    def test_pass(self):
        late = Late(submitted='20:10',active='20:10',complete='+20:10')
        #print("test_pass",str(late))
        self.assertEqual(self.late, late, "late shoud be the same:\n" + str(self.late) + "\n" + str(late))
        
        late = Late(submitted='10:10',active='10:10',complete='10:10')
        self.assertEqual(self.late1, late, "late shoud be the same:\n" + str(self.late1) + "\n" + str(late))
  
        late = Late(submitted='10:10')
        self.assertEqual(self.late2, late, "late shoud be the same:\n" + str(self.late2) + "\n" + str(late))
  
        late = Late(active='10:10')
        self.assertEqual(self.late3, late, "late shoud be the same:\n" + str(self.late3) + "\n" + str(late))
  
        late = Late(complete='10:12')
        self.assertEqual(self.late4, late, "late shoud be the same:\n" + str(self.late4) + "\n" + str(late))

    def test_fail(self):
        self.assertNotEqual(self.late ,  self.late1, "Late should not be equal")
         
        with self.assertRaises(RuntimeError):
            late = Late("");  # no keyword arguments
 
        with self.assertRaises(RuntimeError):
            late = Late(submittedd='20:10',active='20:10',complete='+20:10') # miss-spelt keyword
 
        with self.assertRaises(RuntimeError):
            late = Late(submitted='20:10',activee='20:10',complete='+20:10') # miss-spelt keyword
 
        with self.assertRaises(RuntimeError):
            late = Late(submitted='20:10',active='20:10',completee='+20:10') # miss-spelt keyword

if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
