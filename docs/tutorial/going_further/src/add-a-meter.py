import os
from ecflow import Defs, Suite, Family, Task, Edit, Trigger, Complete, Event, Meter


def create_family_f1():
    return Family(
        "f1",
        Edit(SLEEP=20),
        Task("t1", Meter("progress", 1, 100, 90)),
        Task("t2", Trigger("t1 == complete"), Event("a"), Event("b")),
        Task("t3", Trigger("t2:a")),
        Task("t4", Trigger("t2 == complete"), Complete("t2:b")),
        Task("t5", Trigger("t1:progress ge 30")),
        Task("t6", Trigger("t1:progress ge 60")),
        Task("t7", Trigger("t1:progress ge 90")),
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_f1()))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
