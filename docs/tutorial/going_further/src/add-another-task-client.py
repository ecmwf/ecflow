#!/usr/bin/env python3

import pathlib
from ecflow import Client

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    try:
        ci = Client()

        print("[1] Delete all suites from the server")
        ci.delete_all()  # clear out the server

        print("[2] Load suite definition in 'test.def' into the server")
        ci.load(str(base / "test.def"))
        # line above reads suite definition from disk and loads into the server

        print("[3] Begin the 'test' suite")
        ci.begin_suite("test")

        ### An alternative to deleting all suites before reloading
        ### is to simply replace the existing suite
        print("[1] Suspend the 'test' suite")
        ci.suspend("/test")

        print("[2] Replace the 'test' suite")
        ci.replace( "/test", "test.def")

    except RuntimeError as e:
        print("Failed:", e)
