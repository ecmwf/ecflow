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

class TestAddSuiteFamilyTask0(unittest.TestCase):
    def setUp(self):
        defs = Defs()
        s = Suite('s1')
        f = Family('f1')
        t = Task('t1')
        defs.add_suite(s)
        s.add_family(f)
        f.add_task(t)
        self.defs = defs

    def test_0(self):
        defs = Defs()
        defs.add_suite('s1').add_family('f1').add_task('t1')
        self.assertEqual(self.defs, defs, "defs not equal")
        
    def test_1(self):
        defs = Defs().add(
            Suite('s1').add(
                Family('f1').add(
                    Task('t1'))))
        self.assertEqual(self.defs, defs, "defs not equal")

    def test_2(self):
        defs = Defs(
            Suite('s1',
                  Family('f1',
                         Task('t1'))))
        self.assertEqual(self.defs, defs, "defs not equal")

    def test_3(self):
        defs = Defs() + (Suite('s1') + (Family('f1') + Task('t1')))
        self.assertEqual(self.defs, defs, "defs not equal")

    def test_3(self):
        defs = Defs() + (Suite('s1') + (Family('f1') + Task('t1')))
        self.assertEqual(self.defs, defs, "defs not equal")
          
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
                    [ Task("t{0}".format(t)) 
                      for t in ("a", "b", "c")] )))
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
                     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
 
    def test_me3(self):
        defs = Defs()
        defs += Suite("s1") 
        defs.s1 += Family("f1") 
        defs.s1.f1 += [ Task("t{0}".format(t)) 
                        for t in ("a", "b", "c")] 
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
                     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
        
    def test_me4(self):
        defs = Defs(
                Suite("s1",
                    Family("f1",
                        [ Task("t{0}".format(t)) for t in ("a", "b", "c")])))
        defs.save_as_defs("test.def")   # save defs to file "test.def"  
                     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
 
    def test_me5(self):
        defs = Defs().add(   
          [ Suite("s{0}".format(i)).add( 
              [ Family("f{0}".format(i)).add( 
                  [ Task("t{0}".format(i)) for i in range(1,6)] ) 
                for i in range(1,6)]  ) 
            for i in range(1,6) ] )
        assert len(defs)==5, " expected 5 suites but found " + str(len(defs))
        for suites in defs:
            assert len(suites)==5, " expected 5 familes but found " + str(len(suites))
            for fam in suites:
                assert len(fam)==5, " expected 5 tasks but found " + str(len(fam)) 
                
    def test_me6(self):
        defs = Defs(
                [ Suite("s{0}".format(i),
                    [ Family("f{0}".format(i),
                        [ Task("t{0}".format(i)) for i in range(1,6)] )
                    for i in range(1,6)]  ) 
                for i in range(1,6) ] )
        assert len(defs)==5, " expected 5 suites but found " + str(len(defs))
        for suites in defs:
            assert len(suites)==5, " expected 5 familes but found " + str(len(suites))
            for fam in suites:
                assert len(fam)==5, " expected 5 tasks but found " + str(len(fam))     
             
    def tearDown(self):
        unittest.TestCase.tearDown(self)
        try: os.remove("test.def")
        except: pass
         
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
        defs = Defs() + Suite("s1")
        defs.s1 += Task("t1") + Event(2) + Event("wow") + \
                    Event(10,"Eventname2" ) + Meter("metername3",0,100) + \
                    Label("label_name4", "value") 
         
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
        
    def test_alternative3(self):
        defs = Defs( 
                Suite('s1',
                    Task("t1",
                        Event(2),
                        Event("wow"),
                        Event(10,"Eventname2" ),
                        Meter("metername3",0,100),
                        Label("label_name4", "value")))) 
         
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
            f1.add_task( "t{0}".format(i))
             
        self.defs = defs
 
    def test_alternative(self):
        defs = Defs().add(
            Suite("s1").add(
                Limit("limitName4", 10),
                Family("f1").add(
                    InLimit("limitName4","/s1/f1",2),
                    [ Task("t{0}".format(t)) for t in range(1,4) ]
                    )))
 
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")        
 
    def test_alternative1(self):
        defs = Defs() + Suite("s1") 
        defs.s1 += [ Limit("limitName4", 10),Family("f1") ]
        defs.s1.f1 += [ InLimit("limitName4","/s1/f1",2),
                        [ Task("t{0}".format(t)) for t in range(1,4) ] ]
 
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
                    f1 += [ Task("t{0}".format(t)) for t in range(1,4) ]
                     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative3(self):
        defs = Defs( 
            Suite("s1",
                Limit("limitName4", 10),# name, maximum token
                Family("f1",
                    InLimit("limitName4","/s1/f1",2), # limit name, path to limit, tokens consumed
                    [ Task("t{0}".format(t)) for t in range(1,4) ] )))
                     
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

    def test_alternative0(self):
         
        defs = Defs(Suite("s1",HELLO="world",FRED="bloggs",BILL=1,NAME="value",NAME2="value2"))
        defs.s1.add_variable('NAME4',4)
        defs.s1 += Edit(NAME3="value3")
 
        defs.s1.sort_attributes("variable");
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
 
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
         
        defs = Defs() + Suite("s1")
        defs.s1 += [ Edit(HELLO="world"),
                     Edit({ "NAME":"value", "NAME2":"value2", "NAME3":"value3", "NAME4":4 }, BILL=1),
                     Edit(FRED="bloggs")
                   ]
 
        defs.s1.sort_attributes("variable");
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative2(self):
         
        defs = Defs(
                Suite("s1",
                    { "HELLO":"world", "NAME":"value", "NAME2":"value2", 
                     "NAME3":"value3", "NAME4":4, "BILL":1, "FRED":"bloggs" }))
 
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

    def test_alternative0(self):
        
        defs = Defs(
                Suite("s1",
                    Task("t1"),
                    Task("t2",
                        Trigger("t1 == active and t3 == aborted"),
                        Complete("t3 == complete")),
                    Task("t3")))
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
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
         
        defs = Defs() + Suite("s1")
        defs.s1 += [ Task("t{0}".format(i)) for i in range(1,4) ]
        defs.s1.t2 += [ Trigger("t1 == active and t3 == aborted"),
                        Complete("t3 == complete") ]
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

