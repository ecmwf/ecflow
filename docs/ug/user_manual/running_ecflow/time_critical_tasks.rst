.. _time_critical_tasks:

Time critical tasks
///////////////////

At ECMWF we use ecFlow to schedule our operational suites. We also separate out critical and non-critical tasks, thus allowing our
operators to more easily monitor suites. In :numref:`time_critical_tasks_ui` we show the
coarse structure of one of our operational suites.

The suite is divided into four sections: "main" handles the time-critical parts of the suite, such as the actual model, "lag"
handles the archiving and other non-time-critical tasks, "pop" handles
the plotting of results and "msjobs" handle the submission of member
state jobs.

Note that each family has its own date **repeat** labelled YMD. This allows us to use triggers including the suite date (YMD) and also
allows the less critical tasks to run even a few days behind if
necessary. This is useful when running a test suite not in real-time.

.. figure:: /_static/ug/time_critical_tasks/image1.png
   :width: 5.78125in
   :height: 3.89583in
   :name: time_critical_tasks_ui

   Sample suite structure

ecFlow can help in the monitoring of suites in many ways, beyond the indication of task status.
For instance, the :ref:`late<tutorial-late-attribute>` command in ecFlow can be used to
highlight problems with time-critical scripts. The command will mark a node as late when
certain conditions are met; such as submitted for too long, running for too long, or not
active by a certain time. This is used in conjunction with a GUI to
launch a pop-up window once a late condition is reached. To use this
option you need to make sure that the GUI option "show/special/late
nodes" is selected.

In a number of our suites, we have also defined check tasks that
interrogate ecFlow using the status command to find out if tasks have,
for instance, completed at a given time.
