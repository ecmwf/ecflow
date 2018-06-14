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

#  code for testing errors in creation of defs file in python

import os
from ecflow import Day, Date, Meter, Event, Queue, Clock, Variable, Label, Limit, InLimit, \
                   RepeatDate, RepeatEnumerated, RepeatInteger, RepeatString, \
                   Task, Family, Suite, Defs, Client, debug_build

def check_day(day):
    try:    
        Day(day)
        return True
    except RuntimeError: 
        return False

def check_date(day,month,year):
    try:    
        Date(day,month,year)
        return True
    except IndexError: 
        return False
    
def check_date_str(str_date):
    try:    
        Date( str_date)
        return True
    except IndexError: 
        return False
    except  RuntimeError: 
        return False

def check_meter(name,min_meter_value,max_meter_value,color_change):
    try:    
        Meter(name,min_meter_value,max_meter_value,color_change)
        return True
    except IndexError: 
        return False
    except RuntimeError: 
        return False

def check_queue(name,queue_items):
    try:    
        Queue(name, queue_items)
        return True
    except IndexError: 
        return False
    except RuntimeError: 
        return False
    
def check_event_number_and_name(number,name):
    try:    
        Event(number,name)
        return True
    except: 
        return False
    
def check_event(number):
    try:    
        Event(number)
        return True
    except RuntimeError: 
        return False
        
def check_clock(day_of_month,month,year):
    try:    
        Clock(day_of_month,month,year)
        return True
    except IndexError: 
        return False

def check_variable(name,value):
    try:    
        Variable(name,value)
        return True
    except RuntimeError: 
        return False
    
def check_label(name,value):
    try:    
        Label(name,value)
        return True
    except RuntimeError: 
        return False
    
def check_limit(name,int_token):
    try:    
        Limit(name,int_token)
        return True
    except RuntimeError: 
        return False
    except TypeError: 
        return False
    
def check_inlimit(name,path_to_node,int_token):
    try:    
        InLimit(name,path_to_node,int_token)
        return True
    except RuntimeError: 
        return False
    except TypeError: 
        return False
        
def check_repeat_date(name, start, end, step):
    try:    
        RepeatDate(name,start,end,step)
        return True
    except RuntimeError: 
        return False
    
def check_repeat_integer(name, start, end, step):
    try:    
        RepeatInteger(name,start,end,step)
        return True
    except RuntimeError: 
        return False
    except TypeError: 
        return False

def check_repeat_enumerated(name, list_of_strings):
    try:    
        RepeatEnumerated(name,list_of_strings)
        return True
    except RuntimeError: 
        return False
    except TypeError: 
        return False
 
def check_repeat_string(name, list_of_strings):
    try:    
        RepeatString(name,list_of_strings)
        return True
    except RuntimeError: 
        return False
    except TypeError: 
        return False
        
def check_node_name(name):
    try:
        Task(name)
        Family(name)
        Suite(name)
        return True;
    except RuntimeError: 
        return False
  
def check_defs(path_to_defs):
    try:    
        Defs(path_to_defs)
        return True
    except RuntimeError: 
        return False
          
