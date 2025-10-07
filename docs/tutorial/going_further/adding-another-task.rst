.. index::
   single: Add another task (tutorial)
   module: os
   module: ecflow

.. _tutorial-add-task:

Adding another task
===================

To add another task to the existing :term:`suite` :code:`test`, implies the following steps:

#. Modify the :term:`suite definition` file to include the new task.
#. Create a new :term:`ecf script` for the new task.
#. Reload and begin the modified :term:`suite definition` into the :term:`ecflow_server`.

The following shows the two methods of reloading the modified :term:`suite definition` into
the :term:`ecflow_server`. The options presented are: manually updating the text definition, and
loading it via the CLI :term:`ecflow_client`; or, to use the :ref:`Python API <python_api>` to
programmatically update and load the :term:`suite definition`.

.. tabs::

    .. tab:: Text

        Text
        ----

        It is good practice to suspend a suite before starting to update any part of it.

        This avoids the suite automatically starting before the complete set of changes is complete.

        Use the :ref:`ecflow_ui` to suspend the :code:`test` suite, or run the following command
        to do the same using the CLI :term:`ecflow_client`:

        .. code-block:: shell

           ecflow_client --suspend /test

        Update the :term:`suite definition` file to add the new task :code:`t2`.

        .. code-block:: shell

           # Definition of the suite test
           suite test
              edit ECF_HOME "{{HOME}}/course"
              task t1
              task t2
           endsuite

        .. note::

           As before, replace :code:`{{HOME}}/course` to point at the tutorial directory.

        Load the modified :term:`suite definition` into the :term:`ecflow_server` using the CLI :term:`ecflow_client`:

        .. code-block:: shell

           # Ensure suite 'test' doesn't exist by deleting all suites
           ecflow_client --delete _all_

           # Load the modified suite definition
           ecflow_client --load=test.def

           # or, equivalently, replace the suite if it already exists
           ecflow_client --replace /test test.def     # replace the whole suite

        After loading the updated :term:`suite definition`, begin the suite:

        .. code-block:: shell

           # Begin the suite
           ecflow_client --begin test

    .. tab:: Python

        The following script updates on the previous :file:`test.py` to add the new task :code:`t2`:

        .. literalinclude:: src/add-another-task.py
           :language: python
           :caption: $HOME/course/test.py

        The following script deletes all suites in the server and reload modified :file:`test.def`, we could update :file:`client.py`

        .. literalinclude:: src/add-another-task-client.py
           :language: python
           :caption: $HOME/course/client.py

        An alternative to deleting, loading and beginning the suite every time, the following script replaces the existing
        suite with the modified one. Notice that to avoid starting the suite straight away, the script suspends the suite
        using the :ref:`Python API <python_api>`.

        .. literalinclude:: src/add-another-task-client_1.py
           :language: python
           :caption: $HOME/course/client.py

        .. note::

           For the sake of brevity the examples that follow will not show the step of loading the suite.

**What to do**

#. Suspend the :code:`test` suite, either using the :ref:`ecflow_ui` or the CLI :term:`ecflow_client`.
#. Update the :term:`suite definition` file by adding a new task :code:`t2`.
#. Create :term:`ecf script` file, named :file:`t2.ecf` by copying :file:`t1.ecf`.
#. (Optional) Update the Python scripts :file:`test.py` and :file:`client.py` (as shown above).
#. Replace the :code:`test` suite using the CLI :term:`ecflow_client`.
#. Resume the the suite using :ref:`ecflow_ui` or the CLI :term:`ecflow_client`.
#. Observe the parallel execution of the tasks in :ref:`ecflow_ui`.
