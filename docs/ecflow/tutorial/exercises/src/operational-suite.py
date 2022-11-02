#!/usr/bin/env python2.7
import os
import ecflow 
   
defs = ecflow.Defs()
suite = defs.add_suite("operation_suite")
suite.add_repeat( ecflow.RepeatDay(1) )
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_FILES",   os.getenv("HOME") + "/course/oper")
  
# Defines the triggers for the first cycle
cycle_triggers = "1"
for cycle in ( 0 , 12 ):

    if cycle == 12 :
        last_step = 240
    else:
        last_step = 24
 
    fcycle_fam =  suite.add_family(str(cycle))
    fcycle_fam.add_variable("CYCLE", cycle)
    fcycle_fam.add_variable("LAST_STEP", last_step)
      
    if cycle_triggers != "1" :
        fcycle_fam.add_trigger(cycle_triggers)
                 
    analysis_fam = fcycle_fam.add_family("analysis")
    analysis_fam.add_task("get_observations")
    analysis_fam.add_task("run_analysis").add_trigger("get_observations == complete")
    analysis_fam.add_task("post_processing").add_trigger("run_analysis == complete")
 
    forecast_fam = fcycle_fam.add_family("forecast")
    forecast_fam.add_trigger("analysis == complete")
    forecast_fam.add_task("get_input_data") 
    run_forecast_task = forecast_fam.add_task("run_forecast") 
    run_forecast_task.add_trigger("get_input_data == complete")
    run_forecast_task.add_meter("step", 0, last_step, last_step)
 
    archive_fam = fcycle_fam.add_family("archive")
    fam_analsis = archive_fam.add_family("analysis")
    fam_analsis.add_variable("TYPE","analysis")
    fam_analsis.add_variable("STEP","0")
    fam_analsis.add_trigger("../analysis/run_analysis == complete")
    fam_analsis.add_task("save")
    
    for i in range(6, last_step, 6):  
        step_fam = fam_analsis.add_family("step_" + str(i))     
        step_fam.add_variable("TYPE", "forecast")  
        step_fam.add_variable("STEP", i)  
        step_fam.add_trigger("../../forecast/run_forecast:step ge " + str(i))
        step_fam.add_task("save")

    # Defines the triggers for the next cycle
    cycle_triggers = "./" + str(cycle) + " == complete"
    
    
if __name__ == "__main__":
    print defs    
 