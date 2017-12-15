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
import ecflow_test_util as Test
import unittest 
import shutil   # used to remove directory tree
import os

class TestNewSuite(unittest.TestCase):
    def setUp(self):
        home = os.path.join(os.getenv("HOME"),"course")
        self.defs = Defs()
        suite = self.defs.add_suite("test")
        suite.add_variable("ECF_HOME", home)
        suite.add_task("t1")
         
        with Defs() as self.defs2: 
            with self.defs2.add_suite("test") as suite:
                suite.add_variable("ECF_HOME", home)
                suite.add_task("t1") 
         
        with Defs() as self.defs3:  
            self.defs3.add(Suite("test").add(
                Edit(ECF_HOME=home),
                Task("t1")))
             
        self.defs4 = Defs() + (Suite("test") + Edit(ECF_HOME=home))
        self.defs4.test += Task("t1")
        
        self.defs5 = Defs().add(
                Suite("test").add(
                    Edit(ECF_HOME= os.path.join(os.getenv("HOME"),  "course")),
                    Task("t1")))
        
     
    def test_defs_equal(self):
        self.assertEqual(self.defs, self.defs2, "defs not the same")
        self.assertEqual(self.defs, self.defs3, "defs not the same")
        self.assertEqual(self.defs, self.defs4, "defs not the same")
        self.assertEqual(self.defs, self.defs5, "defs not the same")
         
 
class TestFamilies(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
         
        print("Creating suite definition") 
        home = os.path.join(os.getenv("HOME"),  "course") 
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                Family("f1").add(
                    Task("t1"),
                    Task("t2"))))
        print(defs)
        print("Checking job creation: .ecf -> .job0")
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
         
        self.defs = defs
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
         
        print("Creating suite definition")
        home = os.path.join(os.getenv("HOME"),  "course") 
        defs = Defs() + (Suite("test") + Edit(ECF_INCLUDE=home,ECF_HOME=home))
        defs.test += Family("f1") + [ Task("t{0}".format(i)) for i in range(1,3) ]
        print(defs)
        print("Checking job creation: .ecf -> .job0")
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")        
 
    def tearDown(self):
        os.remove("test.def")
 
     
