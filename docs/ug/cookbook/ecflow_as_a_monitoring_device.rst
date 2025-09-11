.. _ecflow_as_a_monitoring_device:

ecFlow as a monitoring device
*****************************

When hundreds of tiny/frequent jobs must be managed on a single system, it may be more efficient not to let ecFlow server create each instance of a job, and to avoid queuing system submission. ecFlow can still be used for monitoring only purpose.

Provided ECF_NAME, ECF_HOST, ECF_PORT, ECF_PASS=FREE are defined and they match a node on a suite hosted by the server.

When ECF_TRYNO=0 and ECF_JOBOUT match the job output, output can be directly accessible from the GUI, too.

The suite/family "looks like" suspended.

.. image:: /_static/ecflow_as_a_monitoring_device/image1.png
   :width: 4.63611in
   :height: 2.60417in
