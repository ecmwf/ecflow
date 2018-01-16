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
# SCRATCH test for ecflow python api

from ecflow import Alias, AttrType, Autocancel, CheckPt, ChildCmdType, Client, Clock, Cron, DState, Date, Day, Days, \
                  Defs, Ecf, Event, Expression, Family, FamilyVec, File, Flag, FlagType, FlagTypeVec, InLimit, \
                  JobCreationCtrl, Label, Late, Limit, Meter, Node, NodeContainer, NodeVec, PartExpression, PrintStyle, \
                  Repeat, RepeatDate, RepeatDay, RepeatEnumerated, RepeatInteger, RepeatString, SState, State, Style, \
                  Submittable, Suite, SuiteVec, Task, TaskVec, Time, TimeSeries, TimeSlot, Today, UrlCmd, Variable, \
                  VariableList, Verify, WhyCmd, ZombieAttr, ZombieType, ZombieUserActionType, Trigger, Complete, Edit, Defstatus
import unittest 
import sys
import os

class Test_default_defs(unittest.TestCase):
     
    def setUp(self):
        self.file_name = 'Test_default_defs.defs'
        defs = Defs() + Suite('s1')
        defs.save_as_defs( self.file_name)
 
    def test_defs_default_constructor(self):
        defs = Defs()                 
        #PrintStyle.set_style( Style.MIGRATE )  
        #print defs
        self.assertEqual(defs.get_state(), State.unknown, "expected unknow state but found " + str(defs.get_state()))
        self.assertEqual(defs.get_server_state(), SState.RUNNING, "expected default server state tobe running state but found " + str(defs.get_server_state()))
        self.assertEqual(len(list(defs.suites)), 0, "expected no suites but found " + str(len(list(defs.suites))))
 
        defs = Defs(self.file_name)  
        self.assertEqual(defs.get_state(), State.unknown, "expected unknow state but found " + str(defs.get_state()))
        self.assertEqual(defs.get_server_state(), SState.RUNNING, "expected default server state tobe running state but found " + str(defs.get_server_state()))
        self.assertEqual(len(list(defs.suites)), 1, "expected 1 suites but found " + str(len(list(defs.suites))))
         
    def tearDown(self):
        try:  os.remove(self.file_name)
        except: pass
     
