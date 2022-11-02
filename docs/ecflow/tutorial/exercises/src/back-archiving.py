#!/usr/bin/env python2.7
import os
import ecflow 
   
defs = ecflow.Defs()
suite = defs.add_suite("back_archiving")
suite.add_repeat( ecflow.RepeatDay(1) )
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_FILES",   os.getenv("HOME") + "/course/back")
suite.add_variable("SLEEP", "2")
suite.add_limit("access", 2)
for kind in ( "analysis", "forecast", "climatology", "observations", "images" ):
    find_fam = suite.add_family(kind)
    find_fam.add_repeat( ecflow.RepeatDate("DATE", 19900101, 19950712) )
    find_fam.add_variable("KIND", kind)
    find_fam.add_task("get_old").add_inlimit("access")
    find_fam.add_task("convert").add_trigger("get_old == complete")
    find_fam.add_task("save_new").add_trigger("convert == complete")
    
if __name__ == "__main__":
    print defs    