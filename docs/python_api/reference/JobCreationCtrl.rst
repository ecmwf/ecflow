ecflow.JobCreationCtrl
//////////////////////


.. py:class:: JobCreationCtrl
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

The class JobCreationCtrl is used in `job creation` checking

Constructor::

   JobCreationCtrl()


Usage:

.. code-block:: python

   defs = Defs('my.def')                     # specify the definition we want to check, load into memory
   job_ctrl = JobCreationCtrl()
   job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node
   defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB
   print(job_ctrl.get_error_msg())            # report any errors in job generation

   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks
   job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory
   defs.check_job_creation(job_ctrl)
   print(job_ctrl.get_error_msg())

   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks
   job_ctrl.generate_temp_dir()              # automatically generate directory for job file
   defs.check_job_creation(job_ctrl)
   print(job_ctrl.get_error_msg())


.. py:method:: JobCreationCtrl.generate_temp_dir(self: ecflow.JobCreationCtrl) -> None
   :module: ecflow

Automatically generated temporary directory for job creation. Directory written to stdout for information


.. py:method:: JobCreationCtrl.get_dir_for_job_creation(self: ecflow.JobCreationCtrl) -> str
   :module: ecflow

Returns the directory set for job creation


.. py:method:: JobCreationCtrl.get_error_msg(self: ecflow.JobCreationCtrl) -> str
   :module: ecflow

Returns an error message generated during checking of job creation


.. py:method:: JobCreationCtrl.is_verbose(self: ecflow.JobCreationCtrl) -> bool
   :module: ecflow

Returns true if verbose mode is on, false otherwise


.. py:method:: JobCreationCtrl.set_dir_for_job_creation(self: ecflow.JobCreationCtrl, arg0: str) -> None
   :module: ecflow

Specify directory, for job creation


.. py:method:: JobCreationCtrl.set_node_path(self: ecflow.JobCreationCtrl, arg0: str) -> None
   :module: ecflow

The node we want to check job creation for. If no node specified check all tasks


.. py:method:: JobCreationCtrl.set_verbose(self: ecflow.JobCreationCtrl, arg0: bool) -> None
   :module: ecflow

Output each task as its being checked.

