.. _tutorial-checking-the-results:

Inspecting the results
====================

The state of a :term:`suite` can be checked using the CLI :term:`ecflow_client` or programmatically via the :ref:`Python API <python_api>`.

There are several interesting information to be inspected regarding the execution of the :term:`suite`, including:

#. The :term:`suite definition` structure loaded into the :term:`ecflow_server`.
#. The :term:`suite definition` including the :term:`status` of each :term:`node`.
#. The :term:`job file` created by the server.
#. The output of the executed :term:`task`.

Retrieving the suite definition
-------------------------------

.. tabs::

   .. tab:: CLI

       To retrieve the :term:`suite definition` in a form that is parse-able, use the :code:`--get` command:

       .. code-block:: shell

          ecflow_client --get

   .. tab:: Python

       The following Python snippet shows how to retrieve the :term:`suite definition` using the :ref:`Python API <python_api>`:

       .. code-block:: python

          import ecflow

          try:
             ci = ecflow.Client()                              # create the client, will read ECF_HOST and ECF_PORT from environment
             ci.sync_local()                                   # get server definition, by syncing with client defs
             ecflow.PrintStyle.set_style( ecflow.Style.DEFS )  # set printing to show structure
             print(ci.get_defs())                              # print the returned suite definition
          except RuntimeError as e:
             print("Failed:",e)

Retrieving the suite state
--------------------------

.. tabs::

   .. tab:: CLI

       To retrieve the :term:`suite definition` including the state:

       .. code-block:: shell

          ecflow_client --get_state

   .. tab:: Python

       The following Python snippet shows how to retrieve the :term:`suite definition` including the state using the :ref:`Python API <python_api>`:

       .. code-block:: python

          import ecflow

          try:
             ci = ecflow.Client()
             ci.sync_local()                                     # retrieve server definition, by sync with client defs
             ecflow.PrintStyle.set_style( ecflow.Style.STATE )   # set printing to show structure and state, expanded trigger expression, generated variables
             print(ci.get_defs())                                # print the returned suite definition
             ecflow.PrintStyle.set_style( ecflow.Style.MIGRATE ) # set printing to show structure and state, and node history
             print(ci.get_defs())                                # print the returned suite definition
          except RuntimeError as e:
             print("Failed:",  e)

       To list just the node paths and states in python please see: :ref:`print-all-states`

Inspect the job file
--------------------

The :term:`job file` created by the server is located in the same directory as the :term:`ecf script`, and is named after the task name and the extension :code:`.jobN` (e.g. :code:`t1.job1`) where *N* is based on :code:`ECF_TRIES`.

Is is usefull to compare the files jobs files and include files, with the job file.

Inspect the job output
----------------------

The output of the job is located in the same directory as the :term:`ecf script`, and is named after the task and the extension `.N` (e.g. :code:`t1.1`) where *N* is based on :code:`ECF_TRIES`.

This output file contains the standard output and standard error of the executed job.

**What to do**

#. Use the CLI :term:`ecflow_client` to retrieve the :term:`suite definition`
#. Use the CLI :term:`ecflow_client` to retrieve the :term:`suite definition` including the state
#. (Optional) Create the Python script to retrieve the :term:`suite definition`, with and without the state
#. Inspect the :term:`job file` and the output file
