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
    Autoarchive,
    Autorestore,
)


def create_family(name):
    return Family(name, Autoarchive(0), [Task("t{}".format(i)) for i in range(1, 10)])


def create_family_restore():
    return Family(
        "restore",
        Trigger("./lf1<flag>archived and ./lf2<flag>archived and ./lf3<flag>archived"),
        Task("t1", Autorestore(["../lf1", "../lf2", "../lf3"])),
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite(
        "test",
        Edit(ECF_INCLUDE=home, ECF_HOME=home, SLEEP=20),
        create_family("lf1"),
        create_family("lf2"),
        create_family("lf3"),
        create_family_restore(),
    )
)
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions and inlimits")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
