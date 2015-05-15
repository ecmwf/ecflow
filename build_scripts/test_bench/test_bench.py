# Used to build ecflow, incrementally and nightly
import os
import ecflow 

# =====================================================================================
# Load the defs from disk
# =====================================================================================
generated_defs_file = os.getenv("WK") + "/build_scripts/test_bench/test_force_cmd.def"
defs = ecflow.Defs(generated_defs_file)
 
# =====================================================================================
# Generated scripts in $SCRATCH
# This assumes the ECF_HOME and ECF_INCLUDE have not been defined
# =====================================================================================
for suite in defs.suites:
    suite.add_variable("ECF_HOME", os.getenv("WK") + "/build_scripts/test_bench/test_force_cmd")
    suite.add_variable("ECF_INCLUDE", os.getenv("WK") + "/build_scripts/test_bench/test_force_cmd/includes")

defs.generate_scripts()


# ====================================================================================
# Check job generation     
# ====================================================================================
job_ctrl = ecflow.JobCreationCtrl()  # no set_node_path() hence check job generation for all tasks
defs.check_job_creation(job_ctrl)
assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()


# =====================================================================================
# delete the definition in the server, load/replace a new definition
# Assumes server already started
# ======================================================================================
ci = ecflow.Client("localhost","4141")
try:
    ci.restart_server();
   
    ci.delete_all(True)           # clear out the server
    ci.load(defs)             # load the definition into the server
    
    #ci.replace("/suite",defs,True,True)     
    ci.begin_all_suites()   # need only begin once 
 
except RuntimeError, e:
    print "failed: " + str(e)
