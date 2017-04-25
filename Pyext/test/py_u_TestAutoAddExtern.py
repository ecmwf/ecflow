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

def check_then_auto_add_extern(defs):
    error_msg = defs.check() 
    print error_msg
    assert len(error_msg) !=0,"Expect error in checks\n" + str(defs)
    
    defs.auto_add_externs(True)
    error_msg = defs.check()
    assert len(error_msg) == 0,"Expect check to pass after auto add extern\n" + error_msg + "\n" + str(defs)
    
if __name__ == "__main__":    
    print("####################################################################")
    print("Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")")
    print("####################################################################")

    defs = Defs()
    error_msg = defs.check();  assert len(error_msg) == 0,"Expect empty defs to pass check"
    suite = defs.add_suite("ext");

    f1 = suite.add_family("f1")
    f1.add_task("t").add_trigger("/a/b/c/d == complete");     check_then_auto_add_extern(defs)
    f1.add_task("t1").add_trigger("/a/b/c/d/e:event == set"); check_then_auto_add_extern(defs)
    f1.add_task("t2").add_trigger("/a/b/c/d/x:event");        check_then_auto_add_extern(defs)     
    f1.add_task("t3").add_trigger("/a/b/c/d/y:meter le 30");  check_then_auto_add_extern(defs)  
    f1.add_task("t4").add_trigger("/a/b/c/d/z<flag>late");    check_then_auto_add_extern(defs)   
    
    f2 = suite.add_family("f2")
    f2.add_inlimit("hpcd","limits");                           check_then_auto_add_extern(defs)
    f2.add_task("t").add_inlimit("sg1","/suiteName");          check_then_auto_add_extern(defs)
    f2.add_task("t1").add_inlimit("hpcd","/obs/limits");       check_then_auto_add_extern(defs) 
    f2.add_task("t2").add_inlimit("c1a","/limits");            check_then_auto_add_extern(defs)     
    print defs
    print("All Tests pass")
