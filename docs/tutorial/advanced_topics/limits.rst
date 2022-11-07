.. index::
   single: limit (tutorial)
   single: inlimit (tutorial)

.. _tutorial-limits:

Limits
======

Limits provide simple load management by limiting the number of tasks submitted by a specific :term:`ecflow_server`
      
We have learnt from experience that suite designers were using :term:`trigger`'s in two different ways: as data dependency triggers and as courtesy triggers. Triggers where designed for the former. The latter are used to prevent too many jobs running at once and are actually an artificial way of queueing jobs. 

Because ecFlow does not distinguish between the two sorts of triggers, suites can become difficult to maintain after a while. So the concept of :term:`limit` was introduced. Limits are declared with the **limit** keyword 

.. _tutorial-inlimit:

inlimit
-------

Limits are used in conjunction with :term:`inlimit` keyword.

First, a :term:`limit` must be defined using the 'limit NAME N'. The limit definition is typically placed at the :term:`suite` scope.

Next we create a group of tasks to which we want to apply the limit. This is done by attaching an 'inlimit NAME' attribute to the nodes. Attaching the attribute to a :term:`task` adds the task to the group. Attaching it to a :term:`family` adds all tasks from that :term:`family`.

The effect of a :term:`limit` is that no more than N tasks of a group will run at once.

A :term:`node` can be limited by several limits.

Ecf script
----------

We will create :term:`family` f5 with nine tasks. Create new :term:`ecf script` s in :file:`$HOME/course/test/f5/` directory, each one containing:

.. code-block:: bash
   :caption: $HOME/course/test/f5/t1.ecf,t2.ecf.....t9.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   %include <tail.h>

Text
----

Let us modify our :term:`suite definition` file:

.. code-block:: shell

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    limit l1 2

    family f5
        inlimit l1
        edit SLEEP 20
        task t1
        task t2
        task t3
        task t4
        task t5
        task t6
        task t7
        task t8
        task t9
    endfamily
   endsuite


Python
------

.. literalinclude:: src/limits.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Edit the changes
#. Replace the :term:`suite definition`
#. In :term:`ecflow_ui`, observe the triggers of the :term:`limit` **l1**
#. Open the Info panel for **l1**
#. Change the value of the :term:`limit`
#. Open the Why? panel for one of the :term:`queued` tasks of **/test/f5**
#. Introduce an error in the limits and make sure this error is trapped. i.e. change the Limit.

   .. code-block:: python
      :caption: Check  InLimit/Limit references

      Limit("unknown",2)