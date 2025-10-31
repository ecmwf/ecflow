.. index::
   single: limit (tutorial)
   single: inlimit (tutorial)

.. _tutorial-limits:

Limit
=====

Limits provide simple load management by limiting the number of tasks submitted by a specific :term:`ecflow_server`
      
At ECMWF, suite designers tend to use :term:`triggers <trigger>` in two different ways: to represent data dependency, or as *courtesy* triggers (i.e. a means to manage resources).
Triggers where originally designed to represent data dependency, but can artificially prevent too many jobs from executing at once and thus be used to manage queues.

The use of triggers to manage queues, although possible, is undesired as it results in suites difficult to maintain.

The concept of :term:`limit` was introduced as better and first class alternative to manage limited resources. Limits are declared with the :code:`limit` keyword.

.. _tutorial-inlimit:

InLimit
-------

Limits are used in conjunction with :term:`inlimit`, which defines a need to consider the referred :term:`limit`.

A :term:`limit` must be defined using :code:`limit NAME N` -- the limit definition is typically placed at the :term:`suite` scope.
Then, a limit can be imposed on a group of tasks by attaching :code:`inlimit NAME` attribute to the restricted nodes.
Attaching the attribute to a :term:`task` adds the task to the group. Attaching it to a :term:`family` adds all tasks from that :term:`family`.

The effect of a :term:`limit` is that no more than :code:`N` tasks of a group will run at once.

A :term:`node` can be limited by several limits.


Suite Definition
----------------

.. tabs::

    .. tab:: Text

        Create :term:`family` f5 with nine tasks, modifying the :term:`suite definition` file, as follows:

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

    .. tab:: Python

        .. literalinclude:: src/limits.py
           :language: python
           :caption: $HOME/course/test.py

Task script
-----------

Create new :term:`task scripts <ecf script>` in :file:`$HOME/course/test/f5/` directory, each one containing:

.. code-block:: bash
   :caption: $HOME/course/test/f5/t1.ecf,t2.ecf.....t9.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   %include <tail.h>

**What to do**

#. Apply the changes to :term:`suite definition`.
#. Apply the changes to :term:`task script <ecf script>`.
#. In :term:`ecflow_ui`

   * Observe the triggers of the :term:`limit` :code:`l1`
   * Open the Info panel for :code:`l1`
   * Change the value of the :term:`limit`
   * Open the *Why?* panel for one of the :term:`queued` tasks of :code:`/test/f5`
