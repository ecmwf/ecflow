.. index::
   single: add an event
   
.. _add-event:

Add an event
============

| Sometimes waiting for the completion of a task is not good enough. 
| If a task is producing several results, another task may start as 
| soon as the first results are ready. 
| For that, ecFlow introduces the concept of :term:`event`'s. 

| An :term:`event` is a message that a task will report to :term:`ecflow_server` while 
| it is running. 

Events have names and a :term:`task` can set several of them

Ecf Script
----------

| We will create new tasks (t3, t4) that will be triggered by the  events emitted in task t2. 
| Create the :term:`ecf script` for t3 and t4 by copying t1.
    
| To notify :term:`ecflow_server`, the task (**t2** in the example below)
| must call :term:`ecflow_client` --event which is one of the :term:`child command`'s

::

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   ecflow_client --event a       # Set the first event 
   sleep %SLEEP%                 # Sleep a bit more 
   ecflow_client --event b       # Set the second event 
   sleep %SLEEP%                 # A last nap... 
   %include <tail.h>
   
Text
----

::

   # Definition of the suite test.
   suite test
      edit ECF_INCLUDE "$HOME/course"  # replace '$HOME' with the path to your home directory
      edit ECF_HOME    "$HOME/course"
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

Python
------

.. literalinclude:: src/add-an-event.py


**What to do:**

#. Update :file:`test.def` or :file:`test.py`
#. Edit :file:`t2.ecf` to call :term:`ecflow_client` --event.
#. Copy :file:`t1.ecf` to :file:`t3.ecf` and :file:`t4.ecf`
#. Replace the :term:`suite`
#. Observe the tasks in :term:`ecflowview`.
#. See the triggers by selecting **t3** and clicking on the icon |triggers|     
#. See the triggers by selecting **t2** and click on **Dependencies**
 
.. |triggers| image:: triggers.jpg
