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
    Cron,
)


def create_family_house_keeping():
    return Family("house_keeping", Task("clear_log", Cron("22:30", days_of_week=[0])))


print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite("test", Edit(ECF_INCLUDE=home, ECF_HOME=home), create_family_house_keeping())
)
print(defs)

print("Checking job creation: .ecf -> .job0")
print(defs.check_job_creation())

print("Checking trigger expressions")
errors = defs.check()
assert len(errors) == 0, errors

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
