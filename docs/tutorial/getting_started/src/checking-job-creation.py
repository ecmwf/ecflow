#!/usr/bin/env python3

import pathlib
from ecflow import Defs, Suite, Task, Edit

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    print("[1] Creating suite definition")

    defs = Defs(Suite("test", Edit(ECF_HOME=str(base)), Task("t1")))
    print(defs)

    print("[2] Checking job creation: .ecf -> .job0")
    print(defs.check_job_creation())

    # Consider asserting, to progress once job creation works
    # assert len(defs.check_job_creation()) == 0, "Job generation failed"
