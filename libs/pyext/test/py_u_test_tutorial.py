#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

from ecflow import Alias, AttrType, Autocancel, CheckPt, ChildCmdType, Client, Clock, Cron, DState, Date, Day, Days, \
    Defs, Ecf, Event, Expression, Family, FamilyVec, File, Flag, FlagType, FlagTypeVec, InLimit, \
    JobCreationCtrl, Label, Late, Limit, Meter, Node, NodeContainer, NodeVec, PartExpression, PrintStyle, \
    Repeat, RepeatDate, RepeatDay, RepeatEnumerated, RepeatInteger, RepeatString, SState, State, Style, \
    Submittable, Suite, SuiteVec, Task, TaskVec, Time, TimeSeries, TimeSlot, Today, UrlCmd, Variable, \
    VariableList, Verify, WhyCmd, ZombieAttr, ZombieType, ZombieUserActionType, Trigger, Complete, Edit, Defstatus
import ecflow_test_util as Test
import unittest
import shutil  # used to remove directory tree
import os, sys


def test_def_file():
    # return "test.def"
    return "test_tutorial_def_" + str(os.getpid()) + ".def"


def test_compile(text):
    # replace test.def in the text with test_def_file() so that we have a unique file per process
    text = text.replace('test.def', test_def_file())

    test_file = "py_u_test_tutorial_" + str(os.getpid()) + ".def"
    file = open(test_file, 'w')
    file.write(text)
    file.close()

    # execfile(test_file) only work for python 2.7
    with open(test_file) as f:
        code = compile(f.read(), test_file, 'exec')
        exec(code)

    os.remove(test_file)


def do_tear_down():
    Ecf.set_debug_equality(False)
    try:
        os.remove(test_def_file())
    except:
        pass


######################################################################################################
class TestNewSuite(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        home = os.path.join(os.getenv("HOME"), "course")
        self.defs = Defs()
        suite = self.defs.add_suite("test")
        suite.add_variable("ECF_HOME", home)
        suite.add_task("t1")

        with Defs() as self.defs2:
            with self.defs2.add_suite("test") as suite:
                suite.add_variable("ECF_HOME", home)
                suite.add_task("t1")

        with Defs() as self.defs3:
            self.defs3.add(
                Suite("test",
                      Edit(ECF_HOME=home),
                      Task("t1")))

        self.defs4 = Defs() + (Suite("test") + Edit(ECF_HOME=home))
        self.defs4.test += Task("t1")

        self.defs5 = Defs().add(
            Suite("test").add(
                Edit(ECF_HOME=home),
                Task("t1")))

        # print("Creating suite definition")
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs(
            Suite('test',
                  Task('t1'),
                  ECF_HOME=home))
        self.assertEqual(self.defs, defs, "defs not equal\n" + str(self.defs) + "\n" + str(defs))

        text = """import os
from ecflow import Defs,Suite,Task,Edit
   
#xx print("Creating suite definition")
home = os.path.join(os.getenv("HOME"),  "course")
defs = Defs( 
        Suite('test',
            Edit(ECF_HOME=home),
            Task('t1')))
#xx print(defs)

defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, defs, "defs not equal\n" + str(test_defs) + "\n" + str(defs))

    def test_defs_equal(self):
        self.assertEqual(self.defs, self.defs2, "defs not the equal")
        self.assertEqual(self.defs, self.defs3, "defs not the equal")
        self.assertEqual(self.defs, self.defs4, "defs not the equal")
        self.assertEqual(self.defs, self.defs5, "defs not the equal")

    def tearDown(self):
        do_tear_down()


######################################################################################################
class TestFamilies(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
                Family("f1").add(
                    Task("t1"),
                    Task("t2"))))
        defs.save_as_defs(test_def_file())

        self.defs = defs

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit
         
def create_family_f1():
    return Family("f1",
                Task("t1"),
                Task("t2"))
            
#xx print("Creating suite definition") 
home = os.path.join(os.getenv("HOME"),  "course") 
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        # print("Creating suite definition")
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + (Suite("test") + Edit(ECF_HOME=home) + Edit(ECF_INCLUDE=home))
        defs.test += Family("f1") + [Task("t{0}".format(i)) for i in range(1, 3)]
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestVariables(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Task("t1").add(Edit(SLEEP=20)),
                Task("t2").add(Edit(SLEEP=20)))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f1()))

        defs.save_as_defs(test_def_file())

        self.defs = defs

    def test_me0(self):
        text = """#import os
from ecflow import Defs,Suite,Family,Task,Edit

def create_family_f1():
    return Family("f1",
                Task("t1",Edit(SLEEP=20)),
                Task("t2",Edit(SLEEP=20)))
  
#xx print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
        Suite("test", 
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))         
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def') 
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me2(self):
        import os
        home = os.path.join(os.getenv("HOME"), "course")

        with Defs() as defs:
            defs += Suite("test").add(
                Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home))
            defs.test += Family("f1").add(
                Task("t1").add(Edit(SLEEP=20)),
                Task("t2").add(Edit(SLEEP=20)))
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def test_me3(self):
        import os
        home = os.path.join(os.getenv("HOME"), "course")

        defs = Defs() + (Suite("test") + Edit(ECF_HOME=home) + Edit(ECF_INCLUDE=home))
        defs.test += Family("f1") + (Task("t1") + Edit(SLEEP=20)) + (Task("t2") + Edit(SLEEP=20))
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestVariableInheritance(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2"))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f1()))
        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit

def create_family_f1():
    return Family("f1",
            Edit(SLEEP=20),
                Task("t1"),
                Task("t2"))
             
#xx print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        def create_family_f1():
            return Family("f1") + Edit(SLEEP=20) + Task("t1") + Task("t2")

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + (Suite("test") + create_family_f1())
        defs.test += [Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home)]
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestTriggers(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2").add(Trigger("t1 == complete")))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f1()))
        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger

def create_family_f1():
    return Family("f1",
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2",Trigger("t1 == complete")))
             
#xx print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        with Suite("test") as suite:
            suite += [Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home)]
            suite += Family("f1") + Edit(SLEEP=20) + Task("t1") + Task("t2")
            suite.f1.t2 += Trigger(["t1"])

        defs = Defs().add(suite)
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestEvents(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2").add(
                    Trigger("t1 == complete"),
                    Event("a"),
                    Event("b")),
                Task("t3").add(Trigger("t2:a")),
                Task("t4").add(Trigger("t2:b")))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
                create_family_f1()))
        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Event

