.. index::
   single: complete (tutorial)
   
.. _tutorial-add-complete:

Adding a complete
=================

There are cases where a task does not need to run, because a certain condition has been met.
Considering that the condition can be signalled by an event, ecFlow provides the :term:`complete expression` to handle this case.
From the previous example, suppose that event :code:`t2:b` indicates that task :code:`t2` did not manage to produce an expected result, and in this case there is no need to run task :code:`t4`.

The :term:`complete expression` keyword has a similar syntax to the :term:`trigger` keyword but sets a :term:`task` :term:`complete` rather than running it.

When :term:`ecflow_server` tries to start a :term:`task`, it  evaluates the :term:`trigger` and the :term:`complete expression`.
If the :term:`complete <complete expression>` condition evaluates to _true_, the task will be set as :term:`complete`.
This means that the :term:`complete <complete expression>` evaluation takes precedence over the :term:`trigger` evaluation.

A :term:`complete` can be defined between tasks, between families, or both. A :term:`complete` can be used in conjunction with a :term:`trigger`, as follows.

Update Suite Definition
-----------------------

Update the suite definition to add a :term:`complete expression` to task :code:`t4`.

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
                    trigger t2 eq complete
                    complete t2:b
              endfamily
           endsuite
   
    .. tab:: Python

        .. literalinclude:: src/add-a-complete.py
           :language: python
           :caption: $HOME/course/test.py

**What to do:**

#. Update the suite definition, as shown above.
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
#. See the triggers by selecting :code:`t4`.
#. See the trigger relation by clicking on the arrow.
#. See the triggers in the tree, using the Show menu.
#. Note the icon indicating that the task has not run.
#. To check the triggers modify task :code:`t2` so that event :code:`b` is not triggered; task :code:`t4` should run when :code:`t2` completes.
