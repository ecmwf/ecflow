import ecflow

try:
    # When no arguments specified uses ECF_HOST and/or ECF_PORT,
    # otherwise defaults to localhost:3141
    ci = ecflow.Client()  # inherit from shell variables
    ci.ping()

    # Explicitly set host and port using the same client
    # For alternative argument list see ecflow.Client.set_host_port()
    ci.set_host_port(
        "machineX:4141"
    )  # actually set the host and port (change to your host and port)
    ci.ping()

    # Create a new client, Explicitly setting host and port.
    # For alternative argument list see ecflow.Client
    ci = ecflow.Client("oetzi:3444")  # another server
    ci.ping()

    # Ping inlined
    ecflow.Client("polonius:4266").ping()

except RuntimeError as e:
    print("ping failed: ", str(e))
