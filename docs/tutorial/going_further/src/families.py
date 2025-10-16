#!/usr/bin/env python3

import pathlib
from ecflow import Defs, Suite, Family, Task, Edit

def create_family_f1():
    return Family("f1", Task("t1"), Task("t2"))

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    print("[1] Creating suite definition")

    defs = Defs(
        Suite("test",
              Edit(ECF_INCLUDE=str(base), ECF_HOME=str(base)),
              create_family_f1()))
    print(defs)

    print("[2] Checking job creation: .ecf -> .job0")
    print(defs.check_job_creation())

    print("[3] Saving definition to file 'test.def'")
    defs.save_as_defs(str(base / "test.def"))