def create_family_f1():
    return Family("f1",
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2",
                    Trigger("t1 == complete"),
                    Event("a"),
                    Event("b")),
                Task("t3",
                    Trigger("t2:a")),
                Task("t4",
                    Trigger("t2:b")))
             
#xx print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        def create_family_f1():
            f1 = Family("f1") + [Edit(SLEEP=20),
                                 Task("t1"),
                                 Task("t2") + Trigger(["t1"]) + Event("a") + Event("b"),
                                 Task("t3") + Trigger("t2:a"),
                                 Task("t4") + Trigger("t2:b")]
            return f1

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + Suite("test")
        defs.test += [Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home), create_family_f1()]
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestComplete(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2").add(Trigger("t1 == complete"),
                               Event("a"), Event("b")),
                Task("t3").add(Trigger("t2:a")),
                Task("t4").add(Trigger("t2 == complete"),
                               Complete("t2:b")))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f1()))
        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event

def create_family_f1():
    return Family("f1",
                Edit(SLEEP= 20),
                Task("t1"),
                Task("t2",
                    Trigger("t1 == complete"),
                    Event("a"), 
                    Event("b")),
                Task("t3",
                    Trigger("t2:a")),
                Task("t4",
                    Trigger("t2 == complete"), 
                    Complete("t2:b")))
        
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        def create_family_f1():
            f1 = Family("f1") + Edit(SLEEP=20)
            f1 += [Task("t{0}".format(i)) for i in range(1, 5)]
            f1.t2 += [Trigger(["t1"]), Event("a"), Event("b")]
            f1.t3 += Trigger("t2:a")
            f1.t4 += [Trigger(["t2"]), Complete("t2:b")]
            return f1

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + (Suite("test") + Edit(ECF_HOME=home) + Edit(ECF_INCLUDE=home))
        defs.test += create_family_f1()
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestMeter(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1").add(Meter("progress", 1, 100, 90)),
                Task("t2").add(Trigger("t1 == complete"),
                               Event("a"),
                               Event("b")),
                Task("t3").add(Trigger("t2:a")),
                Task("t4").add(Trigger("t2 == complete"),
                               Complete("t2:b")),
                Task("t5").add(Trigger("t1:progress ge 30")),
                Task("t6").add(Trigger("t1:progress ge 60")),
                Task("t7").add(Trigger("t1:progress ge 90")))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f1()))
        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter

