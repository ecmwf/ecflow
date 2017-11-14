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
            
        with Defs() as self.defs4:  
            self.defs4 += [ Suite("test").add(
                Edit(ECF_HOME=home),
                Task("t1")) ]   
    
    def test_defs_equal(self):
        self.assertEqual(self.defs, self.defs2, "defs not the same")
        self.assertEqual(self.defs, self.defs3, "defs not the same")
        self.assertEqual(self.defs, self.defs4, "defs not the same")
        
class Test1(unittest.TestCase):
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        def generate():
            # print "Creating suite definition"  
            return Defs().add(
                Suite("test").add(
                    Edit(ECF_HOME= os.path.join(os.getenv("HOME"),  "course")),
                    Task("t1")))
 
        defs = generate()      
        print( defs )
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
        try:
            print "Load the in memory definition(defs) into the server"
            #Client("localhost@%s" % os.getenv("ECF_PORT")).load(defs)
        except RuntimeError as e: print "Failed:", e
        
    def tearDown(self):
        os.remove("test.def")

class TestFamilies(unittest.TestCase):
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
        
        print "Creating suite definition" 
        home = os.path.join(os.getenv("HOME"),  "course") 
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                Family("f1").add(
                    Task("t1"),
                    Task("t2"))))
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")

    def tearDown(self):
        os.remove("test.def")

    
class TestVariables(unittest.TestCase):
    def test_me(self):
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
 
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")

    def test_me2(self):
        #!/usr/bin/env python2.7
        import os
        home = os.path.join(os.getenv("HOME"), "course")
 
        with Defs() as defs: 
            defs += [ Suite("test").add(
                      Edit(ECF_INCLUDE=home,ECF_HOME=home)) ]
            defs.test += [ Family("f1" ).add(
                                Task("t1").add(Edit(SLEEP= 20)),
                                Task("t2").add(Edit(SLEEP= 20))) ]                          
 
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
        
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
            
        print "Creating suite definition"
        defs = Defs().add(Suite("test").add(
                            Edit(ECF_INCLUDE=home,ECF_HOME=home),
                            create_family_f1() ))

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
  
        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
        
        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            return Family("f1").add(
                Edit(SLEEP=20),
                Task("t1"),
                Task("t2"))
            
        print "Creating suite definition"
        with Defs() as defs:
            defs += [ Suite("test").add(create_family_f1()) ]
            defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home) ]

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
        
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
                Task("t2").add(Trigger("t1 eq complete")))
            
        print "Creating suite definition"
        defs = Defs().add(Suite("test").add(
                            Edit(ECF_INCLUDE=home,ECF_HOME=home),
                            create_family_f1()))
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
 
        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os
        
        home = os.path.join(os.getenv("HOME"), "course")
        with Suite("test") as suite:
            suite += [ Edit(ECF_INCLUDE=home,ECF_HOME=home) ]
            suite += [ Family("f1") ]
            suite.f1 += [ Edit(SLEEP=20) ]
            suite.f1 += [ Task("t1")]
            suite.f1 += [ Task("t2")]
            suite.f1.t2 += [ Trigger("t1 eq complete") ]
            
        print "Creating suite definition"
        defs = Defs().add( suite )

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
        
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
            
        print "Creating suite definition"
        defs = Defs().add(
            Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def") 
 
        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1")
            f1 += [ Edit(SLEEP=20),
                    Task("t1"),
                    Task("t2").add( Trigger("t1 == complete"), Event("a"), Event("b")),
                    Task("t3").add( Trigger("t2:a")),
                    Task("t4").add( Trigger("t2:b")) ]
            return f1
        
        print "Creating suite definition"
        defs = Defs().add(Suite("test"))
        defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home), create_family_f1()]
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   
        
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
                Task("t2").add(Trigger("t1 eq complete"),
                               Event("a"),
                               Event("b")),
                Task("t3").add(Trigger("t2:a")),
                Task("t4").add(Trigger("t2 eq complete"), 
                               Complete("t2:b")  ))
       
        print "Creating suite definition"  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   

        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1")
            f1 += [ Edit(SLEEP=20) ]
            for i in range(1,5):
                f1 += [ Task("t{}".format(i)) ]
            f1.t2 += [ Trigger("t1 eq complete"), Event("a"), Event("b") ]
            f1.t3 += [ Trigger("t2:a") ]
            f1.t4 += [ Trigger("t2 eq complete"),Complete("t2:b") ]
            return f1
       
        print "Creating suite definition"  
        defs = Defs().add(Suite("test"))
        defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home), create_family_f1() ]     
        
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   
        
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
                        Task("t2").add(Trigger("t1 eq complete"),
                                       Event("a"),
                                       Event("b")),
                        Task("t3").add(Trigger("t2:a")),
                        Task("t4").add(Trigger("t2 eq complete"),
                                       Complete("t2:b")),
                        Task("t5").add(Trigger("t1:progress ge 30")),
                        Task("t6").add(Trigger("t1:progress ge 60")),
                        Task("t7").add(Trigger("t1:progress ge 90")))  
       
        print "Creating suite definition"  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f1() ))

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   

        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f1():
            f1 = Family("f1").add(Edit(SLEEP=20))
            for i in range(1,8):
                f1 += [ Task("t{}".format(i)) ]
            f1.t1 += [ Meter("progress", 1, 100, 90) ]
            f1.t2 += [ Trigger("t1 eq complete"), Event("a"), Event("b") ]
            f1.t3 += [ Trigger("t2:a") ]
            f1.t4 += [ Trigger("t2 eq complete"),Complete("t2:b") ]
            f1.t5 += [ Trigger("t1:progress ge 30")  ]
            f1.t6 += [ Trigger("t1:progress ge 60")  ]
            f1.t7 += [ Trigger("t1:progress ge 90")  ]
            return f1
       
        print "Creating suite definition"  
        defs = Defs().add(Suite("test"))
        defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home), create_family_f1() ]     
        
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   
        
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

        print "Creating suite definition"  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f2()))

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   

        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        home = os.path.join(os.getenv("HOME"), "course")
        def create_family_f2():
            f1 = Family("f2").add(Edit(SLEEP=20))
            for i in range(1,6):
                f1 += [ Task("t{}".format(i))]
            f1.t1 += [ Time("00:30 23:30 00:30") ] # start(hh:mm) end(hh:mm) increment(hh:mm)
            f1.t2 += [ Day( "sunday" ) ]
            f1.t3 += [ Date("1.*.*"),    
                       Time("12:00")]   
            f1.t4 += [ Time("+00:02") ]  
            f1.t5 += [ Time("00:02") ]   
            return f1

        print "Creating suite definition"  
        defs = Defs().add(Suite("test"))
        defs.test += [ Edit(ECF_INCLUDE=home,ECF_HOME=home), create_family_f2() ]     
        
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   
        
        self.assertEqual(self.defs,defs,"defs not equal")       
        
    def tearDown(self):
        os.remove("test.def")
        
