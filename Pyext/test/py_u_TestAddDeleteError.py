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

import os
from ecflow import Defs, Suite, Variable, Limit, InLimit, Task, PartExpression, \
                   Event, Meter, Label, RepeatInteger, RepeatEnumerated, RepeatDate, RepeatString, \
                   TimeSlot, TimeSeries, Today, Time, Date, Day, Days, Cron, Autocancel, Late, \
                   DState, Clock, ChildCmdType, ZombieType, ZombieAttr, ZombieUserActionType, Client, debug_build
import ecflow_test_util as Test
    
if __name__ == "__main__":
    
    Test.print_test_start(os.path.basename(__file__))
    
    #
    # Test for: See  ECFLOW-106 Times/Dates attributes attached to suite node
    #
    defs = Defs()
    suite = defs.add_suite("s1") 
    
    #
    # Suite should not be allowed time based dependencies
    # Check Today
    expected_error = False
    try:
        suite.add_today("00:30")
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"

    expected_error = False
    try:
        suite.add_today(0,30)
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"
    
    #
    # Check Time
    expected_error = False
    try:
        suite.add_time("+00:30")
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"

    expected_error = False
    try:
        suite.add_time(0,30)
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"

    # 
    # Check Date::See  ECFLOW-106 Times/Dates attributes attached to suite node
    expected_error = False
    try:
        suite.add_date( 1,1,2010)
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"
 
    # 
    # Check Day:: See  ECFLOW-106 Times/Dates attributes attached to suite node
    expected_error = False
    try:
        suite.add_day("sunday")
    except RuntimeError: 
        expected_error = True
    assert expected_error, "Suite should not allow any time based dependencies"
     

    print("All Tests pass")
    