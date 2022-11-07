import ecflow

print("Client -> Server: replacing suite '/test' in the server, with a new definition")
try:
    ci = ecflow.Client()
    ci.suspend("/test")  # so that we can resume manually in ecflow_ui
    ci.replace(
        "/test", "test.def"
    )  # replace suite /test with suite of same name in test.def
except RuntimeError as e:
    print("Failed:", e)
