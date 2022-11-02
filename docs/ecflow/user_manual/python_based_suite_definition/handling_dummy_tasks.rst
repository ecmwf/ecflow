.. _handling_dummy_tasks:

Handling Dummy Tasks
//////////////////////

Sometimes tasks are created for which there are no associated '.ecf'
file. During job generation in the server and checking via the python
API, these tasks will show as errors. To suppress job generation
errors, a task can be marked as a dummy task.

.. code-block:: python

  the_task = ecflow.Task("t1")
  the_task.add_variable("ECF_DUMMY_TASK", "any")  # the_task += Edit(ECF_DUMMY_TASK="any")