if __name__ == "__main__":
    print("####################################################################")
    print("Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")")
    print("####################################################################")
 
    # Names with leading '.' should not be allowed. Will interfere with triggers
    # Empty names not allowed
    # Spaces not allowed
    invalid_names = [ ".", "", " ","   ",  "fred doc", "1 "]
    
    # Allow names with leading underscore
    valid_names = [ "_", "__", "_._", "1.2", "fred.doc", "_.1"]
    
    assert check_day("monday"),            "Expected valid day"
    assert check_day("tuesday"),           "Expected valid day"
    assert check_day("wednesday"),         "Expected valid day"
    assert check_day("thursday"),          "Expected valid day"
    assert check_day("friday"),            "Expected valid day"
    assert check_day("saturday"),          "Expected valid day"
    assert check_day("sunday"),            "Expected valid day"
    assert check_day("")        == False,  "Expected exeception"
    assert check_day("sunday1") == False,  "Expected exeception"
    assert check_day("2")       == False,  "Expected exeception"

    assert check_date(0,1,2010),            "Expected valid date"
    assert check_date(10,0,2010),           "Expected valid date"
    assert check_date(10,1,0),              "Expected valid date"
    assert check_date(0,0,0),               "Expected valid date"
    assert check_date(40,1,2010) == False,  "Expected exception since day > 31"
    assert check_date(-10,1,2010) == False, "Expected exception since day >= 0"
    assert check_date(1,14,2010) == False,  "Expected exception since month > 12"
    assert check_date(1,-1,2010) == False,  "Expected exception since month >= 0"
    assert check_date(1,1,-2) == False,     "Expected exception since year >= 0"

    assert check_date_str("*.1.2010"),            "Expected valid date"
    assert check_date_str("10.*.2010"),           "Expected valid date"
    assert check_date_str("10.1.*"),              "Expected valid date"
    assert check_date_str("*.*.*"),               "Expected valid date"
    assert check_date_str("40.1.2010") == False,  "Expected exception since day > 31"
    assert check_date_str("-10.1.2010") == False, "Expected exception since day >= 0"
    assert check_date_str("1.14.2010") == False,  "Expected exception since month > 12"
    assert check_date_str("1.-1.2010") == False,  "Expected exception since month >= 0"
    assert check_date_str("1.1.-2") == False,     "Expected exception since year >= 0"

    # clock do not support wild carding hence we cant use 0 like in Date
    assert check_clock(12,1,2010),           "Expected valid date"
    assert check_clock(10,1,2010),           "Expected valid date"
    assert check_clock(10,1,1400),           "Expected valid date"
    assert check_clock(31,12,2010),          "Expected valid date"
    assert check_clock(40,1,2010) == False,  "Expected exception since day > 31"
    assert check_clock(-10,1,2010) == False, "Expected exception since day >= 0"
    assert check_clock(1,14,2010) == False,  "Expected exception since month > 12"
    assert check_clock(1,-1,2010) == False,  "Expected exception since month >= 0"
    assert check_clock(1,1,-2) == False,     "Expected exception since year >= 0"

    assert check_meter("m",0,100,100),             "Expected valid Meter"
    assert check_meter("m",0,100,0),               "Expected valid Meter"
    assert check_meter("m",200,100,100) == False,  "Expected exception since min > max"
    assert check_meter("m",0,100,-20) == False,    "Expected exception since color_change should between min-max"
    assert check_meter("m",0,100,200) == False,    "Expected exception since color_change should between min-max"
    assert check_meter("",0,100,100) == False,     "Expected exception since no name specified"
    assert check_meter(" ",0,100,100) == False,    "Expected Exception can not have spaces for a name"

    assert check_queue("m",["a"]),                 "Expected valid Queue"
    assert check_queue("m",["a","b"]),             "Expected valid Queue"
    assert check_queue("",["a","b"]) == False,     "Expected exception queue name is empty"
    assert check_queue(" ",["a","b"]) == False,    "Expected Exception can not have spaces for a name"
    assert check_queue(".",["a","b"]) == False,    "Expected Exception can not start name with a ."
    assert check_queue("m",[]) == False,           "Expected Exception queue items list is empty"
 
    assert check_event(1),                            "Expected valid Event"
    assert check_event(2),                            "Expected valid Event"
    assert check_event_number_and_name(2,"fred"),     "Expected valid Event"
    assert check_event_number_and_name(2,2) == False, "Expected failure since the name is not a string"

    assert check_repeat_date("m",20000101,20001201,200),          "Expected valid repeat"
    assert check_repeat_date("m",20001201,20000101,200) == False, "Expected exception since end YMD > start YMD"
    assert check_repeat_date("m",200001011,20001201,200)== False, "Expected Exception since start is invalid."
    assert check_repeat_date("m",20000101,200012013,200)== False, "Expected Exception since send is invalid."
    assert check_repeat_date("m",00000000,00000000,200)== False,  "Expected Exception since start/end are not valid dates is invalid."
    assert check_repeat_date("",20000101,20001201,200)==False,    "Expected Exception since no name specified"
    assert check_repeat_date(" ",20000101,20001201,200)==False,   "Expected Exception can not have spaces for a name"
    assert check_repeat_integer("name",0, 10, 2 ),                "Expected valid repeat"
    assert check_repeat_integer("",0, 10, 2 )==False,             "Expected Exception since no name specified"
    assert check_repeat_integer(" ",0, 10, 2 )==False,            "Expected Exception can not have spaces for a name"
    assert check_repeat_string("name",[ "a" ]),                   "Expected valid repeat"
    assert check_repeat_string("",["a"] )==False,                 "Expected Exception since no name specified"
    assert check_repeat_string(" ",["a"] )==False,                "Expected Exception can not have spaces for a name"
    assert check_repeat_string("name",[ 1,2 ])==False,            "Expected Exception since a list of strings was expected"
    assert check_repeat_string("name",[])==False,                 "Expected Exception since list of strings is empty"
    assert check_repeat_enumerated("name",[ "a" ]),               "Expected valid repeat"
    assert check_repeat_enumerated("",["a"] )==False,             "Expected Exception since no name specified"
    assert check_repeat_enumerated(" ",["a"] )==False,            "Expected Exception since no name specified"
    assert check_repeat_enumerated("name",[ 1,2 ])==False,        "Expected Exception since a list of strings was expected"
    assert check_repeat_enumerated("name",[])==False,             "Expected Exception since list is empty"


    assert check_variable("name","value"),        "Expected valid Variable"
    assert check_variable("name",""),             "Expected valid Variable"
    assert check_variable("name"," "),            "Expected valid Variable"
    assert check_variable("name","12"),           "Expected valid Variable"
    assert check_variable("","12")==False,        "Expected Exception name must be specified"
    assert check_variable(" ","12")==False,       "Expected Exception can not have spaces for a name"

    assert check_label("name","value"),        "Expected valid label"
    assert check_label("name",""),             "Expected valid label"
    assert check_label("name"," "),            "Expected valid label"
    assert check_label("name","12"),           "Expected valid label"
    assert check_label("","12")==False,        "Expected exception name must be specified"
    assert check_label(" ","12")==False,       "Expected Exception can not have spaces for a name"

    assert check_limit("name",1),              "Expected valid limit"
    assert check_limit("name",20000),          "Expected valid limit"
    assert check_limit("name","ten")==False,   "Expected exception, token must be a integer"
    assert check_limit("name","2")==False,     "Expected exception, token must be a integer"
    assert check_limit("","2")==False,         "Expected exception, no name specified"
    assert check_limit(" ","2")==False,        "Expected exception, can not have spaces for a name"

    assert check_inlimit("limit_name","/path/to/limit",1),      "Expected valid in limit"
    assert check_inlimit("limit_name","/path/to/limit",999999), "Expected valid in limit"
    assert check_inlimit("limit_name","",1),                    "Expected valid in limit"
    assert check_inlimit("","",1)==False,                       "Expected exception, no limit name specified"
    assert check_inlimit(" ","",1)==False,                      "Expected exception, can not have spaces for a name"
 

    # ========================================================================
    print("Check node names")
    for i in range(25):
        assert check_node_name(str(i)),             "Integer names should be allowed"
     
    for name in valid_names:
        assert check_node_name(name),  "Expected valid name " + name
    
    for name in invalid_names:
        assert check_node_name(name)==False,  "Expected exception for invalid name " + name

   
    assert check_defs("a_made_up_path_that_doesnt_not_exit.def") == False, "Expected exception, Defs file does not exist"

    # =================================================================================
    print("test save_as_defs")
    defs = Defs()                      # create a empty definition
    s1 = defs.add_suite("s1")          # create a suite "s1" and add to defs
    defs.save_as_defs("testerror.def") # create a defs on disk
    assert check_defs("testerror.def"), "Expected defs file to exist"
    os.remove("testerror.def")

    # =================================================================================
    print("Check duplicate suites not allowed")
    test_passed = False
    try :
        defs = Defs()               # create a empty definition
        s1 = defs.add_suite("s1")   # create a suite "s1" and add to defs
        s2 = defs.add_suite("s1")   # Exception thrown trying to add suite name "s1" again
    except RuntimeError as e : 
        test_passed = True
        pass 
    assert test_passed,"duplicate suite test failed"   

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_today("00:30")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding today at the suite level should fail"   
    print("check adding today at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_time("00:30")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding time at the suite level should fail"    
    print("check adding time at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_date(1,1,2016)
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding date at the suite level should fail"    
    print("check adding date at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_day("monday")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding day at the suite level should fail"    
    print("check adding day at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_trigger("1 == 0")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding a trigger at suite level should fail"    
    print("check adding trigger at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_part_trigger("1 == 0")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding a part trigger at suite level should fail"    
    print("check adding part trigger at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_complete("1 == 0")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding a complete trigger at suite level should fail"    
    print("check adding complete trigger at the suite level: RuntimeError: ")

    # =================================================================================
    test_passed = False
    try:
        defs = Defs()
        defs.add_suite("1").add_part_complete("1 == 0")
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Adding a part complete trigger at suite level should fail"    
    print("check adding part complete trigger at the suite level: RuntimeError: ")

    # =================================================================================
    print("check duplicate family not allowed")
    test_passed = False
    try:
        suite = Suite("1")
        fam1 = Family("1")
        fam2 = Family("1")
        suite.add_family(fam1)
        suite.add_family(fam2)
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate Family test failed"   
  
    # =================================================================================
    print("duplicate task not allowed")
    test_passed = False
    try:
        suite = Suite("1")
        ta = Task("a")
        tb = Task("a")
        suite.add_task(ta)
        suite.add_task(tb)
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate Task test failed"   

    # =================================================================================
    print("Task and family of same name should not be allowed")
    test_passed = False
    try:
        suite = Suite("1")
        ta = Task("a")
        tb = Family("a")
        suite.add_task(ta)
        suite.add_family(tb)
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"Task and family of same name should not be allowed"   

    # =================================================================================
    print("check duplicate meter not allowed")
    test_passed = False
    try:
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_meter("meter",0,100);
        ta.add_meter("meter",0,100);
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate meter test failed"   

    # =================================================================================
    print("check duplicate queue not allowed")
    test_passed = False
    try:
        defs = Defs()
        suite = defs.add_suite("1")
        suite.add_queue("q",["a"]);
        suite.add_queue("q",["a","b"]);
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate queue test failed"   

    # =================================================================================
    print("check duplicate event not allowed")
    test_passed = False
    try:
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event(1);
        ta.add_event("1");
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate event test failed"   
    
    test_passed = False
    try:
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event("name");
        ta.add_event("name");
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate event test failed"   
    
    test_passed = False
    try:
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event(1,"name");
        ta.add_event(1,"name");
    except RuntimeError as e : 
        test_passed = True
        pass
    assert test_passed,"duplicate event test failed"   
    
    
    # =================================================================================
    print("check cannot add same node to different containers")
    defs1 = Defs();
    suite = defs1.add_suite("s1")
    family =  suite.add_family("f1")
    task  = family.add_task("t1")
    
    test_passed = False
    try :
        defs2 = Defs();
        defs2.add_suite(suite)
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add same suite to two different defs"   

    test_passed = False
    try :
        defs3 = Defs();
        suite = defs3.add_suite(suite)
        suite.add_family(family)
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add same family to two different suites"   

    test_passed = False
    try :
        defs4 = Defs();
        suite = defs4.add_suite("s1")
        suite.add_task(task)
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add same task to two different containers"   

    test_passed = False
    try :
        defs4 = Defs();
        suite = defs4.add_suite("s1")
        suite.add_autocancel(3)
        suite.add_autocancel(4)
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add two autocancel on same node" 
    
    test_passed = False
    try :
        defs4 = Defs();
        suite = defs4.add_suite("s1")
        suite.add_autoarchive(3)
        suite.add_autocancel(4)
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add autocancel and autoarchive on same node"  
    
    test_passed = False
    try :
        defs4 = Defs();
        suite = defs4.add_suite("s1")
        suite.add_autorestore(["/s1"])
        suite.add_autorestore(["/s1"])
    except RuntimeError as e : 
        test_passed = True
        pass    
    assert test_passed,"Can't add two autorestore on same node"  
    print("All Tests pass")