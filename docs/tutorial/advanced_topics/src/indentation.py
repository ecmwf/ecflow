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
)

print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite(
        "test",
        Edit(ECF_INCLUDE=home, ECF_HOME=home),
        Family(
            "f1",
            Edit(SLEEP=20),
            Task("t1", Meter("progress", 1, 100, 90)),
            Task("t2", Trigger("t1 == complete"), Event("a"), Event("b")),
            Task("t3", Trigger("t2:a")),
            Task("t4", Trigger("t2 == complete"), Complete("t2:b")),
            Task("t5", Trigger("t1:progress ge 30")),
            Task("t6", Trigger("t1:progress ge 60")),
            Task("t7", Trigger("t1:progress ge 90")),
        ),
        Family(
            "f2",
            Edit(SLEEP=20),
            Task("t1", Time("00:30 23:30 00:30")),
            Task("t2", Day("sunday")),
            Task("t3", Date("1.*.*"), Time("12:00")),
            Task("t4", Time("+00:02")),
            Task("t5", Time("00:02")),
        ),
    )
)
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
