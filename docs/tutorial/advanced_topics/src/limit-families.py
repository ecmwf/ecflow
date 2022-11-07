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


def create_family(name):
    return Family(
        name,
        # limit_name(fam),limit_path(""),no_of_tokens_to_consume(1),limit node(True), limit submission(False)
        InLimit("fam", "", 1, True, False),
        [Task("t{}".format(i)) for i in range(1, 10)],
    )


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite(
        "test",
        Edit(ECF_INCLUDE=home, ECF_HOME=home, SLEEP=20),
        Limit("fam", 2),
        create_family("lf1"),
        create_family("lf2"),
        create_family("lf3"),
    )
)
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions and inlimits")
assert len(defs.check()) == 0, defs.check()

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
