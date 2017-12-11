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

class Test_get_attr(unittest.TestCase):
    def test_get_attr(self):
        defs = Defs() + (Suite('s') + Family('f').add(Task('t') + Edit(var="1")))
        defs += Edit(var="1")
        
        self.assertIsInstance(defs.s, Suite, "Expected suite")
        self.assertIsInstance(defs.s.f, Family, "Expected family")
        self.assertIsInstance(defs.s.f.t, Task, "Expected Task but found " + str(type(defs.s.f.t)))
        
        self.assertIsInstance(defs.var, Variable, "Expected Variable but found " + str(type(defs.var)))
        self.assertIsInstance(defs.s.f.t.var, Variable, "Expected Variable but found " + str(type(defs.s.f.t.var)))
        
    def test_get_attr_generated_variables(self):
        defs = Defs() + (Suite('s') + Family('f').add((Task('t') + Edit(var="1") + RepeatDate("YMD", 20100111, 20100115, 2))))
        defs.s.f.t += Meter("meter",0,100)
        defs.s.f.t += Event("event")
        defs.s.f.t += Limit("limitx",10)
        #PrintStyle.set_style(Style.STATE)
        #print(defs)
        
        self.assertTrue(defs.ECF_MICRO, "expected generated variable")
        self.assertTrue(defs.ECF_HOME, "expected generated variable")
        self.assertTrue(defs.ECF_JOB_CMD , "expected generated variable")
        self.assertTrue(defs.ECF_KILL_CMD , "expected generated variable")
        self.assertTrue(defs.ECF_STATUS_CMD , "expected generated variable")
        self.assertTrue(defs.ECF_URL_CMD , "expected generated variable")
        self.assertTrue(defs.ECF_LOG  , "expected generated variable")
        self.assertTrue(defs.ECF_INTERVAL   , "expected generated variable")
        self.assertTrue(defs.ECF_LISTS   , "expected generated variable")
        self.assertTrue(defs.ECF_CHECK   , "expected generated variable")
        self.assertTrue(defs.ECF_CHECKOLD   , "expected generated variable")
        self.assertTrue(defs.ECF_CHECKINTERVAL   , "expected generated variable")
        self.assertTrue(defs.ECF_CHECKMODE   , "expected generated variable")
        self.assertTrue(defs.ECF_TRIES   , "expected generated variable")
        self.assertTrue(defs.ECF_VERSION   , "expected generated variable")
        self.assertTrue(defs.ECF_PORT   , "expected generated variable")
        self.assertTrue(defs.ECF_HOST    , "expected generated variable")

        self.assertTrue(defs.s.SUITE, "expected generated variable")
        self.assertEqual(defs.s.SUITE.value() , 's', "expected suite name of 's' but found")
        self.assertTrue(defs.s.ECF_DATE , "expected generated variable")
        self.assertTrue(defs.s.YYYY , "expected generated variable")
        self.assertTrue(defs.s.DOW , "expected generated variable")
        self.assertTrue(defs.s.DOY , "expected generated variable")
        self.assertTrue(defs.s.DATE , "expected generated variable")
        self.assertTrue(defs.s.DAY , "expected generated variable")
        self.assertTrue(defs.s.DD , "expected generated variable")
        self.assertTrue(defs.s.MM , "expected generated variable")
        self.assertTrue(defs.s.MONTH , "expected generated variable")
        self.assertTrue(defs.s.ECF_CLOCK , "expected generated variable")
        self.assertTrue(defs.s.ECF_TIME , "expected generated variable")
        self.assertTrue(defs.s.TIME  , "expected generated variable")
    
        self.assertTrue(defs.s.f.FAMILY   , "expected generated variable")
        self.assertTrue(defs.s.f.FAMILY1   , "expected generated variable")

        self.assertTrue(defs.s.f.t.TASK    , "expected generated variable")
        self.assertEqual(defs.s.f.t.TASK.value() , 't', "expected task name of 's'")
        self.assertTrue(defs.s.f.t.ECF_JOB     , "expected generated variable")
        self.assertTrue(defs.s.f.t.ECF_SCRIPT      , "expected generated variable")
        self.assertTrue(defs.s.f.t.ECF_JOBOUT      , "expected generated variable")
        self.assertTrue(defs.s.f.t.ECF_TRYNO      , "expected generated variable")
        self.assertEqual(defs.s.f.t.ECF_TRYNO.value() , '0', "expected task try no of '0'")
        self.assertTrue(defs.s.f.t.ECF_RID      , "expected generated variable")
        self.assertTrue(defs.s.f.t.ECF_NAME      , "expected generated variable")
        self.assertEqual(defs.s.f.t.ECF_NAME.value()  , '/s/f/t', "expected task ECF_NAME of '/s/f/t'")
        self.assertTrue(defs.s.f.t.ECF_PASS       , "expected generated variable")

        self.assertEqual(defs.s.f.t.YMD.value() , '20100111', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.YMD_YYYY.value() , '2010', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.YMD_MM.value() , '1', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.YMD_DD.value() , '11', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.YMD_DOW.value() , '1', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.YMD_JULIAN.value() , '2455208', "expected generated YMD of value")
        self.assertEqual(defs.s.f.t.event.value() , 0, "expected generated event of value 0 but found " + str(defs.s.f.t.event.value()))
        self.assertEqual(defs.s.f.t.meter.value() , 0, "expected generated meter of value 0 but found " + str(defs.s.f.t.meter.value()))
        self.assertEqual(defs.s.f.t.limitx.value() , 0, "expected generated limit of value 0 but found " + str(defs.s.f.t.limitx.value()))

if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