def create_family_f1():
    return Family("f1",
                Edit(SLEEP= 20),
                Task("t1", Meter("progress", 1, 100, 90)),
                Task("t2", Trigger("t1 == complete"), Event("a"), Event("b")),
                Task("t3", Trigger("t2:a")),
                Task("t4", Trigger("t2 == complete"), Complete("t2:b")),
                Task("t5", Trigger("t1:progress ge 30")),
                Task("t6", Trigger("t1:progress ge 60")),
                Task("t7", Trigger("t1:progress ge 90")))  
        
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f1()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        def create_family_f1():
            f1 = Family("f1") + Edit(SLEEP=20)
            f1 += [Task("t{0}".format(i)) for i in range(1, 8)]
            f1.t1 += Meter("progress", 1, 100, 90)
            f1.t2 += [Trigger(["t1"]), Event("a"), Event("b")]
            f1.t3 += Trigger("t2:a")
            f1.t4 += [Trigger(["t2"]), Complete("t2:b")]
            f1.t5 += Trigger("t1:progress ge 30")
            f1.t6 += Trigger("t1:progress ge 60")
            f1.t7 += Trigger("t1:progress ge 90")
            return f1

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + (Suite("test") + Edit(ECF_HOME=home) + Edit(ECF_INCLUDE=home))
        defs.test += create_family_f1()
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestTime(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f2():
            f2 = Family("f2")
            f2.add_variable("SLEEP", 20)
            f2.add_task("t1").add_time("00:30 23:30 00:30")  # start(hh:mm) end(hh:mm) increment(hh:mm)
            f2.add_task("t2").add_day("sunday")

            # for add_date(): day,month,year; here 0 means every month, and every year
            t3 = f2.add_task("t3")
            t3.add_date(1, 0, 0)  # day month year, first of every month or every year
            t3.add_time(12, 0)  # hour, minutes at 12 o'clock

            f2.add_task("t4").add_time(0, 2, True)  # hour, minutes, relative to suite start
            # 2 minutes after family f2 start
            f2.add_task("t5").add_time(0, 2)  # hour, minutes suite site
            # 2 minutes past midnight
            return f2

        defs = Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_HOME", os.path.join(os.getenv("HOME"), "course"))
        suite.add_variable("ECF_INCLUDE", os.path.join(os.getenv("HOME"), "course"))

        # suite.add_family( create_family_f1() )
        suite.add_family(create_family_f2())
        errors = defs.check()
        assert len(errors) == 0, errors

        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date

def create_family_f2():
    return Family("f2",
            Edit(SLEEP=20),
            Task("t1", Time("00:30 23:30 00:30")),     # start(hh:mm) end(hh:mm) increment(hh:mm)
            Task("t2", Day("sunday")),
            Task("t3", Date("1.*.*"), Time("12:00")),  # Date(day,month,year) - * means every day,month,year
            Task("t4", Time("+00:02")),                # + means realative to suite begin/requeue time
            Task("t5", Time("00:02")))                 # 2 minutes past midnight
 
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            #create_family_f1(),
            create_family_f2()
            ))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        def create_family_f2():
            f1 = Family("f2") + Edit(SLEEP=20)
            f1 += [Task("t{0}".format(i)) for i in range(1, 6)]
            f1.t1 += Time("00:30 23:30 00:30")  # start(hh:mm) end(hh:mm) increment(hh:mm)
            f1.t2 += Day("sunday")
            f1.t3 += [Date("1.*.*"),
                      Time("12:00")]
            f1.t4 += Time("+00:02")
            f1.t5 += Time("00:02")
            return f1

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + (Suite("test") + Edit(ECF_HOME=home) + Edit(ECF_INCLUDE=home))
        defs.test += create_family_f2()
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def test_add(self):
        import os

        def create_family_f2():
            return Family("f2").add(
                Edit(SLEEP=20),
                Task("t1").add(Time("00:30 23:30 00:30")),
                Task("t2").add(Day("sunday")),
                Task("t3").add(Date("1.*.*"),
                               Time("12:00")),
                Task("t4").add(Time("+00:02")),
                Task("t5").add(Time("00:02")))

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("test").add(
            Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home),
            create_family_f2()))
        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestCron(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_house_keeping():
            cron = Cron()
            cron.set_week_days([0])
            cron.set_time_series("22:30")
            f2 = Family("house_keeping")
            f2.add_task("clear_log").add_cron(cron)
            return f2

        defs = Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_HOME", os.path.join(os.getenv("HOME"), "course"))
        suite.add_variable("ECF_INCLUDE", os.path.join(os.getenv("HOME"), "course"))

        # suite.add_family( create_family_f1() )
        suite.add_family(create_family_house_keeping())
        errors = defs.check()
        assert len(errors) == 0, errors

        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Cron
  
def create_family_house_keeping():
    return Family("house_keeping",
                Task("clear_log",
                    Cron("22:30",days_of_week=[0])))
          
print("Creating suite definition") 
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_house_keeping()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0,errors

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def tearDown(self):
        do_tear_down()


class TestIndentation(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        # version = sys.version_info
        # if version[1] < 7:
        #    #print "This example requires python version 2.7, but found : " + str(version)
        #    exit(0)

        with Defs() as defs:
            with defs.add_suite("test") as suite:
                suite += Edit(ECF_HOME=os.path.join(os.getenv("HOME"), "course"))
                suite += Edit(ECF_INCLUDE=os.path.join(os.getenv("HOME"), "course"))

            with suite.add_family("f1") as f1:
                f1 += Edit(SLEEP=20)
                f1 += Task("t1", Meter("progress", 1, 100, 90))
                f1 += Task("t2", Trigger("t1 == complete"), Event("a"), Event("b"))
                f1 += Task("t3", Trigger("t2:a"))
                f1 += Task("t4", Trigger("t2 == complete"), Complete("t2:b"))
                f1 += Task("t5", Trigger("t1:progress ge 30"))
                f1 += Task("t6", Trigger("t1:progress ge 60"))
                f1 += Task("t7", Trigger("t1:progress ge 90"))

            with suite.add_family("f2") as f2:
                f2 += Edit(SLEEP=20)
                f2 += Task("t1", Time("00:30 23:30 00:30"))
                f2 += Task("t2", Day("sunday"))
                f2 += Task("t3", Date(1, 0, 0), Time(12, 0))
                f2 += Task("t4", Time(0, 2, True))
                f2 += Task("t5", Time(0, 2))

        defs.save_as_defs(test_def_file())
        self.defs = defs;

    def test_preferred(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date

#xx print("Creating suite definition") 
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            Family("f1",
                Edit(SLEEP=20),
                Task("t1", Meter("progress", 1, 100, 90)),
                Task("t2", Trigger("t1 == complete"),Event("a"),Event("b")),
                Task("t3", Trigger("t2:a")),
                Task("t4", Trigger("t2 == complete"), Complete("t2:b")),
                Task("t5", Trigger("t1:progress ge 30")),
                Task("t6", Trigger("t1:progress ge 60")),
                Task("t7", Trigger("t1:progress ge 90"))),
            Family("f2",
                Edit(SLEEP=20),
                Task("t1", Time( "00:30 23:30 00:30" )),
                Task("t2", Day( "sunday" )),
                Task("t3", Date("1.*.*"), Time("12:00")),
                Task("t4", Time("+00:02")),
                Task("t5", Time("00:02")))))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        with Defs() as defs:
            with defs.add_suite("test") as suite:
                suite += [Edit(ECF_HOME=home), Edit(ECF_INCLUDE=home)]
                with suite.add_family("f1") as f1:
                    f1 += [Task("t{0}".format(i)) for i in range(1, 8)]
                    f1 += Edit(SLEEP=20)
                    f1.t1 += Meter("progress", 1, 100, 90)
                    f1.t2 += [Trigger(["t1"]), Event("a"), Event("b")]
                    f1.t3 += Trigger("t2:a")
                    f1.t4 += [Trigger(["t2"]), Complete("t2:b")]
                    f1.t5 += Trigger("t1:progress ge 30")
                    f1.t6 += Trigger("t1:progress ge 60")
                    f1.t7 += Trigger("t1:progress ge 90")
                with suite.add_family("f2") as f2:
                    f2 += [Edit(SLEEP=20), [Task("t{0}".format(i)) for i in range(1, 6)]]
                    f2.t1 += Time("00:30 23:30 00:30")
                    f2.t2 += Day("sunday")
                    f2.t3 += [Date("1.*.*"), Time("12:00")]
                    f2.t4 += Time("+00:02")
                    f2.t5 += Time("00:02")

        defs.save_as_defs(test_def_file())

        self.assertEqual(self.defs, defs, "defs not equal:\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        do_tear_down()


class TestLabel(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f3():
            fam = Family('f3')
            fam.add_task('t1').add_label("info", "")
            return fam

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_HOME", home)
        suite.add_variable("ECF_INCLUDE", home)

        suite.add_family(create_family_f3())
        errors = defs.check()
        assert len(errors) == 0, errors

        defs.save_as_defs(test_def_file())

        self.defs = defs;

    def test_me(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date,Label

def create_family_f3():
    return Family("f3",
                Task("t1",
                Label("info","")))
 
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f3()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def tearDown(self):
        do_tear_down()


class TestRepeat(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        self.ecf_home = File.build_dir() + "/libs/pyext/test/data/course_py" + str(sys.version_info[0]) + "_" + str(os.getpid())  # allow paralled runs
        self.ecf_includes = File.source_dir() + "/libs/pyext/test/data/includes"
        # print("self.ecf_home ",self.ecf_home )

        try:
            os.makedirs(self.ecf_home + "/test/f4/f5")
        except:
            pass

        t1_ecf = '%include <head.h>\n'
        t1_ecf += 'ecflow_client --label=info "My name is %NAME% My value is %VALUE% My date is %DATE%\n'
        t1_ecf += 'ecflow_client --label=date "year:%DATE_YYYY% month:%DATE_MM% day of month:%DATE_DD% day of week:%DATE_DOW%"\n'
        t1_ecf += 'sleep %SLEEP%\n'
        t1_ecf += '%include <tail.h>\n'

        self.t1_ecf_path = self.ecf_home + "/test/f4/f5/t1.ecf"
        # print("self.t1_ecf_path",self.t1_ecf_path)
        file = open(self.t1_ecf_path, "w")
        file.write(t1_ecf)
        file.close()

    def tearDown(self):
        unittest.TestCase.tearDown(self)
        do_tear_down()
        os.remove(self.t1_ecf_path)
        shutil.rmtree(self.ecf_home, ignore_errors=True)

    def test_repeat0(self):

        def create_family_f4():
            return Family("f4",
                          Edit(SLEEP=2),
                          RepeatString("NAME", ["a", "b", "c", "d", "e", "f"]),
                          Family("f5",
                                 RepeatInteger("VALUE", 1, 10),
                                 Task("t1",
                                      RepeatDate("DATE", 20101230, 20110105),
                                      Label("info", ""),
                                      Label("date", ""))))

        defs = Defs(
            Suite("test",
                  Edit(ECF_HOME=self.ecf_home, ECF_INCLUDE=self.ecf_includes),
                  create_family_f4()))
        # print(defs)

        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)

        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date,Label, \
                   RepeatString,RepeatInteger,RepeatDate
        
def create_family_f4():
    return Family("f4",
                Edit(SLEEP=2), 
                RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ]),
                Family("f5",
                    RepeatInteger("VALUE", 1, 10),
                    Task("t1",
                        RepeatDate("DATE", 20101230, 20110105),
                        Label("info",""),
                        Label("date",""))))
 
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f4()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
assert len(defs.check()) == 0,defs.check() 

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)

    def test_repeat(self):

        # print("Creating suite definition")
        defs = Defs(
            Suite("test",
                  Edit(ECF_HOME=self.ecf_home, ECF_INCLUDE=self.ecf_includes),
                  Family("f4",
                         Edit(SLEEP=2),
                         RepeatString("NAME", ["a", "b", "c", "d", "e", "f"]),
                         Family("f5",
                                RepeatInteger("VALUE", 1, 10),
                                Task("t1",
                                     RepeatDate("DATE", 20101230, 20110105),
                                     Label("info", ""),
                                     Label("date", ""))))))
        # print(defs)

        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)

        defs.save_as_defs(test_def_file())

    def test_repeat3(self):

        defs = Defs().add(Suite("test"))
        defs.test += [{"ECF_INCLUDE": self.ecf_includes, "ECF_HOME": self.ecf_home},
                      Family("f4")]
        defs.test.f4 += [Edit(SLEEP=2),
                         RepeatString("NAME", ["a", "b", "c", "d", "e", "f"]),
                         Family("f5")]
        defs.test.f4.f5 += [RepeatInteger("VALUE", 1, 10),
                            Task("t1")]
        defs.test.f4.f5.t1 += [RepeatDate("DATE", 20101230, 20110105),
                               Label("info", ""),
                               Label("date", "")]
        # print(defs)
        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)

        defs.save_as_defs(test_def_file())


