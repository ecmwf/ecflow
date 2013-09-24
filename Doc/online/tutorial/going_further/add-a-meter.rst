.. index::
   single: add a meter
   
.. _add-meter:

Add a meter
===========

| A :term:`meter` is very similar to an :term:`event`. 
| Instead of being a boolean value (on/off), it can take a range of integer values. 
| Other tasks are then triggered when the :term:`meter` reaches a certain value. 
| Like events, :term:`meter`'s have names and a :term:`task` can have several of them.

Ecf Script
----------

| We will create new tasks (t5, t6 and t7) that will be triggered
| when the :term:`meter` in task t1 reaches certain values.
    
| To notify :term:`ecflow_server`, the :term:`task` ( **t1** in the example below) must call 
| the :term:`ecflow_client` --meter. This is also one of the :term:`child command`'s.

::

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   n=1 
   while [[ $n -le 100 ]]            # Loop 100 times 
   do 
      sleep 1                        # Wait a short time 
      ecflow_client --meter progress $n # Notify ecFlow 
      (( n = $n + 1 )) 
   done 
   %include <tail.h>
   
Text
----

::

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"    # replace '$HOME' with the path to your home directory
    edit ECF_HOME    "$HOME/course"
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
   
Python
------

.. literalinclude:: src/add-a-meter.py


**What to do:**

#. Edit the definition file or python to add the modifications.
#. Edit :file:`t1.ecf` to call :term:`ecflow_client` --meter
#. Copy :file:`t4.ecf` to :file:`t5.ecf`, :file:`t6.ecf` and :file:`t7.ecf`
#. Replace the :term:`suite`
#. Observe the tasks in :term:`ecflowview`.
#. See the triggers by selecting progress and clicking on the Triggers icon.
#. Move the mouse pointer over |progress| then with the right mouse button, choose **Edit...**
#. Modify the value of the :term:`meter` and click on the Apply icon |apply|

.. |progress| image:: progress.jpg
.. |apply| image:: apply.jpg
   