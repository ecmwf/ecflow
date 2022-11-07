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
)


def create_family_f4():
    return Family(
        "f4",
        Edit(SLEEP=2),
        RepeatString("NAME", ["a", "b", "c", "d", "e", "f"]),
        Family(
            "f5",
            RepeatInteger("VALUE", 1, 10),
            Task(
                "t1",
                RepeatDate("DATE", 20101230, 20110105),
                Label("info", ""),
                Label("date", ""),
            ),
        ),
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_f4()))
print(defs)
print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
