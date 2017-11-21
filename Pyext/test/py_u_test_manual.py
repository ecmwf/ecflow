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
        assert len(defs)==5, " expected 5 suites but found " + str(len(defs))
        for suite in defs:
            assert len(suite)==5, " expected 5 familes but found " + str(len(suite))
            for fam in suite:
                assert len(fam)==5, " expected 5 tasks but found " + str(len(fam))   
             
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
 
    def test_alternative1(self):
        defs = Defs()
        defs += [ Suite("s1")]
        defs.s1 += [ Task("t1") ]
        defs.s1.t1 += [ Event(2),
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
            
class TestAddLimitInlimit(unittest.TestCase):
     
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        s1.add_limit( "limitName4", 10 ) # name, maximum token
        f1 = s1.add_family("f1")
        f1.add_inlimit( "limitName4","/s1/f1",2) # limit name, path to limit, tokens consumed
        for i in range(1,4):
            f1.add_task( "t{}".format(i))
             
        self.defs = defs
 
    def test_alternative(self):
        defs = Defs().add(
            Suite("s1").add(
                Limit("limitName4", 10),
                Family("f1").add(
                    InLimit("limitName4","/s1/f1",2),
                    [ Task("t{}".format(t)) for t in range(1,4) ]
                    )))
 
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")        
 
    def test_alternative1(self):
        defs = Defs()
        defs += [ Suite("s1") ]
        defs.s1 += [ Limit("limitName4", 10),Family("f1") ]
        defs.s1.f1 += [ InLimit("limitName4","/s1/f1",2),
                        [ Task("t{}".format(t)) for t in range(1,4) ] ]
 
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")        
         
    def test_alternative2(self):
        with Defs() as defs:
            with defs.add_suite("s1") as s1:
                s1.add_limit( "limitName4", 10 ) # name, maximum token
                with s1.add_family("f1") as f1:
                    f1.add_inlimit( "limitName4","/s1/f1",2) # limit name, path to limit, tokens consumed
                    f1 += [ Task("t{}".format(t)) for t in range(1,4) ]
                     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
 
class TestAddVariable(unittest.TestCase):
     
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        s1.add_variable("HELLO","world") # name, value
        s1.add_variable({ "NAME":"value", "NAME2":"value2", "NAME3":"value3", "NAME4":4 }  )
        s1.add_variable(Variable("FRED","bloggs"))
        s1.add_variable(Variable("BILL","1"))
         
        defs.s1.sort_attributes("variable"); # sort since with dictionary order of addition is arbitary
        self.defs = defs
 
    def test_alternative(self):
         
        defs = Defs().add(
                      Suite("s1").add(
                          Edit(HELLO="world",FRED="bloggs",BILL=1,NAME="value",NAME2="value2",NAME3="value3",NAME4=4 )
                          ))
 
        defs.s1.sort_attributes("variable");
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
 
    def test_alternative1(self):
         
        defs = Defs()
        defs += [Suite("s1")]
        defs.s1 += [ Edit(HELLO="world"),
                     Edit({ "NAME":"value", "NAME2":"value2", "NAME3":"value3", "NAME4":4 }, BILL=1),
                     Edit(FRED="bloggs")
                   ]
 
        defs.s1.sort_attributes("variable");
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

class TestAddTrigger(unittest.TestCase):
     
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        t1 = s1.add_task("t1")
        t2 = s1.add_task("t2")
        t2.add_trigger( "t1 == active and t3 == aborted" )
        t2.add_complete( "t3 == complete" )
        t3 = s1.add_task("t3")
        self.defs = defs
 
    def test_alternative(self):
        
        defs = Defs().add(
                 Suite("s1").add(
                    Task("t1"),Task("t2").add(
                        Trigger("t1 == active and t3 == aborted"),
                        Complete("t3 == complete")),
                    Task("t3")))
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
 
    def test_alternative1(self):
         
        defs = Defs().add(Suite("s1"))
        defs.s1 += [ Task("t{}".format(i)) for i in range(1,4) ]
        defs.s1.t2 += [ Trigger("t1 == active and t3 == aborted"),
                        Complete("t3 == complete") ]
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

class TestAddLargeTrigger(unittest.TestCase):
    
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        t1 = s1.add_task("t1")
        t2 = s1.add_task("t2")
        t3 = s1.add_task("t3")
        t3.add_part_trigger( "t1 == complete")
        t3.add_part_trigger( "t2 == active", True) # here True means add as 'AND'
        t3.add_part_trigger( "t2 == aborted", False) # here False means add as 'OR'
        self.defs = defs

    def test_alternative(self):
         
        defs = Defs().add(
                Suite("s1").add(
                    Task("t1"),
                    Task("t2"),
                    Task("t3").add(
                        Trigger("t1 == complete"), 
                        Trigger("t2 == active"),
                        Trigger("t2 == aborted",False),
                        )
                    ))
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

    def test_alternative1(self):
         
        defs = Defs().add(Suite("s1"))
        defs.s1 += [ Task("t{}".format(i)) for i in range(1,4) ]
        defs.s1.t3 += [ Trigger("t1 == complete"),
                        Trigger("t2 == active"),
                        Trigger("t2 == aborted",False) ]
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
class TestAddTimeDependencies(unittest.TestCase):
    
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        t1 = s1.add_task("date")
        t1.add_date(1, 0, 0)       # first of every month and every year
        t1.add_date(Date("2.*.*")) # second of every month and every year
        t1.add_date(28,2,2026)     # 28 february 2026

        t2 = s1.add_task("day")
        t2.add_day("monday")
        t2.add_day(Days.tuesday)
        
        t3 = s1.add_task("time")
        t3.add_time("+00:30")                     # 30 minutes after suite has begun
        t3.add_time("+00:30 20:00 01:00")         # 00:30,01:30,02:30....07:30 after suite start
              
        t3.add_time(0, 59, True)                  #  00:59 - 59 minutes past midnight
        t3.add_time(Time(TimeSlot(20, 10)))       #  20:10 - 10 minutes pas eight
        t3.add_time(Time(TimeSlot(20, 20), True)) # +20:20 - 20 minutes and 20 hours, after suite start

        start = TimeSlot(0, 0)
        finish = TimeSlot(23, 0)
        incr = TimeSlot(0, 30)
        time_series = TimeSeries(start, finish, incr, True)        
        t3.add_time(Time(time_series))
        t3.add_time(Time(0, 10))                  #  00:10 
        t3.add_time(Time("+00:40"))               # +00:40
        t3.add_time(Time("+00:40 20:00 01:00"))   # 00:40,01:40,02:30...17:40 after suite start
        
        cron = Cron()
        cron.set_week_days( [0,1,2,3,4,5,6] )
        cron.set_days_of_month( [1,2,3,4,5,6] )
        cron.set_months( [1,2,3,4,5,6] )
        cron.set_time_series( "+00:00 23:00 00:30" )
        s1.add_task("cron").add_cron( cron );
        
        self.defs = defs
        
    def test_alternative(self):
        start = TimeSlot(0, 0)
        finish = TimeSlot(23, 0)
        incr = TimeSlot(0, 30)
        time_series = TimeSeries(start, finish, incr, True)
        
        cron = Cron()
        cron.set_week_days( [0,1,2,3,4,5,6] )
        cron.set_days_of_month( [1,2,3,4,5,6] )
        cron.set_months( [1,2,3,4,5,6] )
        cron.set_time_series( "+00:00 23:00 00:30" )

        defs = Defs().add(
            Suite("s1").add(
                Task("date").add(
                    Date(1, 0, 0),
                    Date("2.*.*"),
                    Date(28,2,2026)),
                Task("day").add(
                    Day("monday"),
                    Day(Days.tuesday)),
                Task("time").add(
                    Time("+00:30"),
                    Time("+00:30 20:00 01:00"),
                    Time(0, 59, True),
                    Time(TimeSlot(20, 10)),
                    Time(TimeSlot(20, 20), True),
                    Time(time_series),
                    Time(0, 10),
                    Time("+00:40"),
                    Time("+00:40 20:00 01:00")),
                Task("cron").add(
                    cron)))

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative1(self):
        start = TimeSlot(0, 0)
        finish = TimeSlot(23, 0)
        incr = TimeSlot(0, 30)
        time_series = TimeSeries(start, finish, incr, True)
        
        cron = Cron()
        cron.set_week_days( [0,1,2,3,4,5,6] )
        cron.set_days_of_month( [1,2,3,4,5,6] )
        cron.set_months( [1,2,3,4,5,6] )
        cron.set_time_series( "+00:00 23:00 00:30" )

        defs = Defs().add( Suite("s1").add(
                             Task("date"),Task("day"),Task("time"),Task("cron")))
        defs.s1.date += [ Date(1, 0, 0), Date("2.*.*"), Date(28,2,2026) ]
        defs.s1.day += [ Day("monday"), Day(Days.tuesday) ]
        defs.s1.time += [ Time("+00:30"), Time("+00:30 20:00 01:00"), Time(0, 59, True),
                          Time(TimeSlot(20, 10)), Time(TimeSlot(20, 20), True),
                          Time(time_series), Time(0, 10), Time("+00:40"),
                          Time("+00:40 20:00 01:00") ]
        defs.s1.cron += [cron]

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
