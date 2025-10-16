#!/usr/bin/env python3

import pathlib
from ecflow import Client

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    try:
        ci = Client()

        print("[1] Syncing local defs with server")
        ci.sync_local()

        print("[2] Access the suite definition from the server")
        defs = ci.get_defs()

        print("[3] Restarting the server")
        if len(defs) == 0:
            print("No suites in server, loading defs from disk")
            ci.load(str(base / "test.def"))

            print("Restarting the server. This starts job scheduling")
            ci.restart_server()
        else:
            print("read definition from disk and replace on the server")
            ci.replace("/test", str(base / "test.def"))

        print("[4] Begin the suite")
        ci.begin_suite("test")

    except RuntimeError as e:
        print("Failed:", e)
