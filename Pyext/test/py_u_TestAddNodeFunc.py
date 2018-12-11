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
from ecflow import Defs, Client, debug_build
import ecflow_test_util as Test

def create_defs_sequentially():
    local_defs = Defs()
    suite = local_defs.add_suite("s1")
    suite.add_variable("Var","value")
    suite.add_meter( "metername3", 0, 100, 50 )
    suite.add_event( 2 )
    suite.add_event( 3 )
    f1 = suite.add_family("f1")
    f1 = f1.add_family("f1")
    t1 = f1.add_task("t1")
    t1.add_variable("fred","jones")
    
    f2 = suite.add_family("f2")
    f1 = f2.add_family("f1")
    t1 = f1.add_task("t1")
    t1.add_variable("fred","jones")

    f3 = suite.add_family("f3")
    t1 = f3.add_task("t1")
    t1.add_variable("fred",    "jones")
    
    suite.add_task("t1").add_variable("fred","jones")
    return local_defs

def create_defs_functionally():
    f_defs = Defs()
    suite = f_defs.add_suite("s1")
    suite.add_variable("Var","value").add_family("f1").add_family("f1").add_task("t1").add_variable("fred","jones")
    suite.add_family("f2").add_family("f1").add_task("t1").add_variable("fred","jones")
    suite.add_meter( "metername3", 0, 100, 50 ).add_family("f3").add_task("t1").add_variable("fred","jones")
    suite.add_event( 2 ).add_event( 3 ).add_task("t1").add_variable("fred","jones")
    return f_defs
    
    
if __name__ == "__main__":
        
    Test.print_test_start()

    #
    # Add Nodes functional way
    #
    defs = Defs()
    defs.add_suite("s1").add_task("t1").add_variable("var","v")
    defs.add_suite("s2").add_family("f1").add_task("t1").add_variable("var","v")
    defs.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable("var","v")
    assert len(defs) == 3,"Expected 3 suites"    
  
    # These test show that although add_variable() returns a node_ptr, its really a suite
    # hence calling add_family/add_task still works.
    assert create_defs_sequentially() == create_defs_functionally(),"Functional suite not as expected "
  
    print("All Tests pass")
    