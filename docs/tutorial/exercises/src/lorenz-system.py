import os
import ecf
from ecf import *

home = os.getenv("HOME") + "/ecflow_server"
user = os.getenv("USER")
port = os.getenv("ECF_PORT")
node = Suite("lorenz").add(
    Defstatus("suspended"),
    ecf.Edit(
        ECF_HOME=home,
        ECF_INCLUDE=home + "/include",
        ECF_FILES=home + "/files",
        ECF_OUT=home,
        ECF_EXTN=".ecf",
        USER=user,
        SCHOST="localhost",
        ECF_JOB_CMD="/home/ma/emos/bin/trimurti %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%",
    ),
    ecf.Family("ecf").add(
        Task("compute").add(),
        ecf.Family("multi").add(  # once imported, alter script, run multiple tasks
            [
                ecf.Family("%02d" % num).add(
                    Edit(XYZ="[%d.0, %d.0, %d.0]" % (num, num, num)), Task("compute")
                )
                for num in range(0, 5)
            ]
        ),
    ),
)
client = ecf.Client("localhost@%s" % port)
defs = ecf.Defs()
defs.add_suite(node)
client.replace("/lorenz", defs)
