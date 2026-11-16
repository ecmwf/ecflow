import os
from ecflow import Defs, Suite, Family, Task, Edit, Permissions

# The user running this script; it must be listed by the server attribute
user = os.getenv("USER")

print("Creating suite definition")
home = os.path.join(os.getenv("HOME"), "course")
defs = Defs(
    Suite(
        "test",
        Edit(ECF_INCLUDE=home, ECF_HOME=home),
        Family(
            "f1",
            Permissions("{}:rxp".format(user)),
            Edit(SLEEP=20),
            Task("t1"),
            Task("t2"),
        ),
    )
)
print(defs)

print("Checking the permissions against the server attribute")
server = Permissions("{}:rwxlop,guest:r".format(user))
assert defs.check_permissions(server) == "", defs.check_permissions(server)

print("Saving definition to file 'test.def'")
defs.save_as_defs("test.def")
