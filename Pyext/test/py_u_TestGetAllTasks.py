#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2012 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

# Simple check for get all tasks
from ecflow import Defs, Client, debug_build
  
def create_defs():
    defs = Defs()
    suite = defs.add_suite("test_get_all")
    suite.add_task("t0")
    fam = suite.add_family("f1")
    fam.add_task("t1") 
    fam.add_task("t2") 
    fam2 = fam.add_family("f2")
    fam2.add_task("t3") 
    return defs
   
def test_get_all_tasks(defs):
  
    task_vec2 = defs.get_all_tasks()
    assert len(task_vec2) == 4, "Expected four tasks, but found " + str(len(task_vec2))
   
    assert task_vec2[0].name() == "t0", "Expected task of name t0 but found " + task_vec2[0].name()
    assert task_vec2[1].name() == "t1", "Expected task of name t1 but found " + task_vec2[1].name()
    assert task_vec2[2].name() == "t2", "Expected task of name t2 but found " + task_vec2[2].name()
    assert task_vec2[3].name() == "t3", "Expected task of name t3 but found " + task_vec2[3].name()
   
    print "test_get_all_tasks PASSED"
   
def test_get_all_nodes(defs):
    
    # Get all the nodes make sure they are in order
    node_vec = defs.get_all_nodes()
    assert len(node_vec) == 7, "Expected seven nodes, but found " + str(len(node_vec))
    assert node_vec[0].name() == "test_get_all", "Nodes should be returned in order, expected test_get_all but found" + node_vec[0].name()
    assert node_vec[1].name() == "t0", "Nodes should be returned in order, expected 't0' but found" + node_vec[1].name()
    assert node_vec[2].name() == "f1", "Nodes should be returned in order, expected 'f1' but found" + node_vec[2].name()
    assert node_vec[3].name() == "t1", "Nodes should be returned in order, expected 't1' but found" + node_vec[3].name()
    assert node_vec[4].name() == "t2", "Nodes should be returned in order, expected 't2' but found" + node_vec[4].name()
    assert node_vec[5].name() == "f2", "Nodes should be returned in order, expected 'f2' but found" + node_vec[5].name()
    assert node_vec[6].name() == "t3", "Nodes should be returned in order, expected 't3' but found" + node_vec[6].name()

    # test empty defs
    empty_defs = Defs()
    node_vec = empty_defs.get_all_nodes()
    assert len(node_vec) == 0, "Expected zero nodes, for an empty defs but found " + str(len(node_vec))

    # test one suite defs
    one_suite_defs = Defs()
    one_suite_defs.add_suite("s1")
    node_vec = one_suite_defs.get_all_nodes()
    assert len(node_vec) == 1, "Expected one node, but found " + str(len(node_vec))

    print "test_get_all_nodes PASSED"
    
def test_get_all_nodes_from_nodes():
    defs = Defs()
    suite = defs.add_suite("test_get_all")
    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 1, "Expected one nodes but found " + str(len(node_vec))

    fam1 = suite.add_family("f1")
    fam2 = suite.add_family("f2")
    fam3 = suite.add_family("f3")
    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 4, "Expected 4 nodes but found " + str(len(node_vec))

    node_vec = fam1.get_all_nodes()
    assert len(node_vec) == 1, "Expected 1 nodes but found " + str(len(node_vec))

    f1_t1 = fam1.add_task("t1") 
    f1_t2 = fam1.add_task("t2") 
    node_vec = fam1.get_all_nodes()
    assert len(node_vec) == 3, "Expected 3 nodes but found " + str(len(node_vec))

    node_vec = f1_t1.get_all_nodes()
    assert len(node_vec) == 1, "Expected 1 nodes but found " + str(len(node_vec))

    node_vec = suite.get_all_nodes()
    assert len(node_vec) == 6, "Expected 6 nodes but found " + str(len(node_vec))

    print "test_get_all_nodes_from_nodes PASSED"
   

if __name__ == "__main__":
    print "####################################################################"
    print "Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")"
    print "####################################################################"
 
    test_get_all_tasks(create_defs())
    test_get_all_nodes(create_defs())
    test_get_all_nodes_from_nodes()
    print "All Tests pass"    