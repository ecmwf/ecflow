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
import sys
import os
import copy
import unittest # for assertItemsEqual

class TestSizedProtocol(unittest.TestCase):
    def setUp(self):
        self.defs0 = ecflow.Defs()
        self.suite0 = ecflow.Suite("s")
        self.family0 = ecflow.Family("s")
        self.task0 = ecflow.Task("s")

        self.defs1 = ecflow.Defs();        self.defs1.add_suite("s1")
        self.suite1 = ecflow.Suite("s");   self.suite1.add_task("t")
        self.family1 = ecflow.Family("s"); self.family1.add_task("t")

        self.defs3 = ecflow.Defs()
        for i in  [ 1, 2, 3 ]:
            self.defs3.add_suite("s" + str(i))   
        self.suite3 = ecflow.Suite("s") 
        for i in [ 1, 2, 3 ] :
            self.suite3.add_family("f" + str(i))  
        self.family3 = ecflow.Family("s") 
        for i in  [ 1, 2, 3 ]:
            self.family3.add_task("t" + str(i))   
            
    def test_empty_defs(self):
        self.assertEqual(len(self.defs0), 0, "Expected empty defs to be of size zero")
        self.assertEqual(len(self.suite0), 0, "Expected empty suite to be of size zero")
        self.assertEqual(len(self.family0), 0, "Expected empty family to be of size zero")
        self.assertEqual(len(self.task0), 0, "Expected empty task to be of size zero")

    def test_size1(self):
        self.assertEqual(len(self.defs1), 1, "Expected defs with one suite")
        self.assertEqual(len(self.suite1), 1, "Expected suite with one child")
        self.assertEqual(len(self.family1), 1, "Expected family with one child")

    def test_size3(self):
        self.assertEqual(len(self.defs3), 3, "Expected defs with 3 suite but found " + str(len(self.defs3)))
        self.assertEqual(len(self.suite3), 3, "Expected suite with 3 child but found " + str(len(self.suite3)))
        self.assertEqual(len(self.family3), 3, "Expected family with 3 child but found " + str(len(self.family3)))

class TestContainerProtocol(unittest.TestCase):
    def setUp(self):
        self.defs0 = ecflow.Defs()
        self.suite0 = ecflow.Suite("s")
        self.family0 = ecflow.Family("s")
        self.task0 = ecflow.Task("s")

        self.defs3 = ecflow.Defs()
        for i in  [ 1, 2, 3 ]:
            self.defs3.add_suite("s" + str(i))   
        self.suite3 = ecflow.Suite("s") 
        for i in [ 1, 2, 3 ] :
            self.suite3.add_family("f" + str(i))  
        self.family3 = ecflow.Family("s") 
        for i in  [ 1, 2, 3 ]:
            self.family3.add_task("t" + str(i))   
            
    def test_contains(self):
        self.assertEqual(("s" in self.defs0), False, "Err in defs container")
        self.assertEqual(("s1" in self.defs3), True, "Err in defs container")
        self.assertEqual(("s2" in self.defs3), True, "Err in defs container")
        self.assertEqual(("s3" in self.defs3), True, "Err in defs container")

        self.assertEqual(("f" in self.suite0), False, "Err in suites container")
        self.assertEqual(("f1" in self.suite3), True, "Err in suites container")
        self.assertEqual(("f2" in self.suite3), True, "Err in suites container")
        self.assertEqual(("f3" in self.suite3), True, "Err in suites container")

        self.assertEqual(("t" in self.family0), False, "Err in suites container")
        self.assertEqual(("t1" in self.family3), True, "Err in suites container")
        self.assertEqual(("t2" in self.family3), True, "Err in suites container")
        self.assertEqual(("t3" in self.family3), True, "Err in suites container")
    
        
if __name__ == "__main__":
    print("####################################################################")
    print("Running ecflow version " + ecflow.Client().version()  + " debug build(" + str(ecflow.debug_build()) +")")
    print("PYTHONPATH: " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    print("sys.path:   " + str(sys.path))
    print("####################################################################")
    unittest.main()
    print("All Tests pass")
    
