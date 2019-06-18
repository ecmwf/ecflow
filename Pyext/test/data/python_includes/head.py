import os
import ecflow

print("PYTHONPATH=====================================================")
print(os.environ['PYTHONPATH'].split(os.pathsep))

print("Creating Client")
ci = ecflow.Client()
ci.set_host_port("%ECF_HOST%","%ECF_PORT%")
ci.set_child_pid(os.getpid())
ci.set_child_path("%ECF_NAME%")
ci.set_child_password("%ECF_PASS%")
ci.set_child_try_no(%ECF_TRYNO%)

print("Only wait 2 minutes, if the server cannot be contacted (note default is 24 hours) before failing")
ci.set_child_timeout(120) 

try:
   ci.ping();
   print("ping ok")
except RuntimeError as e:
   print("Error: abort in head.h for ci.ping()" + str(e))
   ci.child_abort("Abort in head for ci.ping()" + str(e))
except:
   print("Error: abort in head.h for ci.ping()")
   ci.child_abort("Abort in head for ci.ping()")
   
try:
   ci.child_init();
   print("child init ok")
except RuntimeError as e:
   print("Error: abort in head.h for ci.child_init()" + str(e))
   ci.child_abort("Abort in head for ci.child_init()" + str(e))
except:
   print("Error: abort in head.h for ci.child_init()")
   ci.child_abort("Abort in head for ci.child_init()")
