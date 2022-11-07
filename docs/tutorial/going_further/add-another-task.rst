.. index::
   single: Add another task (tutorial)
   module: os
   module: ecflow

.. _tutorial-add-task:

Add another task
================

Let's add another :term:`task` named **t2**.  You will need to modify the :term:`suite definition` file and add a new script. 

You must also create a file :file:`t2.ecf` in :file:`$HOME/course/test`. Simply copy :file:`t1.ecf`.

First modify the :term:`suite definition`

Text
----

It is good practice to suspend your suite before you reload any part of it. In :ref:`ecflow_ui` right click on the suite and select "Suspend". Once you made your change(specified below) you can right click on the suite and "Resume" it.

.. code-block:: shell

   # Definition of the suite test
   suite test
      edit ECF_HOME "$HOME/course"  # replace '$HOME' with the path to your home directory
      task t1
      task t2 
   endsuite

.. note::

   As before replace $HOME with the real path to your home directory.


Then you must load the file again:

.. code-block:: shell

   ecflow_client --load test.def
   
.. warning::

   This will fail because the suite is already loaded.

Because the suite is already defined, you need to delete and reload it first:

.. code-block:: shell

   ecflow_client --delete=_all_
   ecflow_client --load=test.def

Then restart the suite:

.. code-block:: shell

   ecflow_client --begin=test 

Rather than deleting, loading and beginning the suite every time you can **replace** all or part of the suite for example to replace whole suite.

.. code-block:: shell

   ecflow_client --replace /test test.def

or to replace part of the suite:

.. code-block:: shell

   ecflow_client --replace /test/t2 test.def

Python
------

To delete the suite definition, reload and begin using the :ref:`python_api`:
First update :file:`test.py`

.. literalinclude:: src/add-another-task.py
   :language: python
   :caption: $HOME/course/test.py

To delete all suites in the server and reload modified :file:`test.def`, we could update :file:`client.py`

.. literalinclude:: src/add-another-task-client.py
   :language: python
   :caption: $HOME/course/client.py

Rather than deleting, loading and beginning the suite every time you can **replace** all or part of the suite. (i.e. to replace the whole suite see below) 

Additionally we do not want the suite to start straight away. This can be done by suspending the suite in :term:`ecflow_ui` before reloading. However we will need to remember to do this, each time. To get round this we will suspend the suite use the :ref:`python_api`:

Modify :file:`client.py` with:

.. literalinclude:: src/add-another-task-client_1.py
   :language: python
   :caption: $HOME/course/client.py

.. note:: 
   
   For **brevity** the examples that follow, will not show the loading of the suite.


**What to do**

#. Suspend the suite using :ref:`ecflow_ui` or via python using :py:class:`ecflow.Client.suspend` 
#. Create the new task
#. Create :file:`t2.ecf` by copying from :file:`t1.ecf`
#. Update python scripts :file:`test.py` and :file:`client.py` or test.def
#. Replace the suite. For Python use:

   | ``python3 test.py`` or ``./test.py``
   | ``python3 client.py`` or ``./client.py``

   For text use: ``ecflow_client --replace=/test test.def``

#. Resume the the suite using :ref:`ecflow_ui`
#. In :ref:`ecflow_ui`, watch the two task running. They should run at the same time
   