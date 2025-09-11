
Native Python tasks
*******************

Here is an example of a script as a local native Python task: This example shows that we can still take advantage of pre-processing and variable substitution. This will work on the manual and comments section, these will also be pre-processed.

There are several ways of doing this. The following are examples.

* There are two ways of accessing the ecFlow child commands (init, event, meter, label, abort, complete). We can either call the child commands directly using ecFlow extension, or we can call system command to access :term:`ecflow_client`. The following examples will use the ecFlow Python extension. This requires that the PYTHONPATH is set to the directory where ecflow.so extension was installed.

* definition file: The default ECF_MICRO is %, this may interfere with your Python scripts. In this case you either redefine it, in the task definition or directly in the Python script. 

    .. literalinclude:: src/python.def
        :language: shell

    Notice that the ECF_JOB_CMD calls Python. This allows us to change the Python version, within the viewer. Alternatively it can be omitted, providing we add "#!/usr/bin/env python" as th first line of our Python script.

* headers:

    .. literalinclude:: src/head.py
        :language: python

    .. literalinclude:: src/tail.py
        :language: python

* task wrapper:

    .. literalinclude:: src/python.ecf
