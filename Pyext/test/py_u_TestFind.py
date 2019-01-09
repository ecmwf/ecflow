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

#  code for testing pointers and hierarchy in python

from ecflow import Suite, Family, Task, Defs, Client, debug_build
import ecflow_test_util as Test
import os

def create_defs(name=""):
    defs = Defs()
    defs.add_variable("DEFS_VAR","0")
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    suite = defs.add_suite(suite_name);
    suite.add_variable("SUITE_VAR","1")

    f1 = suite.add_family("f1")
    f1.add_task("f1_t1").add_variable("TASK_VAR","3")
    f1.add_task("f1_t2").add_variable("TASK_VAR","3")
    f2 = suite.add_family("f2")
    f2.add_task("f2_t1").add_variable("TASK_VAR","3")
    f2.add_task("f2_t2").add_variable("TASK_VAR","3")
    return defs;

if __name__ == "__main__":   
    
    Test.print_test_start(os.path.basename(__file__))
 
    defs = create_defs()
    s1 = defs.find_suite("s1")
    assert s1 != None, "expected find find suite s1"
    assert "s1" in defs, "expected find find suite s1"

    sx = defs.find_suite("sx")
    assert sx == None, "expected not to find suite sx"
    
    f1 = s1.find_family("f1")
    assert f1 != None, "expected find find family f1"
    f1 = s1.find_node("f1")
    assert f1 != None, "expected find find family f1"

    fx = s1.find_family("fx")
    assert fx == None, "expected not to find family fx"
    fx = s1.find_node("fx")
    assert fx == None, "expected not to find family fx"

    f2 = s1.find_family("f2")
    assert f2 != None, "expected find find family f2"
    f2 = s1.find_node("f2")
    assert f2 != None, "expected find find family f2"
    
    f2_t1 = f2.find_task("f2_t1")
    assert f2_t1 != None, "Expected to find task"

    f2_t2 = f2.find_task("f2_t2")
    assert f2_t2 != None, "Expected to find task"
    f2_t2 = f2.find_node("f2_t2")
    assert f2_t2 != None, "Expected to find task"

    f2_tx = f2.find_task("f2_tx")
    assert f2_tx == None, "Expected not to find task"
    f2_tx = f2.find_node("f2_tx")
    assert f2_tx == None, "Expected not to find task"

    tasks = defs.get_all_tasks()
    assert len(tasks) == 4, "Expected four tasks, but found " + str(len(tasks))
    for task in tasks:
        # test find variable
        var = task.find_parent_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"

        var = task.find_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"
        
        var = task.find_parent_variable("SUITE_VAR");
        assert not var.empty()
        assert var.value() == "1"
         
        var = task.find_parent_variable("DEFS_VAR")
        assert not var.empty()
        assert var.value() == "0"

        var = task.find_parent_variable("MADE_UP_VAR")
        assert var.empty()

        if task.name() == "f1_t1":
            node = task.find_node_up_the_tree("f1")
            assert node != None
            assert node.get_abs_node_path() == "/s1/f1"
            
            node = task.find_node_up_the_tree("s1")
            assert node != None
            assert node.get_abs_node_path() == "/s1"

            node = task.find_node_up_the_tree("freddd")
            assert node == None
            
    print("All Tests pass")
