#!/usr/bin/env python3

import pathlib
from ecflow import Client

if __name__ == '__main__':

    base = pathlib.Path.home() / "course"

    try:
        ci = Client()

        print("[1] Load suite definition in 'test.def' into the server")
        ci.load(str(base / "test.def"))
        # line above reads suite definition from disk and loads into the server

    except RuntimeError as e:
        print("Failed:", e)
