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
    RepeatString,
    RepeatInteger,
    RepeatDate,
    InLimit,
    Limit,
    Late,
)


def create_family_f6():
    return Family(
        "f6", Edit(SLEEP=120), Task("t1", Late(complete="+00:01"))
    )  # set late flag if task t1 takes longer than a minute


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_f6()))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
