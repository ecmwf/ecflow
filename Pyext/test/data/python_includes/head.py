#!/usr/local/apps/python/current/bin/python

import os
import ecflow

print "PYTHONPATH====================================================="
print os.environ['PYTHONPATH'].split(os.pathsep)

print "ecflow.Client ====================================================="
print dir(ecflow.Client)

ci = ecflow.Client()
ci.set_host_port("%ECF_NODE%","%ECF_PORT%")
ci.set_child_pid(os.getpid())
ci.set_child_path("%ECF_NAME%")
ci.set_child_password("%ECF_PASS%")
ci.set_child_try_no(%ECF_TRYNO%)

try:
   ci.child_init();
except RuntimeError, e:
   print "Error: abort in head.h " + str(e)
   ci.child_abort("Abort in head" + str(e))
except:
   ci.child_abort("Abort in head")
