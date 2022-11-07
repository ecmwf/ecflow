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
)


def create_family_f5():
    return Family(
        "f5",
        InLimit("l1"),
        Edit(
            SLEEP=20,
            HOST="?????",
            ECF_OUT="/tmp/%s" % os.getenv("USER"),
            ECF_LOGHOST="%HOST%",
            ECF_LOGPORT="?????",  # port=$((35000 + $(id -u))) run this on the command line
            ECF_JOB_CMD="ssh %HOST% 'mkdir -p %ECF_OUT%/%SUITE%/%FAMILY%; %ECF_JOB% > %ECF_JOBOUT% 2>&1 &'",
        ),
        [Task("t{}".format(i)) for i in range(1, 10)],
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite(
        "test",
        Edit(ECF_INCLUDE=home, ECF_HOME=home),
        Limit("l1", 2),
        create_family_f5(),
    )
)
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