class Test_defs_raw_constructor(unittest.TestCase):
     
    def test_defs_raw_constructor_errors(self):
        with self.assertRaises(RuntimeError):
            defs = Defs("file_name.def",Suite('s1'))                
        with self.assertRaises(RuntimeError):
            defs = Defs("file_name.def",Edit(a='b'))                
        with self.assertRaises(RuntimeError):
            defs = Defs("file_name.def",{'a':'b'})                
        with self.assertRaises(RuntimeError):
            defs = Defs("file_name.def",Variable('a','b'))                
        with self.assertRaises(RuntimeError):
            defs = Defs("file_name.def",[Variable('a','b')])                
       
    def test_defs_raw_constructor_suite_creation(self):
        defs = Defs(Suite('s1'))                
        self.assertEqual(len(list(defs.suites)), 1, "expected 1 suites but found " + str(len(list(defs.suites))))

        defs = Defs(Suite('s1'),Suite('s2'),Suite('s3'))                
        self.assertEqual(len(list(defs.suites)), 3, "expected 3 suites but found " + str(len(list(defs.suites))))

        defs = Defs([ Suite('s{0}'.format(i)) for i in range(1,5) ])                
        self.assertEqual(len(list(defs.suites)), 4, "expected 4 suites but found " + str(len(list(defs.suites))))

    def test_defs_raw_constructor_variables(self):
        defs = Defs(Edit(a='b'))                
        self.assertEqual(len(list(defs.user_variables)), 1, "expected 1 user variable but found " + str(len(list(defs.user_variables))))

        defs = Defs(Variable('a','b'))                
        self.assertEqual(len(list(defs.user_variables)), 1, "expected 1 user variable but found " + str(len(list(defs.user_variables))))

        defs = Defs({'a':'a','b':'b'})                
        self.assertEqual(len(list(defs.user_variables)), 2, "expected 2 user variable but found " + str(len(list(defs.user_variables))))

        defs = Defs(a='a',b='b')  # uses kwy word arguments          
        self.assertEqual(len(list(defs.user_variables)), 2, "expected 2 user variable but found " + str(len(list(defs.user_variables))))
 
        defs = Defs(Edit(a='b'),Variable('b','b'),{'c':'a','d':'b'}, e='a',f='b')       
        self.assertEqual(len(list(defs.user_variables)), 6, "expected 6 user variable but found " + str(len(list(defs.user_variables))))

    def test_defs_raw_constructor(self):
        defs = Defs(Suite('s1'),Edit(a='b'))                
        self.assertEqual(len(list(defs.user_variables)), 1, "expected 1 user variable but found " + str(len(list(defs.user_variables))))
        self.assertEqual(len(list(defs.suites)), 1, "expected 1 suites but found " + str(len(list(defs.suites))))

        defs = Defs(Suite('s1'),Suite('s2'),Suite('s3'),Variable('a','b'))                
        self.assertEqual(len(list(defs.user_variables)), 1, "expected 1 user variable but found " + str(len(list(defs.user_variables))))
        self.assertEqual(len(list(defs.suites)), 3, "expected 3 suites but found " + str(len(list(defs.suites))))

        defs = Defs([ Suite('s{0}'.format(i)) for i in range(1,5) ],{'a':'a','b':'b'})                
        self.assertEqual(len(list(defs.user_variables)), 2, "expected 2 user variable but found " + str(len(list(defs.user_variables))))
        self.assertEqual(len(list(defs.suites)), 4, "expected 4 suites but found " + str(len(list(defs.suites))))

        defs = Defs(Suite('s1'),Suite('s2'),Suite('s3'),a='a',b='b')  # uses kwy word arguments          
        self.assertEqual(len(list(defs.user_variables)), 2, "expected 2 user variable but found " + str(len(list(defs.user_variables))))
        self.assertEqual(len(list(defs.suites)), 3, "expected 3 suites but found " + str(len(list(defs.suites))))

        defs = Defs(Suite('s1'),Edit(a='b'),Variable('b','b'),{'c':'a','d':'b'}, e='a',f='b')       
        self.assertEqual(len(list(defs.user_variables)), 6, "expected 6 user variable but found " + str(len(list(defs.user_variables))))
        self.assertEqual(len(list(defs.suites)), 1, "expected 1 suites but found " + str(len(list(defs.suites))))

    def test_defs_constructor(self):
        defs = Defs(Suite('s1'),Edit(a='b'))  
        defs2 = Defs() + Suite('s1') + Edit(a='b')  
        defs3 = Defs()
        defs3 += Suite('s1')
        defs3 += Edit(a='b')
        self.assertEqual(defs,defs2,"defs not equal\n" + str(defs) + "\n\n" + str(defs2))    
        self.assertEqual(defs,defs3,"defs not equal\n" + str(defs) + "\n\n" + str(defs3))    
             
        defs = Defs(Suite('s1'),Suite('s2'),Suite('s3'),Variable('a','b'))  
        defs2 = Defs() + Suite('s1') + Suite('s2') + Suite('s3') + Variable('a','b')   
        defs3 = Defs().add(Suite('s1'),Suite('s2'),Suite('s3'),Variable('a','b') )           
        self.assertEqual(defs,defs2,"defs not equal\n" + str(defs) + "\n\n" + str(defs2))    
        self.assertEqual(defs,defs3,"defs not equal\n" + str(defs) + "\n\n" + str(defs3))    
 
        defs = Defs([ Suite('s{0}'.format(i)) for i in range(1,5) ],{'a':'a'})                
        defs2 = Defs() + [ Suite('s{0}'.format(i)) for i in range(1,5) ] +  {'a':'a'}
        defs3 = Defs()
        for i in range(1,5):
            defs3.add_suite('s{0}'.format(i))   
        defs3.add_variable('a','a')        
        self.assertEqual(defs,defs2,"defs not equal\n" + str(defs) + "\n\n" + str(defs2))    
        self.assertEqual(defs,defs3,"defs not equal\n" + str(defs) + "\n\n" + str(defs3))    
 
        defs = Defs(Suite('s1'),Edit(a='a'),Variable('b','b'),{'c':'c'},{'d':'d'} )       
        defs2 = Defs().add(Suite('s1'),Edit(a='a'),Variable('b','b'),{'c':'c'},{'d':'d'})
        defs3 = Defs()
        defs3.add_suite('s1')   
        defs3.add_variable('a','a')        
        defs3.add_variable('b','b')        
        defs3.add_variable('c','c')        
        defs3.add_variable('d','d')        
        self.assertEqual(defs,defs2,"defs not equal\n" + str(defs) + "\n\n" + str(defs2))    
        self.assertEqual(defs,defs3,"defs not equal\n" + str(defs) + "\n\n" + str(defs3))    


if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
