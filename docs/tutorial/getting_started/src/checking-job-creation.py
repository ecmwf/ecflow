#!/usr/bin/env python3

import pathlib
from ecflow import Defs

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    print("[1] Load suite definition from file 'test.def'")
    defs = Defs(str(base / "test.def"))
    print(defs)

    print("[2] Validating job creation: .ecf -> .job0")
    defs.check_job_creation(throw_on_error=True, verbose=True)
