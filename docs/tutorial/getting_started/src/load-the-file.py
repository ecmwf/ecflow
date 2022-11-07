import ecflow

try:
    print("Loading definition in 'test.def' into the server")
    ci = ecflow.Client()
    ci.load("test.def")  # read definition from disk and load into the server
except RuntimeError as e:
    print("Failed:", e)
