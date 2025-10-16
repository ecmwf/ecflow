.. index::
   single: Starting the suite (tutorial)
   
.. _tutorial-starting-suite:
   
Starting the suite
==================

A manually launched :term:`ecflow_server` will start in a :term:`halted` state, meaning that no tasks will be scheduled.
In order to start task scheduling, the server must be *restarted* and the :term:`suite` must be commanded to *begin*.

.. note::

    The :code:`ecflow_start.sh` script, referred in :ref:`tutorial-getting-started`, will automatically launch and start
    the :term:`ecflow_server`, thus skipping the need for explicitly restarting the server.

As before there are multiple ways to restart the :term:`ecflow_server` and begin the :term:`suite`,
including using the CLI :term:`ecflow_client` or programmatically via the :ref:`Python API <python_api>`.

.. tabs::

    .. tab:: CLI

        To check the status of the server, use the :code:`--stats` command:

        .. code-block:: shell

           ecflow_client --stats

        The output will indicate the server state.
        If the :term:`ecflow_server` is :term:`halted`, restart it with the :code:`--restart` command:

        .. code-block:: shell

           ecflow_client --restart

        With the :term:`ecflow_server` :term:`running`, begin the :term:`suite` with the :code:`--begin` command:

        .. code-block:: shell

           ecflow_client --begin test

    .. tab:: Python

        The following Python script demonstrates how to *restart* the :term:`ecflow_server` and begin the :term:`suite`, using the :ref:`Python API <python_api>`.
        The script also loads the :term:`suite definition` file, in case it was not loaded previously.

        .. literalinclude:: src/starting-the-suite.py
           :language: python
           :caption: $HOME/course/client.py

**What to do**

#. Restart the :term:`ecflow_server` using the CLI :term:`ecflow_client`.
#. Begin the :code:`test` :term:`suite` using the CLI :term:`ecflow_client`.
#. (Optiona) Use the provided Python script to restart the :term:`ecflow_server` and begin the :term:`suite`
