.. _stop_at_first_problem:

Stop at first problem
*********************

It can be as simple as adding a line trigger to prevent a family (orcsuite) to continue any further as soon as a problem (abort) occurs:

.. code-block:: shell

  family forecast
  trigger forecast != aborted


Alternatively a limit may play a similar role, as it may be reduced to 0 manually by an Analyst or an Operator, or even a dedicated batch script.

.. code-block:: shell

  family forecast
  limit hpc 100
  inlimit hpc # use: ecflow_client --alter=change limit hpc 0 $path # to stop

