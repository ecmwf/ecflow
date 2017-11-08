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
import ecflow
import ecflow_test_util as Test
import unittest 
import shutil   # used to remove directory tree
import os

class TestRepeat(unittest.TestCase):
    def setUp(self):
        self.ecf_home     = ecflow.File.build_dir() + "/Pyext/test/data/course"
        self.ecf_includes = ecflow.File.source_dir() + "/Pyext/test/data/includes"
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
            f4 = ecflow.Family("f4")
            f4.add_variable("SLEEP", 2)
            f4.add_repeat( ecflow.RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ] ) )
   
            f5 = f4.add_family("f5")
            f5.add_repeat( ecflow.RepeatInteger("VALUE", 1, 10) )
   
            t1 = f5.add_task("t1")
            t1.add_repeat( ecflow.RepeatDate("DATE", 20101230, 20110105) )
            t1.add_label("info", "").add_label("data","")
            return f4

        print "Creating suite definition"   
        defs = ecflow.Defs()
        suite = defs.add_suite("test")
        suite.add_variable("ECF_INCLUDE", self.ecf_includes)
        suite.add_variable("ECF_HOME",    self.ecf_home)

        suite.add_family( create_family_f4() )
        print defs

        print "Checking job creation: .ecf -> .job0"   
        result = defs.check_job_creation()
        self.assertEqual(result, "", "expected job creation to succed " + result)

        print "Saving definition to file 'test.def'"
        defs.save_as_defs("test.def")
    
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
