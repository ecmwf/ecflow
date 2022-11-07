import os
from ecflow import (
    Defs,
    Suite,
    Family,
    Task,
    Edit,
    Trigger,
    Complete,
    Event,
    Meter,
    Time,
    Day,
    Date,
    Label,
)


def create_family_label():
    return Family("label", Task("t1", Label("info", "")))


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_label()))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
