.. index::
   single: checking-job-creation
     
.. _checking-job-creation:

Checking job creation
=====================

In the previous section we have implemented our first task (the :file:`t1.ecf` file).

The t1.ecf script needs to be preprocessed to generate the :term:`job file`.

This :term:`pre-processing` is done automatically by :term:`ecflow_server` the when the task is about to run.

However it is possible to check the :term:`job creation` before the :term:`suite definition`
is loaded into the :term:`ecflow_server`.

Text
----
  
Automated job creation checking is only available with Python.

If the :term:`ecflow_server` can't locate the :term:`ecf script`, please see :term:`ecf file location algorithm`

Python
------

| The process of :term:`job creation` can be checked before the :term:`suite definition`
| is loaded into the :term:`ecflow_server`. The following checks are done:

* Locating :term:`ecf script` files, corresponding to the :term:`task` in the :term:`suite definition`.  
* Performing :term:`pre-processing`

| When the :term:`suite definition` is large and has many :term:`ecf script` this
| checking can save a lot of time.

The following point's should be noted about about :term:`job creation` checking:

* | It is **independent** of the :term:`ecflow_server`. 
  | Hence ECF_PORT and ECF_HOST in the :term:`job file` will have default values.
* | Job files have a **.job0** extension, whereas the server will always generate
  | jobs with a extension **.job<1-n>**,  i.e t1.job1, t1.job2. 
  | The numbers correspond to :term:`ECF_TRYNO` which is never zero.
* | By default the :term:`job file` is created in the same directory
  | as the :term:`ecf script`. See :term:`ECF_JOB`
  
Checking is done using :py:class:`ecflow.Defs.check_job_creation`

.. literalinclude:: src/checking-job-creation.py   

.. note::

   It is highly advisable that :term:`job creation` checking is enabled
   for all subsequent examples.


**What to do:**

#. Add :term:`job creation` checking.
#. Examine the job file :file:`$HOME/course/test/t1.job0`
