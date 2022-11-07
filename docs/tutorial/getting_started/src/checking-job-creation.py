import os
from ecflow import Defs, Suite, Task, Edit

print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_HOME=home), Task("t1")))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

# We can assert, so that we only progress once job creation works
# assert len(defs.check_job_creation()) == 0, "Job generation failed"