class TestAddTaskChain(unittest.TestCase):

    def setUp(self):
        defs = Defs(
                Suite("s1",
                    Task("t1"),
                    Task("t2",Trigger( "t1 == complete" )),
                    Task("t3",Trigger( "t2 == complete" )),
                    Task("t4",Trigger( "t3 == complete" ))))
        self.defs = defs
        
    def test_alternative(self):
        defs = Defs() + Suite("s1")
        defs.s1 += [ Task("t1"),Task("t2"),Task("t3"),Task("t4") ]
        defs.s1.t2 += Trigger( ["t1"] )
        defs.s1.t3 += Trigger( ["t2"] )
        defs.s1.t4 += Trigger( ["t3"] )

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

    def test_alternative1(self):
        defs = Defs() + Suite("s1")
        defs.s1 += [ Task("t1"),Task("t2"),Task("t3"),Task("t4") ]
        defs.s1.t2 += Trigger( [ defs.s1.t1 ] )
        defs.s1.t3 += Trigger( [ defs.s1.t2 ] )
        defs.s1.t4 += Trigger( [ defs.s1.t3 ] )

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative1(self):
        defs = Defs(Suite("s1"))
        defs.s1 >> Task("t1") >> Task("t2") >> Task("t3") >> Task("t4")

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
class TestAddReverseTaskChain(unittest.TestCase):

    def setUp(self):
        defs = Defs(
                Suite("s1",
                    Task("t1", Trigger( "t2 == complete" )),
                    Task("t2", Trigger( "t3 == complete" )),
                    Task("t3", Trigger( "t4 == complete" )),
                    Task("t4")))
        self.defs = defs
        
    def test_alternative1(self):
        defs = Defs() + Suite("s1")
        defs.s1 << Task("t1") << Task("t2") << Task("t3") << Task("t4")

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

    def test_alternative0(self):
        defs = Defs( 
                Suite("s1",
                    Task("t1"),
                    Task("t2"),
                    Task("t3",
                        Trigger("t1 == complete"), 
                        Trigger("t2 == active"),
                        Trigger("t2 == aborted",False))))
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
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
         
        defs = Defs() + Suite("s1")
        defs.s1 += [ Task("t{0}".format(i)) for i in range(1,4) ]
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
        
    def test_alternative0(self):
        start = TimeSlot(0, 0)
        finish = TimeSlot(23, 0)
        incr = TimeSlot(0, 30)
        time_series = TimeSeries(start, finish, incr, True)
        
        defs = Defs( 
                Suite("s1",
                    Task("date",
                        Date(1, 0, 0),                   # first of every month and every year
                        Date("2.*.*"),                   # second of every month and every yea
                        Date(28,2,2026)),                # 28 february 2026
                    Task("day",
                        Day("monday"),
                        Day(Days.tuesday)),
                    Task("time",
                        Time("+00:30"),                  # 30 minutes after suite has begun
                        Time("+00:30 20:00 01:00"),      # 00:30,01:30,02:30....07:30 after suite start
                        Time(0, 59, True),               # 00:59 - 59 minutes past midnight
                        Time(TimeSlot(20, 10)),          # 20:10 - 10 minutes pas eight
                        Time(TimeSlot(20, 20), True),    # +20:20 - 20 minutes and 20 hours, after suite start
                        Time(time_series),
                        Time(0, 10),
                        Time("+00:40"),
                        Time("+00:40 20:00 01:00")),
                    Task("cron",
                        Cron("+00:00 23:00 00:30",days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6],months=[1,2,3,4,5,6]))))

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
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
        
        defs = Defs() + ( Suite("s1") + Task("date") + Task("day") + Task("time") + Task("cron"))
        defs.s1.date += [ Date(1, 0, 0), Date("2.*.*"), Date(28,2,2026) ]
        defs.s1.day += [ Day("monday"), Day(Days.tuesday) ]
        defs.s1.time += [ Time("+00:30"), Time("+00:30 20:00 01:00"), Time(0, 59, True),
                          Time(TimeSlot(20, 10)), Time(TimeSlot(20, 20), True),
                          Time(time_series), Time(0, 10), Time("+00:40"),
                          Time("+00:40 20:00 01:00") ]
        defs.s1.cron += Cron("+00:00 23:00 00:30",
                             days_of_week=[0,1,2,3,4,5,6],
                             days_of_month=[1,2,3,4,5,6],
                             months=[1,2,3,4,5,6])  

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
class TestAddDefStatus(unittest.TestCase):
    
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        s1.add_task("t1").add_defstatus(Defstatus("complete"))
        s1.add_task("t2").add_defstatus(DState.complete)
        
        self.defs = defs

    def test_alternative0(self):
        defs = Defs( 
                Suite("s1",
                    Task("t1",Defstatus("complete")),
                    Task("t2",Defstatus(DState.complete))))
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

    def test_alternative(self):
        defs = Defs().add(
            Suite("s1").add(
                Task("t1").add(Defstatus("complete")),
                Task("t2").add(Defstatus(DState.complete))))
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")

    def test_alternative1(self):
        defs = Defs() + ( Suite("s1") +  Task("t1") + Task("t2") )
        defs.s1.t1 += Defstatus("complete") 
        defs.s1.t2 += Defstatus(DState.complete) 
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
   
