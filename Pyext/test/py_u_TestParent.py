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

def absNodePath(node):
    """ Given a node return its absolute node path. """
    """ Example of using get_parent()               """
    name_list = []
    name_list.append(node.name())
    parent = node.get_parent();
    while parent:
        name_list.append(parent.name())
        parent = parent.get_parent()
        
    name_list.reverse()
    ret = ""
    for name in name_list:
        ret = ret +  "/"
        ret = ret + name
    return ret
    

if __name__ == "__main__":    
    print("####################################################################")
    print("Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")")
    print("####################################################################")
   
    suite = Suite("s1");
    family = Family("f1")
    task = Task("t1")

    assert not suite.get_parent(), "Suite parent is always NULL"
    assert not family.get_parent(), "Expect no parent"
    assert not task.get_parent(), "Expect no parent"
    
    print("suite.get_defs() = " + str(suite.get_defs()))
    print("family.get_defs() = " + str(family.get_defs()))
    print("task.get_defs() = " + str(task.get_defs()))
    assert not suite.get_defs(),   "Expected no defs, since suite not added to defs yet"
    assert not family.get_defs(),  "Expected no defs"
    assert not task.get_defs(),    "Expected no defs"
    
    suite.add_family(family)
    family.add_task(task)
    
    family2 = Family("f2")
    family.add_family(family2)

    t1 = Task("t1")
    family2.add_task(t1)
       
    defs = Defs()
    defs.add_suite(suite);
    
    print(absNodePath(t1))
    print(absNodePath(family2))
    print(absNodePath(family))
    print(absNodePath(suite))
    
    assert t1.get_abs_node_path() == absNodePath(t1),          "Expected " + t1.get_abs_node_path()      + " but got " + absNodePath(t1)
    assert family2.get_abs_node_path() == absNodePath(family2),"Expected " + family2.get_abs_node_path() + " but got " + absNodePath(family2)
    assert family.get_abs_node_path() == absNodePath(family),  "Expected " + family.get_abs_node_path()  + " but got " + absNodePath(family)
    assert suite.get_abs_node_path() == absNodePath(suite),    "Expected " + family.get_abs_node_path()  + " but got " + absNodePath(suite)
    print("All Tests pass")
