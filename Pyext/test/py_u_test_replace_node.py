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

    def test_replace_on_server_errors(self):
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
