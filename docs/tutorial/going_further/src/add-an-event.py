import os
from ecflow import Defs, Suite, Family, Task, Edit, Trigger, Event


def create_family_f1():
    return Family(
        "f1",
        Edit(SLEEP=20),
        Task("t1"),
        Task("t2", Trigger("t1 == complete"), Event("a"), Event("b")),
        Task("t3", Trigger("t2:a")),
        Task("t4", Trigger("t2:b")),
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_f1()))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