class TestAddAutocancel(unittest.TestCase):
    
    def setUp(self):
        defs = Defs()
        s1 = defs.add_suite("s1") 
        s1.add_task("t1").add_autocancel(3)                                # delete task after 3 days after completion 
        s1.add_task("t2").add_autocancel(1, 10, True)                      # delete task 1hr 10 min after task completion
        s1.add_task("t3").add_autocancel(TimeSlot(2,10), True)             # delete task 2hr 10 min after task completion 
        s1.add_task("t4").add_autocancel(Autocancel(1))                    # delete task after 1 day after task completion
        s1.add_task("t5").add_autocancel(Autocancel( 18, 10, False))       # delete task at 6:10pm once it has completed
        s1.add_task("t6").add_autocancel(Autocancel(TimeSlot(2,10), False))# delete task at 2:10am once it has completed

        self.defs = defs

    def test_alternative0(self):
        defs = Defs( 
                Suite("s1",
                    Task("t1", Autocancel(3)),                    # delete task after 3 days after completion 
                    Task("t2", Autocancel(1, 10, True)),          # delete task 1hr 10 min after task completion
                    Task("t3", Autocancel(TimeSlot(2,10), True)), # delete task 2hr 10 min after task completion
                    Task("t4", Autocancel(1)),                    # delete task after 1 day after task completion
                    Task("t5", Autocancel(18, 10, False)),        # delete task at 6:10pm once it has completed
                    Task("t6", Autocancel(2, 10, False))))        # delete task at 2:10am once it has completed

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative(self):
        defs = Defs().add(
            Suite("s1").add(
                Task("t1").add(Autocancel(3)),
                Task("t2").add(Autocancel(1, 10, True)),
                Task("t3").add(Autocancel(TimeSlot(2,10), True)),
                Task("t4").add(Autocancel(1)),
                Task("t5").add(Autocancel(18, 10, False)),
                Task("t6").add(Autocancel(2, 10, False))))

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")


    def test_alternative1(self):
        defs = Defs() + ( Suite("s1") + [ Task("t{0}".format(i)) for i in range(1,7)] )
        defs.s1.t1 += Autocancel(3) 
        defs.s1.t2 += Autocancel(1, 10, True) 
        defs.s1.t3 += Autocancel(TimeSlot(2,10), True) 
        defs.s1.t4 += Autocancel(1)     
        defs.s1.t5 += Autocancel(18, 10, False) 
        defs.s1.t6 += Autocancel(2, 10, False)       
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
   
