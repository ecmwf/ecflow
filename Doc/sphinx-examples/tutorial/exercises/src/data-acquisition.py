#!/usr/bin/env python2.7
import os
import ecflow  
   
defs = ecflow.Defs()
suite = defs.add_suite("data_aquisition")
suite.add_repeat( ecflow.RepeatDay(1) )
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_FILES",   os.getenv("HOME") + "/course/data")
suite.add_variable("SLEEP","2")
for city in ( "exeter", "toulouse", "offenbach", "washington", "tokyo", "melbourne", "montreal" ) :
    fcity = suite.add_family(city)
    fcity.add_task("archive")
    for obs_type in ( "observations", "fields", "images" ):
        type_fam = fcity.add_family(obs_type)
        if city in ("exeter", "toulouse", "offenbach"): type_fam.add_time("00:00 23:00 01:00")
        if city in ("washington") :                     type_fam.add_time("00:00 23:00 03:00")
        if city in ("tokyo") :                          type_fam.add_time("12:00")
        if city in ("melbourne") :                      type_fam.add_day( "monday" )
        if city in ("montreal") :                       type_fam.add_date(1, 0, 0)
         
        type_fam.add_task("get")
        type_fam.add_task("process").add_trigger("get eq complete")
        type_fam.add_task("store").add_trigger("get eq complete")
        
if __name__ == "__main__":
    print defs    