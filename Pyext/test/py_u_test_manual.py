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
from ecflow import Alias, AttrType, Autocancel, CheckPt, ChildCmdType, Client, Clock, Cron, DState, Date, Day, Days, \
                  Defs, Ecf, Event, Expression, Family, FamilyVec, File, Flag, FlagType, FlagTypeVec, InLimit, \
                  JobCreationCtrl, Label, Late, Limit, Meter, Node, NodeContainer, NodeVec, PartExpression, PrintStyle, \
                  Repeat, RepeatDate, RepeatDay, RepeatEnumerated, RepeatInteger, RepeatString, SState, State, Style, \
                  Submittable, Suite, SuiteVec, Task, TaskVec, Time, TimeSeries, TimeSlot, Today, UrlCmd, Variable, \
                  VariableList, Verify, WhyCmd, ZombieAttr, ZombieType, ZombieUserActionType, Trigger, Complete, Edit, Defstatus
import os 
import unittest 

class TestAddSuiteFamilyTask(unittest.TestCase):
    def setUp(self):
        defs = Defs()            # create an empty definition
        suite = defs.add_suite("s1")    # create a suite and add it to the defs
        family = suite.add_family("f1") # create a family and add it to suite
        for i in [ "a", "b", "c" ]:     # create task ta,tb,tc
            family.add_task( "t" + i)   # create a task and add to family
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
        
        self.defs = defs
        
    def test_me(self):
        with Defs() as defs:
            with defs.add_suite("s1") as suite:
                with suite.add_family("f1") as family:
                    for i in [ "a", "b", "c" ]:      
                        family.add_task( "t" + i)    
        defs.save_as_defs("test.def")    
                    
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
        
    def test_me2(self):
        defs = Defs().add( 
            Suite("s1").add(
                Family("f1").add(
                    [ Task("t{}".format(t)) 
                      for t in ("a", "b", "c")] )))
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
                    
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")

    def test_me3(self):
        defs = Defs()
        defs += [ Suite("s1") ]
        defs.s1 += [ Family("f1") ]
        defs.s1.f1 += [ Task("t{}".format(t)) 
                        for t in ("a", "b", "c")] 
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
                    
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")

    def test_me3(self):
        defs = Defs().add(   
          [ Suite("s{}".format(i)).add( 
              [ Family("f{}".format(i)).add( 
                  [ Task("t{}".format(i)) for i in range(1,6)] ) 
                for i in range(1,6)]  ) 
            for i in range(1,6) ] )
        assert(len(defs)==5, " expected 5 suites but found " + str(len(defs)))
        for suite in defs:
            assert(len(suite)==5, " expected 5 familes but found " + str(len(suite)))
            for fam in suite:
                assert(len(fam)==5, " expected 5 tasks but found " + str(len(fam)))   
            
    def tearDown(self):
        unittest.TestCase.tearDown(self)
        os.remove("test.def")
        
class TestAddMeterEventLabel(unittest.TestCase):
    def setUp(self):
        defs = Defs()
        suite = Suite("s1")
        task = Task("t1")
        defs.add_suite(suite)
        suite.add_task(task)
        task.add_event( 2 )                      # event reference with 2
        task.add_event("wow")                    # event reference with name "wow"
        task.add_event( 10,"Eventname2" )        # event referenced with name "Eventname2"
        task.add_meter( "metername3",0,100 )     # name, min, max
        task.add_label( "label_name4", "value" ) # name, value
        
        self.defs = defs
        
    def test_alternative(self):
        defs = Defs().add(
            Suite("s1").add(
                Task("t1").add(
                    Event(2),
                    Event("wow"),
                    Event(10,"Eventname2" ),
                    Meter("metername3",0,100),
                    Label("label_name4", "value"))))
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs,self.defs, "expected defs to be the same")

    def test_alternative2(self):
        defs = Defs()
        defs += [ Suite("s1")]
        defs.s1 += [ Task("t1") ]
        task.s1.t1 += [ Event(2),
                        Event("wow"),
                        Event(10,"Eventname2" ),
                        Meter("metername3",0,100),
                        Label("label_name4", "value") ]
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative2(self):
        with Defs() as defs:
            with defs.add_suite("s1") as suite:
                with suite.add_task("t1") as t1:
                    t1 += [ Event(2),
                           Event("wow"),
                           Event(10,"Eventname2" ),
                           Meter("metername3",0,100),
                           Label("label_name4", "value") ]
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
           
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