class TestAddRepeat(unittest.TestCase):
    def setUp(self):
        def add_tasks(fam):
            for i in range(1,3):
                fam.add_task(Task("t{0}".format(i)))
                
        defs = Defs()
        s1 = defs.add_suite("s1")
        f1 = s1.add_family("f1")
        f1.add_repeat( RepeatDate("YMD",20100111,20100115,2) )
        add_tasks(f1)

        f2 = s1.add_family("f2")
        f2.add_repeat( RepeatInteger("count",0,100,2)  )
        add_tasks(f2)

        f3 = s1.add_family("f3")
        f3.add_repeat( RepeatEnumerated("enum",["red", "green", "blue" ] )  )
        add_tasks(f3)

        f4 = s1.add_family("f4")
        f4.add_repeat( RepeatString("enum",["a", "b", "c" ] )  )
        add_tasks(f4)

        f5 = s1.add_family("f5")
        f5.add_repeat( RepeatDay(1)  )
        add_tasks(f5)
          
        self.defs = defs
     

    def test_alternative0(self):
        defs = Defs( 
                Suite("s1",
                    Family("f1",
                       RepeatDate("YMD",20100111,20100115,2),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f2",
                       RepeatInteger("count",0,100,2),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f3",
                       RepeatEnumerated("enum",["red", "green", "blue" ] ),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f4",
                        RepeatString("enum",["a", "b", "c" ] ),
                        [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f5",
                        RepeatDay(1),
                        [ Task("t{0}".format(i)) for i in range(1,3) ] )))

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
    def test_alternative1(self):
        defs = Defs().add(
                Suite("s1").add(
                    Family("f1").add(
                       RepeatDate("YMD",20100111,20100115,2),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f2").add(
                       RepeatInteger("count",0,100,2),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f3").add(
                       RepeatEnumerated("enum",["red", "green", "blue" ] ),
                       [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f4").add(
                        RepeatString("enum",["a", "b", "c" ] ),
                        [ Task("t{0}".format(i)) for i in range(1,3) ] ),
                    Family("f5").add(
                        RepeatDay(1),
                        [ Task("t{0}".format(i)) for i in range(1,3) ] )))

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
 
    def test_alternative2(self):
        defs = Defs() + Suite("s1") 
        defs.s1 += [ Family("f{0}".format(i)).add(
                      [ Task("t{0}".format(i)) for i in range(1,3) ]) 
                    for i in range(1,6) ]   
        defs.s1.f1 += RepeatDate("YMD",20100111,20100115,2) 
        defs.s1.f2 += RepeatInteger("count",0,100,2) 
        defs.s1.f3 += RepeatEnumerated("enum",["red", "green", "blue" ] ) 
        defs.s1.f4 += RepeatString("enum",["a", "b", "c" ] ) 
        defs.s1.f5 += RepeatDay(1) 
  
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")
        
        
class TestAddLate(unittest.TestCase):
    def setUp(self):
        defs = Defs()
        suite = defs.add_suite('s1')
        
        late = Late()
        late.submitted( 20,10 )         # hour, min
        late.active( 2, 10 )            # hour, min
        late.complete( 3, 10, True)     # hour, min, relative
        suite.add_task('t1').add_late(late)
        self.defs = defs
 
    def test_1(self):
        
        # Can also pass late into the Task constructor
        defs = Defs(
                Suite('s1',
                    Task('t1',
                        Late(submitted='20:10',active='02:10',complete='+03:10'))))
     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")   
        
    def test_2(self):
        
        # Can also pass late into the Task constructor
        defs = Defs() + (Suite('s1') + Task('t1'))
        defs.s1.t1 += Late(submitted='20:10',active='02:10',complete='+03:10')
     
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(defs, self.defs, "expected defs to be the same")   
        
        
class Deadlock(unittest.TestCase):
    def setUp(self):
        defs = Defs().add(
        Suite("dead_lock").add(
            Family("family").add(
               Task("t1").add( Trigger("t2 == complete")),
               Task("t2").add( Trigger("t1 == complete")))))
        
        self.defs = defs
        
    def test_me(self):
        defs = Defs(
            Suite("dead_lock",
                Family('family',
                    Task('t1', 
                        Trigger("t2 == complete")),
                    Task('t2', 
                        Trigger("t1 == complete")))))
        
        self.assertEqual(defs, self.defs, "expected defs to be the same")
       
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
