.. index::
   single: event (tutorial)
   
.. _tutorial-add-event:

Adding an event
===============

If a task produces several results, it may be useful to start other tasks as soon as the first results are ready.
For that, ecFlow introduces the concept of :term:`event`\ s.

An :term:`event` is an indication that a task has reached a certain point in its execution.
The task informs the :term:`ecflow_server` of this by setting the event, at any point during task execution.
Each event has a name, and a task may have several events.

Update Task Script
------------------

Create new tasks :code:`t3` and :code:`t4` that will be triggered by the events emitted in task `:code:`t2`.
Create the :term:`task scripts <ecf script>` for task :code:`t3` and task :code:`t4` by copying the script from :code:`t1`.
    
To notify :term:`ecflow_server`, the task :code:`t2` uses the :term:`ecflow_client` with the :code:`--event` option (this is one of the :term:`child commands <child command>`).

.. code-block:: shell
   :caption: Amend $HOME/course/test/f1/t2.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   ecflow_client --event a       # Set the first event 
   sleep %SLEEP%                 # Sleep a bit more 
   ecflow_client --event b       # Set the second event 
   sleep %SLEEP%                 # A last nap... 
   %include <tail.h>

Update Suite Definition
-----------------------

Update the suite definition to add the events and the new tasks.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

            # Definition of the suite test.
            suite test
              edit ECF_INCLUDE "{{HOME}}/course" # replace '{{HOME}}' appropriately
              edit ECF_HOME    "{{HOME}}/course"
              family f1
                edit SLEEP 20
                task t1
                task t2
                  trigger t1 eq complete
                  event a
                  event b
                task t3
                  trigger t2:a
                task t4
                  trigger t2:b
              endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/add-an-event.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Create the new task scripts :file:`t3.ecf` and :file:`t4.ecf`, as described above.
#. Modify the task script :file:`t2.ecf` to set the events.
#. Modify the suite definition to add the events and the new tasks.
#. Replace the :term:`suite`, using:

   .. tabs::

      .. tab:: Text

         .. code-block:: shell

            ecflow_client --suspend /test
            ecflow_client --replace /test test.def

      .. tab:: Python

         .. code-block:: shell

            python3 test.py
            python3 client.py

#. Observe the task execution in :term:`ecflow_ui`.
#. Inspect the triggers by selecting node :code:`t2` or :code:`t3` and then the Triggers tab
