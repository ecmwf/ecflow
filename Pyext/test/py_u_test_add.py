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
from ecflow import *
import unittest 

class TestDefsAdd(unittest.TestCase):
    def test_add_suite1(self):
        defs = Defs().add(Suite("a"))
        self.assertEqual(len(defs),1,"Expected 1 suite to be added")

    def test_add_suite3(self):
        defs = Defs().add(Suite("a"),Suite("b"),Suite("c").add(Edit(a="b")))
        self.assertEqual(len(defs),3,"Expected 3 suite to be added")
        
class TestSuiteAdd(unittest.TestCase):
    def test_add_suite1(self):
        s = Suite("a").add(Family("a").add(Edit(a="b")))
        self.assertEqual(len(s),1,"Expected 1 child to be added")

class TestFamilyAdd(unittest.TestCase):
    def test_add_task(self):
        s = Family("a").add(Task("a").add(Edit(a="b")))
        self.assertEqual(len(s),1,"Expected 1 child to be added")

class TestDefstatus(unittest.TestCase):
    def test_illegal_defstatus(self):
        with self.assertRaises(RuntimeError):
            Defstatus("fred")

class TestAll(unittest.TestCase):
    def setUp(self):
        self.cron = Cron()
        self.cron.set_time_series(TimeSeries(TimeSlot(23 , 0) ,True)) # True means relative to suite start
        self.late = Late()
        self.late.submitted(TimeSlot(20, 10))
        self.late.active(TimeSlot(20, 10))
        self.late.complete(TimeSlot(20, 10), True)
        self.late2 = Late()
        self.late2.submitted(20, 10)
        self.late2.active(20, 10)
        self.late2.complete(20, 10, True)
    
        self.defs1 = Defs()
        self.defs1.add_suite("s1").add_task("t1").add_variable("a","v").add_event(1).add_event(11,"event").add_meter("meter",0,10,10).add_label("label","c")
        self.defs1.add_suite("s2").add_family("f1").add_task("t1").add_trigger("1==1").add_complete("1==1")
        self.defs1.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable("var","v")
        self.defs1.add_suite("edit").add_variable("a","a").add_variable("b","b") 
        self.defs1.add_suite("limit").add_limit("limit",10).add_limit("limit2",10) 
        self.defs1.add_suite("inlimit").add_inlimit("limitName", "/limit", 2)  
        self.defs1.add_suite("RepeatInteger").add_repeat(RepeatInteger("integer", 0, 100, 2)) 
        self.defs1.add_suite("RepeatEnumerated").add_repeat(RepeatEnumerated("enum", ["red", "green", "blue" ])) 
        self.defs1.add_suite("RepeatDate").add_repeat(RepeatDate("ymd", 20100111, 20100115, 2))
        self.defs1.add_suite("RepeatString").add_repeat(RepeatString("string", ["a", "b", "c" ]))
        self.defs1.add_suite("RepeatDay").add_repeat(RepeatDay(1))
        self.defs1.add_suite("defstatus").add_defstatus(DState.active) 
        self.defs1.add_suite("today").add_task("today").add_today("00:30").add_today(0,59).add_today("00:00 11:30 00:01")
        self.defs1.add_suite("time").add_task("time").add_time("00:30").add_time(0,59).add_time("00:00 11:30 00:01")
        self.defs1.add_suite("day").add_task("day").add_day("sunday").add_day(Days.monday)
        self.defs1.add_suite("date").add_task("date").add_date(1,1,0).add_date(28,2,1960)
        self.defs1.add_suite("cron").add_task("cron").add_cron(self.cron)
        self.defs1.add_suite("late").add_family("late").add_late(self.late).add_task("late").add_late(self.late2)
        self.defs1.add_suite("clock").add_clock(Clock(1, 1, 2010, False))
        self.defs1.add_suite("autocancel").add_autocancel(3)
        
        zombie_suite = self.defs1.add_suite("zombie") 
        self.zombie_life_time_in_server = 800
        self.child_list = [ ChildCmdType.init, ChildCmdType.event, ChildCmdType.meter, ChildCmdType.label, ChildCmdType.wait, ChildCmdType.abort,ChildCmdType.complete ]
        zombie_type_list = [ ZombieType.ecf, ZombieType.user,ZombieType.path ]
        for zombie_type in zombie_type_list:
            zombie_attr = ZombieAttr(zombie_type, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server)
            zombie_suite.add_zombie(zombie_attr)
   
 
    def test_compare(self):
        defs = Defs().add(
            Suite("s1").add(
                Task("t1").add(
                    Edit(a="v"),
                    Event(1),
                    Event(11,"event"),
                    Meter("meter",0,10,10),
                    Label("label","c")
                )
            ),
            Suite("s2").add(
                Family("f1").add(
                    Task("t1").add(
                        Trigger("1==1"),
                        Complete("1==1")
                    )
                )
            ),
            Suite("s3").add(
                Family("f1").add(
                    Family("f2").add(
                        Task("t1").add(
                            Edit(var = "v")
                        )
                    )
                )
            ),
            Suite("edit").add(  Edit({"a": "a", "b" : "b"}) ),
            Suite("limit").add( Limit("limit",10),Limit("limit2",10)),
            Suite("inlimit").add( InLimit("limitName","/limit",2)),
            Suite("RepeatInteger").add( RepeatInteger("integer", 0, 100, 2)),
            Suite("RepeatEnumerated").add( RepeatEnumerated("enum", ["red", "green", "blue" ])),
            Suite("RepeatDate").add( RepeatDate("ymd", 20100111, 20100115, 2)),
            Suite("RepeatString").add( RepeatString("string", ["a", "b", "c" ])),
            Suite("RepeatDay").add( RepeatDay(1)),
            Suite("defstatus").add( Defstatus(DState.active)),
            Suite("today").add(
                Task("today").add(
                     Today(0,30),Today("00:59"),Today("00:00 11:30 00:01")
                )
            ),
            Suite("time").add(
                Task("time").add(
                     Time(0,30),Time("00:59"),Time("00:00 11:30 00:01")
                )
            ),
            Suite("day").add(
                Task("day").add(
                    Day("sunday"),Day(Days.monday)
                )
            ),
            Suite("date").add(
                Task("date").add(
                    Date(1,1,0),Date(28,2,1960)
                )
            ),
            Suite("cron").add(
                Task("cron").add(
                    self.cron
                )
            ),
            Suite("late").add(
                Family("late").add(
                    self.late,
                    Task("late").add(
                        self.late2
                    )
                )
            ),
            Suite("clock").add( Clock(1, 1, 2010, False)),
            Suite("autocancel").add( Autocancel(3)),
            Suite("zombie").add( 
                ZombieAttr( ZombieType.ecf, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                ZombieAttr( ZombieType.user, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                ZombieAttr( ZombieType.path, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server)
            )
        )      
        self.assertEqual(self.defs1,defs,"defs not equal\n" + str(self.defs1) + "\n\n" + str(defs))    
        
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