class TestVariables(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
        home = os.path.join(os.getenv("HOME"), "course")
  
        def create_family_f1():
            return Family("f1" ).add(
                Task("t1").add(Edit(SLEEP= 20)),
                Task("t2").add(Edit(SLEEP= 20)))
  
        defs = Defs().add( Suite("test").add(
                            Edit(ECF_INCLUDE=home,ECF_HOME=home),
                            create_family_f1()))
  
        print(defs)
        print("Checking job creation: .ecf -> .job0")
        #print(defs.check_job_creation())
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
 
        self.defs = defs
        
    def test_me2(self):
        #!/usr/bin/env python2.7
        import os
        home = os.path.join(os.getenv("HOME"), "course")
  
        with Defs() as defs: 
            defs += Suite("test").add(
                      Edit(ECF_INCLUDE=home,ECF_HOME=home)) 
            defs.test += Family("f1").add(
                                Task("t1").add(Edit(SLEEP= 20)),
                                Task("t2").add(Edit(SLEEP= 20)))                          
  
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       

    def test_me3(self):
        #!/usr/bin/env python2.7
        import os
        home = os.path.join(os.getenv("HOME"), "course")
  
        defs = Defs() + (Suite("test") + Edit(ECF_INCLUDE=home,ECF_HOME=home)) 
        defs.test += Family("f1") + (Task("t1") + Edit(SLEEP=20)) + (Task("t2") + Edit(SLEEP=20))                     
  
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       

    def tearDown(self):
        os.remove("test.def")
         
class TestVariableInheritance(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
         
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2"))
             
        print("Creating suite definition")
        defs = Defs().add(Suite("test").add(
                            Edit(ECF_INCLUDE=home,ECF_HOME=home),
                            create_family_f1() ))
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
   
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
         
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1") + Edit(SLEEP=20) + Task("t1") + Task("t2")
             
        print("Creating suite definition")
        defs = Defs() + (Suite("test") + create_family_f1()) 
        defs.test += Edit(ECF_INCLUDE=home,ECF_HOME=home) 
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestTriggers(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
         
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2").add(Trigger("t1 == complete")))
             
        print("Creating suite definition")
        defs = Defs().add(Suite("test").add(
                            Edit(ECF_INCLUDE=home,ECF_HOME=home),
                            create_family_f1()))
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
  
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
         
        home = os.path.join(os.getenv("HOME"), "course")
        with Suite("test") as suite:
            suite += Edit(ECF_INCLUDE=home,ECF_HOME=home) 
            suite += Family("f1") + Edit(SLEEP=20) + Task("t1") + Task("t2")
            suite.f1.t2 += Trigger(["t1"]) 
             
        print("Creating suite definition")
        defs = Defs().add( suite )
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
         
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestEvents(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
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
             
        print("Creating suite definition")
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def") 
  
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1") + [ Edit(SLEEP=20),
                    Task("t1"),
                    Task("t2") + Trigger(["t1"]) + Event("a") + Event("b"),
                    Task("t3") + Trigger("t2:a"),
                    Task("t4") + Trigger("t2:b") ]
            return f1
         
        print("Creating suite definition")
        defs = Defs() + Suite("test")
        defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home), create_family_f1()]
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestComplete(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP= 20),
                Task("t1"),
                Task("t2").add(Trigger("t1 == complete"),
                               Event("a"), Event("b")),
                Task("t3").add(Trigger("t2:a")),
                Task("t4").add(Trigger("t2 == complete"), 
                               Complete("t2:b")  ))
        
        print("Creating suite definition")  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
 
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1") + Edit(SLEEP=20) 
            f1 += [ Task("t{0}".format(i)) for i in range(1,5) ]
            f1.t2 += [ Trigger(["t1"]), Event("a"), Event("b") ]
            f1.t3 += Trigger("t2:a") 
            f1.t4 += [ Trigger(["t2"]),Complete("t2:b") ]
            return f1
        
        print("Creating suite definition")  
        defs = Defs() + (Suite("test") + Edit(ECF_INCLUDE=home,ECF_HOME=home))
        defs.test += create_family_f1()      
         
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestMeter(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1").add(
                        Edit(SLEEP= 20),
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
        
        print("Creating suite definition")  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
 
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1") + Edit(SLEEP=20) 
            f1 += [ Task("t{0}".format(i)) for i in range(1,8)]
            f1.t1 += Meter("progress", 1, 100, 90) 
            f1.t2 += [ Trigger(["t1"]), Event("a"), Event("b") ]
            f1.t3 += Trigger("t2:a")
            f1.t4 += [ Trigger(["t2"]),Complete("t2:b") ]
            f1.t5 += Trigger("t1:progress ge 30")  
            f1.t6 += Trigger("t1:progress ge 60")  
            f1.t7 += Trigger("t1:progress ge 90")  
            return f1
        
        print("Creating suite definition")  
        defs = Defs() + (Suite("test") + Edit(ECF_INCLUDE=home,ECF_HOME=home))
        defs.test += create_family_f1()   
         
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestTime(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f2():
            return Family("f2").add(
                Edit(SLEEP=20),
                Task("t1").add( Time("00:30 23:30 00:30")),  
                Task("t2").add( Day( "sunday" )),
                Task("t3").add( Date("1.*.*"),   
                                Time("12:00")),  
                Task("t4").add( Time("+00:02")), 
                Task("t5").add( Time("00:02")))  
 
        print("Creating suite definition")  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f2()))
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
 
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f2():
            f1 = Family("f2") + Edit(SLEEP=20)
            f1 += [ Task("t{0}".format(i)) for i in range(1,6) ]
            f1.t1 += Time("00:30 23:30 00:30")  # start(hh:mm) end(hh:mm) increment(hh:mm)
            f1.t2 += Day( "sunday" ) 
            f1.t3 += [ Date("1.*.*"),    
                       Time("12:00")]   
            f1.t4 += Time("+00:02")  
            f1.t5 += Time("00:02")    
            return f1
 
        print("Creating suite definition")  
        defs = Defs() + ( Suite("test") + Edit(ECF_INCLUDE=home,ECF_HOME=home))
        defs.test += create_family_f2()    
         
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
class TestIndentation(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
        print("Creating suite definition") 
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(# Stream like definition
                    Suite("test").add(
                        Edit(ECF_INCLUDE=home,ECF_HOME=home),
                        Family("f1").add(
                            Edit(SLEEP=20),
                            Task("t1").add(Meter("progress", 1, 100, 90)),
                            Task("t2").add( Trigger("t1 == complete"), Event("a"),Event("b")),
                            Task("t3").add(Trigger("t2:a")),
                            Task("t4").add(Trigger("t2 == complete"), Complete("t2:b")),
                            Task("t5").add(Trigger("t1:progress ge 30")),
                            Task("t6").add(Trigger("t1:progress ge 60")),
                            Task("t7").add(Trigger("t1:progress ge 90")),),
                        Family("f2").add(
                            Edit(SLEEP=20),
                            Task("t1").add(Time( "00:30 23:30 00:30" )),
                            Task("t2").add(Day( "sunday" )),
                            Task("t3").add(Date("1.*.*"), Time("12:00")),
                            Task("t4").add(Time("+00:02")),
                            Task("t5").add(Time("00:02")))))
              
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
 
        self.defs = defs;
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        print("Creating suite definition")  
        home = os.path.join(os.getenv("HOME"), "course")
        with Defs() as defs:
            with defs.add_suite("test") as suite:
                suite += Edit(ECF_INCLUDE=home,ECF_HOME=home) 
                with suite.add_family("f1") as f1:
                    f1 += [ Task("t{0}".format(i)) for i in range(1,8)]
                    f1 += Edit(SLEEP=20) 
                    f1.t1 += Meter("progress", 1, 100, 90) 
                    f1.t2 += [ Trigger(["t1"]), Event("a"), Event("b") ]
                    f1.t3 += Trigger("t2:a") 
                    f1.t4 += [ Trigger(["t2"]), Complete("t2:b") ]
                    f1.t5 += Trigger("t1:progress ge 30") 
                    f1.t6 += Trigger("t1:progress ge 60") 
                    f1.t7 += Trigger("t1:progress ge 90") 
                with suite.add_family("f2") as f2:
                    f2 += [ Edit(SLEEP=20),[ Task("t{0}".format(i)) for i in range(1,6)] ]
                    f2.t1 += Time( "00:30 23:30 00:30" )
                    f2.t2 += Day( "sunday" ) 
                    f2.t3 += [ Date("1.*.*"), Time("12:00") ]
                    f2.t4 += Time("+00:02") 
                    f2.t5 += Time("00:02") 
         
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")   
         
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)      
        self.assertEqual(self.defs,defs,"defs not equal")       
         
    def tearDown(self):
        os.remove("test.def")
         
         