class TestLimit(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f5():
            f5 = Family("f5")
            f5.add_inlimit("l1")
            f5.add_variable("SLEEP", 20)
            for i in range(1, 10):
                f5.add_task("t" + str(i))
            return f5

        defs = Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_HOME", os.path.join(os.getenv("HOME"), "course"))
        suite.add_variable("ECF_INCLUDE", os.path.join(os.getenv("HOME"), "course"))

        suite.add_limit("l1", 2)
        suite.add_family(create_family_f5())

        defs.save_as_defs(test_def_file())

        self.defs = defs

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date,Label, \
                   RepeatString,RepeatInteger,RepeatDate,InLimit,Limit
         
def create_family_f5() :
    return Family("f5",
            InLimit("l1"),
            Edit(SLEEP=20),
            [ Task('t{0}'.format(i)) for i in range(1,10) ] )
     
#xx print("Creating suite definition")  
home = os.path.join(os.getenv("HOME"),"course")
defs = Defs(
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            Limit("l1",2),
            create_family_f5()))
#xx print(defs)
 
#xx print("Checking job creation: .ecf -> .job0") 
#print(defs.check_job_creation())
 
#xx print("Checking trigger expressions")
assert len(defs.check()) == 0,defs.check()
 
#xx print("Saving definition to file 'test.def'")
defs.save_as_defs(test_def_file()) 
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def tearDown(self):
        do_tear_down()


