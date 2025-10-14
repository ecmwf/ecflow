#!/usr/bin/env python3

from ecflow import Client

if __name__ == "__main__":

    try:
        # The default is to contact localhost:3141, but will
        # use ECF_HOST and/or ECF_PORT env variables if set.
        ci = Client()
        ci.ping()

        # Explicitly set host and port (n.b. reuses the existing Client)
        ci.set_host_port("machineX:4141")
        ci.ping()

        # Explicitly set host and port when creating the Client
        ci = Client("oetzi:3444")
        ci.ping()

        # Ping inlined
        Client("polonius:4266").ping()

    except RuntimeError as e:
        print(f"ping failed: {e}", )
