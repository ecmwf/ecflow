.. index::
   single: complete (tutorial)
   
.. _tutorial-add-complete:

Add a complete
==============

Sometimes you do not want to run a :term:`task` when a certain condition is met. The condition can be signalled by an event. For example, event t2:b might indicate that task t2 did not manage to produce expected result, so
we do not need to run task t4.

In this case you can use the :term:`complete expression` keyword. This has a similar syntax to the :term:`trigger` keyword but sets a :term:`task` :term:`complete` rather than running it. 

When :term:`ecflow_server` tries to start a :term:`task`, it  evaluates the :term:`trigger` and :term:`complete expression` s. If the :term:`complete expression` condition is correct, the task will set itself :term:`complete`.:term:`complete expression` evaluation takes precedence over the :term:`trigger`.

Completes can be between tasks, between families, or both. It can be used in conjunction with a :term:`trigger`. For example:

Text
----

.. code-block:: shell

   # Definition of the suite test.
   suite test
      edit ECF_INCLUDE "$HOME/course"   # replace '$HOME' with the path to your home directory
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
            trigger t2 eq complete
            complete t2:b
      endfamily
   endsuite
   
Python
------

.. literalinclude:: src/add-a-complete.py
   :language: python
   :caption: $HOME/course/test.py


**What to do:**

#. Update :file:`test.def` or :file:`test.py` to add a :term:`complete expression` to task **t4**
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. Observe the tasks in :term:`ecflow_ui`
#. See the triggers by selecting **t4**
#. See the trigger relation by clicking on the arrow
#. See the triggers in the tree, using the Show menu
#. Note the icon indicating that the task has not run
#. To check the triggers modify task **t2** so that event b is not triggered; task **t4** should run when **t2** completes. 
