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

class TestCron(unittest.TestCase):
    def setUp(self):
        cron = Cron()
        cron.set_week_days( [0,1,2,3,4,5,6] )
        cron.set_days_of_month( [1,2,3,4,5,6] )
        cron.set_months( [1,2,3,4,5,6] )
        cron.set_time_series( "+00:00 23:00 00:30" )
        self.cron1 =  cron
        
        cron2 = Cron()
        cron2.set_time_series(1, 30) # default relative = false, added in release 4.0.7
        self.cron2 = cron2

        cron3 = Cron()
        cron3.set_week_days([0, 1, 2, 3, 4, 5, 6 ])
        cron3.set_time_series(1, 30, True)
        self.cron3 = cron3
    
        cron4 = Cron()
        cron4.set_week_days([0, 1, 2, 3, 4, 5, 6])
        cron4.set_time_series("00:30 01:30 00:01")
        self.cron4 = cron4

        cron5 = Cron()
        cron5.set_week_days([0, 1, 2, 3, 4, 5, 6])
        cron5.set_time_series("+00:30")
        self.cron5 = cron5
        
        start = TimeSlot(0, 0)
        finish = TimeSlot(23, 0)
        incr = TimeSlot(0, 30)
        time_series = TimeSeries(start, finish, incr, True)
        cron6 = Cron( time_series )
        cron6.set_week_days([0, 1, 2, 3])
        self.cron6 = cron6
        
    
    def test_me0(self):
        cron = Cron("+00:00 23:00 00:30", days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6])
        self.assertEqual(self.cron1, cron, "cron not equal\n" + str(self.cron1) + "\n" + str(cron))
                
        cron = Cron("01:30")
        self.assertEqual(self.cron2, cron, "cron not equal\n" + str(self.cron2) + "\n" + str(cron))

        cron = Cron("+01:30",days_of_week=[0,1,2,3,4,5,6])
        self.assertEqual(self.cron3, cron, "cron not equal\n" + str(self.cron3) + "\n" + str(cron))

        cron = Cron("00:30 01:30 00:01",days_of_week=[0,1,2,3,4,5,6])
        self.assertEqual(self.cron4, cron, "cron not equal\n" + str(self.cron4) + "\n" + str(cron))

        cron = Cron("+00:30",days_of_week=[0,1,2,3,4,5,6])
        self.assertEqual(self.cron5, cron, "cron not equal\n" + str(self.cron5) + "\n" + str(cron))

        cron = Cron("+00:00 23:00 00:30",days_of_week=[0,1,2,3])
        self.assertEqual(self.cron6, cron, "cron not equal\n" + str(self.cron6) + "\n" + str(cron))

    def test_fail(self):
        with self.assertRaises(RuntimeError):
            cron = Cron("");  # empty
        with self.assertRaises(RuntimeError):
            cron = Cron("", days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6]); # empty
        with self.assertRaises(RuntimeError):
            cron = Cron("0023", days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6]) # bad time
        with self.assertRaises(RuntimeError):
            cron = Cron("00:10", days_of_weekss=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2,3,4,5,6]) # bad kw,days_of_weekss
        with self.assertRaises(RuntimeError):
            cron = Cron("00:10", days_of_week=[0,1,2,3,4,5,6],days_of_months=[1,2,3,4,5,6], months=[1,2,3,4,5,6]) # bad kw, days_of_months
        with self.assertRaises(RuntimeError):
            cron = Cron("00:10", days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], monthss=[1,2,3,4,5,6]) # bad kw, monthss
        with self.assertRaises(RuntimeError):
            cron = Cron("00:10", days_of_week="0,1,2,3,4,5,6",days_of_month=[1,2,3,4,5,6], month=[1,2,3,4,5,6])  # bad kw, expect vector of string
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
