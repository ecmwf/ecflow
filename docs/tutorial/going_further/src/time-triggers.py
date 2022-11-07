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
    Edit,
)


def create_family_f2():
    return Family(
        "f2",
        Edit(SLEEP=20),
        Task("t1", Trigger(":ECF_DATE ==20200720 and :TIME >= 1000")),
        Task("t2", Trigger(":DOW == 4 and :TIME >= 1300")),
        Task("t3", Trigger(":DD == 1 and :TIME >= 1200")),
        Task(
            "t4",
            Trigger("(:DOW == 1 and :TIME >= 1300) or (:DOW == 5 and :TIME >= 1000)"),
        ),
        Task("t5", Trigger(":TIME == 0002")),
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_f2()))
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0, errors

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
