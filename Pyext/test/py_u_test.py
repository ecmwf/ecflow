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

class TestDataAquistionSolution(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
        
        defs = Defs()
        suite = defs.add_suite("data_aquisition")
        suite.add_repeat( RepeatDay(1) )
        suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
        suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_FILES",   os.getenv("HOME") + "/course/data")
        suite.add_variable("SLEEP","2")
        for city in ( "Exeter", "Toulouse", "Offenbach", "Washington", "Tokyo", "Melbourne", "Montreal" ) :
            fcity = suite.add_family(city)
            fcity.add_task("archive")
            for obs_type in ( "observations", "fields", "images" ):
                type_fam = fcity.add_family(obs_type)
                if city in ("Exeter", "Toulouse", "Offenbach"): type_fam.add_time("00:00 23:00 01:00")
                if city in ("Washington") :                     type_fam.add_time("00:00 23:00 03:00")
                if city in ("Tokyo") :                          type_fam.add_time("12:00")
                if city in ("Melbourne") :                      type_fam.add_day( "monday" )
                if city in ("Montreal") :                       type_fam.add_date(1, 0, 0)
         
                type_fam.add_task("get")
                type_fam.add_task("process").add_trigger("get eq complete")
                type_fam.add_task("store").add_trigger("get eq complete")
                    
        print(defs)
        self.defs = defs
        
    def test_me0(self):
        #!/usr/bin/env python2.7
        import os
        
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + Suite("data_aquisition",
                            RepeatDay(1),
                            Edit(ECF_HOME=home),
                            Edit(ECF_INCLUDE=home),
                            Edit(ECF_FILES=home + "/data"),
                            Edit(SLEEP=2))
        for city in ( "Exeter", "Toulouse", "Offenbach", "Washington", "Tokyo", "Melbourne", "Montreal" ) :
            fcity = defs.data_aquisition.add_family(city)
            fcity += Task("archive")
            for obs_type in ( "observations", "fields", "images" ):
                type_fam = fcity.add_family(obs_type)
                if city in ("Exeter", "Toulouse", "Offenbach"): type_fam + Time("00:00 23:00 01:00")
                if city in ("Washington") :                     type_fam + Time("00:00 23:00 03:00")
                if city in ("Tokyo") :                          type_fam + Time("12:00")
                if city in ("Melbourne") :                      type_fam + Day( "monday" )
                if city in ("Montreal") :                       type_fam + Date(1, 0, 0)
         
                type_fam >> Task("get") >> Task("process") >> Task("store") 
                #type_fam.process += Trigger("get eq complete") 
                #type_fam.store += Trigger("get eq complete")     

        print(defs)
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
