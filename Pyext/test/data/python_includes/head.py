import os
import ecflow

print("PYTHONPATH=====================================================")
try:
    print(os.environ['PYTHONPATH'].split(os.pathsep))
except KeyError:
    print("Could not get PYTHONPATH")

print("Creating Client")
ci = ecflow.Client()
print("Running ecflow version " + ci.version())

ci.set_host_port("%ECF_HOST%","%ECF_PORT%")
ci.set_child_pid(os.getpid())
ci.set_child_path("%ECF_NAME%")
ci.set_child_password("%ECF_PASS%")
ci.set_child_try_no(%ECF_TRYNO%)

print("Only wait 5  minutes, if the server cannot be contacted (note default is 24 hours) before failing")
ci.set_child_timeout(300) 

try:
   ci.child_init();
   print("child init ok")
except RuntimeError as e:
   print("Error: abort in head.h " + str(e))
   ci.child_abort("Abort in head" + str(e))
except:
   print("Error: abort in head.h")
   ci.child_abort("Abort in head")
