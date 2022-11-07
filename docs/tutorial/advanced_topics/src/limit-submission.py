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
        # limit_name(l1),limit_path(""),no_of_tokens_to_consume(1),limit node(False), limit submission(True)
        InLimit("l1", "", 1, False, True),
        Edit(SLEEP=20),
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

print("Checking trigger expressions and inlimits")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
