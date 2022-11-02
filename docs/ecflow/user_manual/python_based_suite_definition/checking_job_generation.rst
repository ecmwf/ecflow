.. _checking_job_generation:

Checking Job Generation
///////////////////////

Job generation involves the following processes:

- Locating the '.ecf' files
- Locating the includes files specified in the '.ecf' file
- Removing comment and manual pre-processor statements
- Variable substitution
- Creating the job file on disk

This is what the ecflow_server does when submitting your task/job.

This process can be checked **BEFORE** loading the suite to the server,
by using the :ref:`python_api`. Since this API is used for checking, the jobs are all
generated with the extension '.job0'. The following example checks job
generation fo all tasks.

.. code-block:: python
   :caption: Default

   defs = ecflow.Defs("my.def")  # load file 'my.def' into memory, not needed if the defs was created in python
   msg = defs.check_job_creation()  # job files generated to ECF_JOB
   print(msg)

For brevity, the following examples do not show how the 'defs' object
was created. This can be read in from disk as shown above or created
directly in python.

.. code-block:: python
   :caption: Checking of job generation for all tasks under '/suite/to_check'

   job_ctrl = ecflow.JobGenCtrl()
   job_ctrl.set_node_path("/suite/to_check")  # hierarchical job generation under /suite/to_check
   defs.check_job_generation(job_ctrl)  # do the check
   print(job_ctrl.get_error_msg())

This example shows the checking of job generation for all tasks, but
where the jobs are generated to a user-specified directory. i.e.
'/tmp/ECF_NAME.job0'.

.. code-block:: python
   :caption: Generated jobs to a user specfied directory

   job_ctrl = ecflow.JobGenCtrl()
   job_ctrl.set_dir_for_job_generation("/tmp")  # generate jobs file under this directory
   defs.check_job_generation(job_ctrl)  # do the check
   print(job_ctrl.get_error_msg())  # report any errors in job generation

This example show job checking to an automatically generated temporary
directory $TMPDIR/ecf_check_job_generation/ECF_NAME.job0

.. code-block:: python
   :caption: Generate temporary directory

   job_ctrl = ecflow.JobGenCtrl()
   job_ctrl.generate_temp_dir()
   defs.check_job_generation(job_ctrl)
   print(job_ctrl.get_error_msg())