class TestLabel(unittest.TestCase):
         
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
 
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f3():
            return Family("f3").add(
                        Task("t1").add(
                            Label("info","")))
 
        print("Creating suite definition")  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f3()))
 
        print(defs)
        print("Checking job creation: .ecf -> .job0")  
        #print(defs).check_job_creation()
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")  
         
    def tearDown(self):
        os.remove("test.def")
         
         
class TestRepeat(unittest.TestCase):
    def setUp(self):
        self.ecf_home     = File.build_dir() + "/Pyext/test/data/course"
        self.ecf_includes = File.source_dir() + "/Pyext/test/data/includes"
        print("self.ecf_home ",self.ecf_home )
 
        try:    os.makedirs( self.ecf_home + "/test/f4/f5")
        except: pass
         
        t1_ecf = '%include <head.h>\n'
        t1_ecf += 'ecflow_client --label=info "My name is %NAME% My value is %VALUE% My date is %DATE%\n'
        t1_ecf += 'ecflow_client --label=date "year:%DATE_YYYY% month:%DATE_MM% day of month:%DATE_DD% day of week:%DATE_DOW%"\n'
        t1_ecf += 'sleep %SLEEP%\n'
        t1_ecf += '%include <tail.h>\n'
         
        self.t1_ecf_path = self.ecf_home + "/test/f4/f5/t1.ecf"
        print("self.t1_ecf_path",self.t1_ecf_path)
        file = open(self.t1_ecf_path ,"w") 
        file.write(t1_ecf)
        file.close()
         
      
    def tearDown(self):
        os.remove("test.def")
        os.remove(self.t1_ecf_path)
        shutil.rmtree(self.ecf_home , ignore_errors=True)  
     
    def test_repeat(self):
     
        print("Creating suite definition") 
        defs = Defs().add(
                Suite("test").add(
                    Edit(   ECF_INCLUDE=self.ecf_includes,
                            ECF_HOME=self.ecf_home),
                    Family("f4").add(
                        Edit(SLEEP=2), 
                        RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ]),
                        Family("f5").add( 
                            RepeatInteger("VALUE", 1, 10),
                            Task("t1").add( 
                                RepeatDate("DATE", 20101230, 20110105),
                                Label("info", ""),
                                Label("data",""))))))
        print(defs)
 
        print("Checking job creation: .ecf -> .job0")   
        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)
 
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")
         
    def test_repeat3(self):
     
        print("Creating suite definition") 
        defs = Defs().add( Suite("test") )
        defs.test += [ { "ECF_INCLUDE":self.ecf_includes, "ECF_HOME":self.ecf_home }, 
                        Family("f4") ]
        defs.test.f4 += [   {"SLEEP":2},
                            RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ]),
                            Family("f5") ]
        defs.test.f4.f5 += [ RepeatInteger("VALUE", 1, 10),
                            Task("t1")]
        defs.test.f4.f5.t1 += [ RepeatDate("DATE", 20101230, 20110105),
                                Label("info", ""),
                                Label("data","") ]
        print(defs)
        print("Checking job creation: .ecf -> .job0")   
        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)
 
        print("Saving definition to file 'test.def'")
        defs.save_as_defs("test.def")

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
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
        
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs() + Suite("data_aquisition").add(
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
         
                type_fam += [ Task("get"),Task("process"),Task("store") ]
                type_fam.process += Trigger("get eq complete") 
                type_fam.store += Trigger("get eq complete")     

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")
  
class TestOperationalSolution(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
   
        defs = Defs()
        suite = defs.add_suite("operation_suite")
        suite.add_repeat( RepeatDay(1) )
        suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
        suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_FILES",   os.getenv("HOME") + "/course/oper")
  
        # Defines the triggers for the first cycle
        cycle_triggers = "1"
        for cycle in ( "00" , "12" ):

            if cycle == "12" :
                last_step = 240
            else:
                last_step = 24
 
            fcycle_fam = suite.add_family(cycle)
            fcycle_fam.add_variable("CYCLE", cycle)
            fcycle_fam.add_variable("LAST_STEP", last_step)
      
            if cycle_triggers != "1" :
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
            fam_analsis.add_variable("TYPE","analysis")
            fam_analsis.add_variable("STEP","0")
            fam_analsis.add_trigger("../analysis/run_analysis == complete")
            fam_analsis.add_task("save")
    
            for i in range(6, last_step+1, 6):  
                step_fam = fam_analsis.add_family("step_" + str(i))     
                step_fam.add_variable("TYPE", "forecast")  
                step_fam.add_variable("STEP", i)  
                step_fam.add_trigger("../../forecast/run_forecast:step ge " + str(i))
                step_fam.add_task("save")

            # Defines the triggers for the next cycle
            cycle_triggers = "./" + cycle + " == complete"   
                 
        print(defs)
        self.defs = defs
        
    def test_sol1(self):
        #!/usr/bin/env python2.7
        import os
         
        home = os.getenv("HOME") + "/course"
        cycle_triggers = None
        last_step = { "12": 240,
                      "00": 24, }
         
        def cycle_trigger(cycle):
            if cycle == "12": return Trigger("./00 == complete")
            return None
         
        defs = Defs().add(
            Suite("operation_suite").add(
                RepeatDay(1),
                Edit(ECF_HOME= home),
                Edit(ECF_INCLUDE= home),
                Edit(ECF_FILES= home + "/oper"),
                [ Family(cycle).add(
                    Edit(CYCLE=cycle),
                    Edit(LAST_STEP=last_step[cycle]),
        
                    cycle_trigger(cycle),
                    
                    Family("analysis").add(
                        Task("get_observations"),
                        Task("run_analysis").add(Trigger(["get_observations"])),
                        Task("post_processing").add(Trigger(["run_analysis"])),
                    ),
    
                    Family("forecast").add(
                        Trigger(["analysis"]),
                        Task("get_input_data"),
                        Task("run_forecast").add(
                            Trigger(["get_input_data"]),
                            Meter("step", 0, last_step[cycle])),
                    ),
                                           
                    Family("archive").add(
                        Family("analysis").add(
                            Edit(TYPE="analysis"),
                            Edit(STEP=0),
                            Trigger("../analysis/run_analysis == complete"),
                            Task("save"),
                            [ Family("step_{0}".format(i)).add(
                                Edit(TYPE="forecast"),
                                Edit(STEP=i),
                                Trigger("../../forecast/run_forecast:step ge {0}".format(i)),
                                Task("save"))
                             for i in range(6, last_step[cycle]+1, 6) ]
                        )
                    )
                ) for cycle in ( "00" , "12" ) ] 
            )
        )
        print(defs)
        
        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)
        self.assertEqual(self.defs, defs, "defs not equal")

class TestBackArchivingSolution(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
    
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs()
        suite = defs.add_suite("back_archiving")
        suite.add_repeat( RepeatDay(1) )
        suite.add_variable("ECF_HOME",    home)
        suite.add_variable("ECF_INCLUDE", home)
        suite.add_variable("ECF_FILES",   home + "/back")
        suite.add_variable("SLEEP", "2")
        suite.add_limit("access", 2)
        for kind in ( "analysis", "forecast", "climatology", "observations", "images" ):
            find_fam = suite.add_family(kind)
            find_fam.add_repeat( RepeatDate("DATE", 19900101, 19950712) )
            find_fam.add_variable("KIND", kind)
            find_fam.add_task("get_old").add_inlimit("access")
            find_fam.add_task("convert").add_trigger("get_old == complete")
            find_fam.add_task("save_new").add_trigger("convert == complete")
             
        print(defs)
         
        self.defs = defs
         
    def test_sol1(self):
        #!/usr/bin/env python2.7
        import os
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(Suite("back_archiving").add(
            RepeatDay(1),
            Edit(ECF_HOME=home),
            Edit(ECF_INCLUDE=home),
            Edit(ECF_FILES= home + "/back"),
            Edit(SLEEP=2),
            Limit("access", 2),
            [ Family(kind).add(
                RepeatDate( "DATE", 19900101, 19950712 ),
                Edit(KIND= kind),
                Task("get_old").add(InLimit("access")),
                Task("convert").add(Trigger("get_old == complete")),
                Task("save_new").add(Trigger("convert == complete")))
                for kind in ( "analysis", "forecast", "climatology", "observations", "images" ) ] ))
        print(defs)

        Ecf.set_debug_equality(True)
        equals = (self.defs == defs)
        Ecf.set_debug_equality(False)         
        self.assertEqual(self.defs, defs, "defs not equal")

if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
