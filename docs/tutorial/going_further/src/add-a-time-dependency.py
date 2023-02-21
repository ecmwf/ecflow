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
        Task(
            "t1", Time("00:30 23:30 00:30")
        ),  # start(hh:mm) end(hh:mm) increment(hh:mm)
        Task("t2", Day("thursday"), Time("13:00")),
        Task(
            "t3", Date("1.*.*"), Time("12:00")
        ),  # Date(day,month,year) - * means every day,month,year
        Task("t4", Time("+00:02")),  # + means relative to suite begin/requeue time
        Task("t5", Time("00:02")),
    )  # 2 minutes past midnight


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