class TestIndentation(unittest.TestCase):
    def setUp(self):
        #!/usr/bin/env python2.7
        import os
        print "Creating suite definition" 
        home = os.path.join(os.getenv("HOME"), "course")
        defs = Defs().add(# Stream like definition
                    Suite("test").add(
                        Edit(ECF_INCLUDE=home,ECF_HOME=home),
                        Family("f1").add(
                            Edit(SLEEP=20),
                            Task("t1").add(Meter("progress", 1, 100, 90)),
                            Task("t2").add( Trigger("t1 eq complete"), Event("a"),Event("b")),
                            Task("t3").add(Trigger("t2:a")),
                            Task("t4").add(Trigger("t2 eq complete"), Complete("t2:b")),
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
             
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   

        self.defs = defs;
        
    def test_me(self):
        #!/usr/bin/env python2.7
        import os

        print "Creating suite definition"  
        home = os.path.join(os.getenv("HOME"), "course")
        with Defs() as defs:
            with defs.add_suite("test") as suite:
                suite += [ Edit(ECF_INCLUDE=home,ECF_HOME=home) ]
                with suite.add_family("f1") as f1:
                    for i in range(1,8):
                        f1 += [ Task("t{}".format(i))]
                    f1 += [ Edit(SLEEP=20) ]
                    f1.t1 += [ Meter("progress", 1, 100, 90) ]
                    f1.t2 += [ Trigger("t1 eq complete"), Event("a"), Event("b") ]
                    f1.t3 += [ Trigger("t2:a") ]
                    f1.t4 += [ Trigger("t2 eq complete"), Complete("t2:b") ]
                    f1.t5 += [ Trigger("t1:progress ge 30") ]
                    f1.t6 += [ Trigger("t1:progress ge 60") ]
                    f1.t7 += [ Trigger("t1:progress ge 90") ]
                with suite.add_family("f2") as f2:
                    f2 += [ Edit(SLEEP=20),
                            Task("t1").add(Time( "00:30 23:30 00:30" )),
                            Task("t2").add(Day( "sunday" )),
                            Task("t3").add(Date("1.*.*"), Time("12:00")),
                            Task("t4").add(Time("+00:02")),
                            Task("t5").add(Time("00:02")) ]
        
        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")   
        
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

        print "Creating suite definition"  
        defs = Defs().add(Suite("test").add(
                Edit(ECF_INCLUDE=home,ECF_HOME=home),
                create_family_f3()))

        print defs
        print "Checking job creation: .ecf -> .job0"  
        #print defs.check_job_creation()
        print "Saving definition to file 'test.def'"
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
    
        def create_family_f4():
            return Family("f4").add(
                        Edit(SLEEP=2), 
                        RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ]),
                        Family("f5").add( 
                            RepeatInteger("VALUE", 1, 10),
                            Task("t1").add( 
                                RepeatDate("DATE", 20101230, 20110105),
                                Label("info", ""),
                                Label("data",""))))

        print "Creating suite definition" 
        defs = Defs().add(
                    Suite("test").add(
                        Edit(ECF_INCLUDE=self.ecf_includes,ECF_HOME=self.ecf_home),
                        create_family_f4()))
        print defs

        print "Checking job creation: .ecf -> .job0"   
        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succeed " + result)

        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
    
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
