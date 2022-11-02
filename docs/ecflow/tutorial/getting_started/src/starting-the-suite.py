import ecflow

try:
    ci = ecflow.Client()
    ci.sync_local()  # get the defs from the server, and place on ci
    defs = ci.get_defs()  # retrieve the defs from ci
    if len(defs) == 0:
        print("No suites in server, loading defs from disk")
        ci.load("test.def")

        print("Restarting the server. This starts job scheduling")
        ci.restart_server()
    else:
        print("read definition from disk and replace on the server")
        ci.replace("/test", "test.def")

    print("Begin the suite named 'test'")
    ci.begin_suite("test")

except RuntimeError as e:
    print("Failed:", e)
