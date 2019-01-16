#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
import ecflow
import unittest 

def setup_test(self):
    self.defs0 = ecflow.Defs()
    self.suite0 = ecflow.Suite("s")
    self.family0 = ecflow.Family("s")
    self.task0 = ecflow.Task("s")

    self.defs1 = ecflow.Defs();        self.defs1.add_suite("s1")
    self.suite1 = ecflow.Suite("s");   self.suite1.add_task("t")
    self.family1 = ecflow.Family("s"); self.family1.add_task("t")

    self.defs3 = ecflow.Defs()
    self.suite3 = ecflow.Suite("s") 
    self.family3 = ecflow.Family("s") 
    [ self.defs3.add_suite("s" + str(i))   for i in [ 1, 2, 3 ] ]
    [ self.suite3.add_family("f" + str(i)) for i in [ 1, 2, 3 ] ]
    [ self.family3.add_task("t" + str(i))  for i in [ 1, 2, 3 ] ]

class TestSizedProtocol(unittest.TestCase):
    def setUp(self):
        setup_test(self)
        
    def test_empty(self):
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
        setup_test(self)

    def test_contains(self):
        self.assertEqual(("s" not in self.defs0), True, "Err in defs container")
        self.assertEqual(("z" not in self.defs3), True, "Err in defs container")
        self.assertEqual(("s1" in self.defs3), True, "Err in defs container")
        self.assertEqual(("s2" in self.defs3), True, "Err in defs container")
        self.assertEqual(("s3" in self.defs3), True, "Err in defs container")

        self.assertEqual(("f" not in self.suite0), True, "Err in suites container")
        self.assertEqual(("z" not in self.suite3), True, "Err in suites container")
        self.assertEqual(("f1" in self.suite3), True, "Err in suites container")
        self.assertEqual(("f2" in self.suite3), True, "Err in suites container")
        self.assertEqual(("f3" in self.suite3), True, "Err in suites container")

        self.assertEqual(("z" not in self.family0), True, "Err in family container")
        self.assertEqual(("t" not in self.family0), True, "Err in family container")
        self.assertEqual(("t1" in self.family3), True, "Err in family container")
        self.assertEqual(("t2" in self.family3), True, "Err in family container")
        self.assertEqual(("t3" in self.family3), True, "Err in family container")
    
class TestIterableProtocol(unittest.TestCase):
    def setUp(self):
        setup_test(self)

    def test_iterator(self):
        self.assertTrue(hasattr(self.defs3,'__iter__') , "defs has no __iter__")
        self.assertTrue(hasattr(self.suite3,'__iter__') , "suite has no __iter__")
        self.assertTrue(hasattr(self.family3,'__iter__') , "family has no __iter__")
        self.assertTrue(hasattr(self.task0,'__iter__') , "task has no __iter__ for aliases")
        self.assertEqual([suite.name() for suite in self.defs3 ] , ["s1","s2","s3"],"Defs iterator protocol not working")
        self.assertEqual([child.name() for child in self.suite3] , ["f1","f2","f3"],"Suites iterator protocol not working")
        self.assertEqual([child.name() for child in self.family3], ["t1","t2","t3"],"Family iterator protocol not working")
        
class TestNodesIteration(unittest.TestCase):
    def setUp(self):
        setup_test(self)

    def test_node_iterator(self):
        self.assertEqual([suite.name() for suite in self.defs3.suites] , ["s1","s2","s3"],"Defs.suites not working")
        self.assertEqual([child.name() for child in self.suite3.nodes] , ["f1","f2","f3"],"Suite.nodes working")
        self.assertEqual([child.name() for child in self.family3.nodes], ["t1","t2","t3"],"Family.nodes not working")
        self.assertEqual([alias.name() for child in self.task0.nodes]   , []              ,"Task.nodes not working")

if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