class TestLateAttribute(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_family_f6():
            f6 = Family("f6")
            f6.add_variable("SLEEP", 120)
            t1 = f6.add_task("t1")
            late = Late()
            late.complete(0, 1, True)  # hour,minute,relative,    set late flag if task take longer than a minute
            t1.add_late(late)
            return f6

        defs = Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_HOME", os.path.join(os.getenv("HOME"), "course"))
        suite.add_variable("ECF_INCLUDE", os.path.join(os.getenv("HOME"), "course"))
        suite.add_family(create_family_f6())
        # print(defs)

        assert len(defs.check()) == 0, defs.check()

        defs.save_as_defs(test_def_file())

        self.defs = defs

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date,Label, \
                   RepeatString,RepeatInteger,RepeatDate,InLimit,Limit,Late
        
def create_family_f6():
    # set late flag if task t1 takes longer than a minute
    return Family("f6",
                Edit(SLEEP=120),
                Task("t1",
                    Late(complete='+00:01')))
     
#xx print("Creating suite definition")   
home = os.path.join(os.getenv("HOME"),"course")
defs = Defs( 
        Suite("test",
            Edit(ECF_HOME=home),Edit(ECF_INCLUDE=home),
            create_family_f6()))
#xx print(defs) 

#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
assert len(defs.check()) == 0,defs.check() 

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def tearDown(self):
        do_tear_down()


class TestPythonScripting(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_suite(name):
            suite = Suite(name)
            for i in range(1, 7):
                fam = suite.add_family("f" + str(i))
                for t in ("a", "b", "c", "d", "e"):
                    fam.add_task(t)
            return suite

        self.defs = Defs(create_suite('s1'))

    def test_me(self):
        def create_suite(name):
            return Suite(name,
                         [Family("f{0}".format(i),
                                 [Task(t) for t in ("a", "b", "c", "d", "e")])
                          for i in range(1, 7)])

        defs = Defs(create_suite('s1'))

        self.assertEqual(self.defs, defs, "defs not equal\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        Ecf.set_debug_equality(False)


class TestPythonScripting2(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        def create_sequential_suite(name):
            suite = Suite(name)
            for i in range(1, 7):
                fam = suite.add_family("f" + str(i))
                if i != 1:
                    fam.add_trigger("f" + str(i - 1) + " == complete")  # or fam.add_family( "f%d == complete" % (i-1) )
                for t in ("a", "b", "c", "d", "e"):
                    fam.add_task(t)
            return suite

        self.defs = Defs(create_sequential_suite('s1'))

    def test_me(self):
        def create_sequential_suite(name):
            suite = Suite(name)
            for i in range(1, 7):
                fam = suite.add_family("f" + str(i))
                if i != 1:
                    fam += Trigger("f" + str(i - 1) + " == complete")  # or fam.add_family( "f%d == complete" % (i-1) )
                for t in ("a", "b", "c", "d", "e"):
                    fam.add_task(t)
            return suite

        defs = Defs(create_sequential_suite('s1'))

        self.assertEqual(self.defs, defs, "defs not equal\n" + str(self.defs) + "\n" + str(defs))

    def tearDown(self):
        Ecf.set_debug_equality(False)


class TestDataAquistionSolution(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        defs = Defs()
        suite = defs.add_suite("data_aquisition")
        suite.add_repeat(RepeatDay(1))
        suite.add_variable("ECF_HOME", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_FILES", os.getenv("HOME") + "/course/data")
        suite.add_variable("SLEEP", "2")
        for city in ("Exeter", "Toulouse", "Offenbach", "Washington", "Tokyo", "Melbourne", "Montreal"):
            fcity = suite.add_family(city)
            fcity.add_task("archive")
            for obs_type in ("observations", "fields", "images"):
                type_fam = fcity.add_family(obs_type)
                if city in ("Exeter", "Toulouse", "Offenbach"): type_fam.add_time("00:00 23:00 01:00")
                if city in ("Washington"):                     type_fam.add_time("00:00 23:00 03:00")
                if city in ("Tokyo"):                          type_fam.add_time("12:00")
                if city in ("Melbourne"):                      type_fam.add_day("monday")
                if city in ("Montreal"):                       type_fam.add_date(1, 0, 0)

                type_fam.add_task("get")
                type_fam.add_task("process").add_trigger("get eq complete")
                type_fam.add_task("store").add_trigger("get eq complete")

        # print(defs)
        self.defs = defs

    def test_me0(self):
        text = """import os
from ecflow import Defs,Suite,Family,Task,Edit,Trigger,Complete,Event,Meter,Time,Day,Date,Label, \
                   RepeatString,RepeatInteger,RepeatDate

home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
        Suite("data_aquisition",
            RepeatDay(1),
            Edit(ECF_HOME=home),
            Edit(ECF_INCLUDE=home),
            Edit(ECF_FILES=home + "/data"),
            Edit(SLEEP=2)))
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
         
        type_fam + Task("get") + Task("process",Trigger("get eq complete")) + Task("store",Trigger("get eq complete"))         
 
#xx print("Checking job creation: .ecf -> .job0")  
#print(defs.check_job_creation())

#xx print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

#xx print("Saving definition to file 'test.def'")
defs.save_as_defs('test.def')
"""
        test_compile(text)
        test_defs = Defs(test_def_file())
        self.assertEqual(test_defs, self.defs, "defs not equal\n" + str(test_defs) + "\n" + str(self.defs))

    def test_me(self):
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + Suite("data_aquisition").add(
            RepeatDay(1),
            Edit(ECF_HOME=home),
            Edit(ECF_INCLUDE=home),
            Edit(ECF_FILES=home + "/data"),
            Edit(SLEEP=2))
        for city in ("Exeter", "Toulouse", "Offenbach", "Washington", "Tokyo", "Melbourne", "Montreal"):
            fcity = defs.data_aquisition.add_family(city)
            fcity += Task("archive")
            for obs_type in ("observations", "fields", "images"):
                type_fam = fcity.add_family(obs_type)
                if city in ("Exeter", "Toulouse", "Offenbach"): type_fam + Time("00:00 23:00 01:00")
                if city in ("Washington"):                     type_fam + Time("00:00 23:00 03:00")
                if city in ("Tokyo"):                          type_fam + Time("12:00")
                if city in ("Melbourne"):                      type_fam + Day("monday")
                if city in ("Montreal"):                       type_fam + Date(1, 0, 0)

                type_fam += [Task("get"), Task("process"), Task("store")]
                type_fam.process += Trigger("get eq complete")
                type_fam.store += Trigger("get eq complete")

        self.assertEqual(self.defs, defs, "defs not equal")

    def tearDown(self):
        Ecf.set_debug_equality(False)


class TestOperationalSolution(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        defs = Defs()
        suite = defs.add_suite("operation_suite")
        suite.add_repeat(RepeatDay(1))
        suite.add_variable("ECF_HOME", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_FILES", os.getenv("HOME") + "/course/oper")

        # Defines the triggers for the first cycle
        cycle_triggers = "1"
        for cycle in ("00", "12"):

            if cycle == "12":
                last_step = 240
            else:
                last_step = 24

            fcycle_fam = suite.add_family(cycle)
            fcycle_fam.add_variable("CYCLE", cycle)
            fcycle_fam.add_variable("LAST_STEP", last_step)

            if cycle_triggers != "1":
                fcycle_fam.add_trigger(cycle_triggers)

            analysis_fam = fcycle_fam.add_family("analysis")
            analysis_fam.add_task("get_observations")
            analysis_fam.add_task("run_analysis").add_trigger("get_observations == complete")
            analysis_fam.add_task("post_processing").add_trigger("run_analysis == complete")

            forecast_fam = fcycle_fam.add_family("forecast")
            forecast_fam.add_trigger("analysis == complete")
            forecast_fam.add_task("get_input_data")
            run_forecast_task = forecast_fam.add_task("run_forecast")
            run_forecast_task.add_trigger("get_input_data == complete")
            run_forecast_task.add_meter("step", 0, last_step, last_step)

            archive_fam = fcycle_fam.add_family("archive")
            fam_analsis = archive_fam.add_family("analysis")
            fam_analsis.add_variable("TYPE", "analysis")
            fam_analsis.add_variable("STEP", "0")
            fam_analsis.add_trigger("../analysis/run_analysis == complete")
            fam_analsis.add_task("save")

            for i in range(6, last_step + 1, 6):
                step_fam = fam_analsis.add_family("step_" + str(i))
                step_fam.add_variable("TYPE", "forecast")
                step_fam.add_variable("STEP", i)
                step_fam.add_trigger("../../forecast/run_forecast:step ge " + str(i))
                step_fam.add_task("save")

            # Defines the triggers for the next cycle
            cycle_triggers = "./" + cycle + " == complete"

            # print(defs)
        self.defs = defs

    def test_sol1(self):
        import os

        home = os.getenv("HOME") + "/course"
        cycle_triggers = None
        last_step = {"12": 240,
                     "00": 24, }

        def cycle_trigger(cycle):
            if cycle == "12": return Trigger("./00 == complete")
            return None

        defs = Defs(
            Suite("operation_suite",
                  RepeatDay(1),
                  Edit(ECF_HOME=home),
                  Edit(ECF_INCLUDE=home),
                  Edit(ECF_FILES=home + "/oper"),
                  [Family(cycle,
                          Edit(CYCLE=cycle),
                          Edit(LAST_STEP=last_step[cycle]),

                          cycle_trigger(cycle),

                          Family("analysis",
                                 Task("get_observations"),
                                 Task("run_analysis", Trigger(["get_observations"])),
                                 Task("post_processing", Trigger(["run_analysis"]))
                                 ),

                          Family("forecast",
                                 Trigger(["analysis"]),
                                 Task("get_input_data"),
                                 Task("run_forecast",
                                      Trigger(["get_input_data"]),
                                      Meter("step", 0, last_step[cycle])),
                                 ),

                          Family("archive",
                                 Family("analysis",
                                        Edit(TYPE="analysis"),
                                        Edit(STEP=0),
                                        Trigger("../analysis/run_analysis == complete"),
                                        Task("save"),
                                        [Family("step_{0}".format(i),
                                                Edit(TYPE="forecast"),
                                                Edit(STEP=i),
                                                Trigger("../../forecast/run_forecast:step ge {0}".format(i)),
                                                Task("save"))
                                         for i in range(6, last_step[cycle] + 1, 6)]
                                        )
                                 )
                          ) for cycle in ("00", "12")]
                  )
        )
        # print(defs)

        self.assertEqual(self.defs, defs, "defs not equal")

    def tearDown(self):
        Ecf.set_debug_equality(False)


class TestBackArchivingSolution(unittest.TestCase):
    def setUp(self):
        Ecf.set_debug_equality(True)

        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs()
        suite = defs.add_suite("back_archiving")
        suite.add_repeat(RepeatDay(1))
        suite.add_variable("ECF_HOME", home)
        suite.add_variable("ECF_INCLUDE", home)
        suite.add_variable("ECF_FILES", home + "/back")
        suite.add_variable("SLEEP", "2")
        suite.add_limit("access", 2)
        for kind in ("analysis", "forecast", "climatology", "observations", "images"):
            find_fam = suite.add_family(kind)
            find_fam.add_repeat(RepeatDate("DATE", 19900101, 19950712))
            find_fam.add_variable("KIND", kind)
            find_fam.add_task("get_old").add_inlimit("access")
            find_fam.add_task("convert").add_trigger("get_old == complete")
            find_fam.add_task("save_new").add_trigger("convert == complete")

        # print(defs)

        self.defs = defs

    def test_sol1(self):
        import os
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs(
            Suite("back_archiving",
                  RepeatDay(1),
                  Edit(ECF_HOME=home),
                  Edit(ECF_INCLUDE=home),
                  Edit(ECF_FILES=home + "/back"),
                  Edit(SLEEP=2),
                  Limit("access", 2),
                  [Family(kind,
                          RepeatDate("DATE", 19900101, 19950712),
                          Edit(KIND=kind),
                          Task("get_old", InLimit("access")),
                          Task("convert", Trigger("get_old == complete")),
                          Task("save_new", Trigger("convert == complete")))
                   for kind in ("analysis", "forecast", "climatology", "observations", "images")]))
        # print(defs)

        self.assertEqual(self.defs, defs, "defs not equal")

    def tearDown(self):
        Ecf.set_debug_equality(False)


if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
