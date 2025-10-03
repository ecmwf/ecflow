#!/usr/bin/env python3

import pathlib
from ecflow import Defs, Suite, Task, Edit

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    print("[1] Creating suite definition")

    defs = Defs(Suite("test", Edit(ECF_HOME=str(base)), Task("t1")))
    print(defs)

    print("[2] Saving definition to file 'test.def'")
    defs.save_as_defs(str(base / "test.def"))
