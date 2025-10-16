#!/usr/bin/env python3

import pathlib
from ecflow import Client

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    try:
        ci = Client()

        print("[1] Suspend the 'test' suite")
        ci.suspend("/test")

        print("[2] Replace the 'test' suite")
        ci.replace( "/test", str(base / "test.def"))

    except RuntimeError as e:
        print("Failed:", e)
