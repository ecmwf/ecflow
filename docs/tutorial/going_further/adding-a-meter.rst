.. index::
   single: meter (tutorial)
   
.. _tutorial-add-meter:

Adding a meter
==============

A :term:`meter` is very similar to an :term:`event`, but instead of a *boolean* value (*true*/*false*) it can take a range of integer values.
Other tasks can be triggered when the :term:`meter` reaches a certain value. Like an :term:`event`, a :term:`meter` has a name and a :term:`task` can have several of them.

Update Task Script
------------------

Create new tasks :code:`t5`, :code:`t6` and :code:`t7` that will be triggered when the :term:`meter` in task :code:`t1` reaches certain values.
    
To notify :term:`ecflow_server`, update the :term:`task` :code:`t1` to use the :term:`ecflow_client` with the :code:`--meter` option (this is one of the :term:`child commands <child command>`).

.. code-block:: shell
   :caption: $HOME/course/f1/t1.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   n=1 
   while [[ $n -le 100 ]]            # Loop 100 times 
   do 
      sleep 1                        # Wait a short time 
      ecflow_client --meter=progress $n # Notify ecFlow 
      (( n = $n + 1 )) 
   done 
   %include <tail.h>

Update Suite Definition
-----------------------

Update the suite definition to add the :term:`meter` to task :code:`t1` and the new tasks :code:`t5`, :code:`t6` and :code:`t7`.

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
                     meter progress 1 100 90
                 task t2
                     trigger t1 eq complete
                     event a
                     event b
                 task t3
                     trigger t2:a
                 task t4
                     trigger t2 eq complete
                     complete t2:b
                 task t5
                     trigger t1:progress ge 30
                 task t6
                     trigger t1:progress ge 60
                 task t7
                     trigger t1:progress ge 90
             endfamily
            endsuite
   
   .. tab:: Python

      .. literalinclude:: src/add-a-meter.py
         :language: python
         :caption: $HOME/course/test.py


**What to do:**

#. Create the new task scripts, as described above.
#. Modify the task script :file:`t1.ecf` to use the :code:`--meter` option, as shown above.
#. Modify the suite definition, as shown above.
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

#. Observe the tasks in :term:`ecflow_ui`.
#. Inspect the meter as it changes.
#. Inspect the triggers by selecting progress and clicking on the Triggers icon.
#. Modify the value of the :term:`meter` using the context menu in the progress icon.
