.. index::
   single: trigger (tutorial)

.. _tutorial-add-trigger:  
   
Adding a trigger
================

Running tasts sequencially is a fundamental ability of ecFlow.
This task sequencing can be enforced by using a :term:`trigger`.
Triggers are used to declare :term:`dependencies` between two tasks e.g. a task needs data created by another task.

For example, when task :code:`t2` processes data produced by task :code:`t1`,
a :term:`trigger` can be used to enforce this dependency.

When ecFlow tries to start a task, the :term:`trigger` expression is evaluated,
and if the condition evaluates to "true" the task is started, otherwise the task remains :term:`queued`.

Triggers can be specified between tasks, between families, or any mixture of these. Remember the two rules:

* A family is :term:`complete` when all its tasks are :term:`complete`.
* A task will be started if its triggers and the triggers of all is parent families evaluate to true.

Each :term:`node` can only have one trigger expression, but very complex expressions can be built
(and keep in mind that the triggers of the parent nodes are also implicit triggers).

Sometimes triggers can also be used to prevent too many jobs from running at the same time.
For these cases, making use of a :term:`limit` (covered later :ref:`tutorial-limits` section) tends be a better solution.

Trigger expressions can refer to nodes using full names or, in some contexts, relative names (such as :code:`../t1`).
Considering the ongoing example,

#. :code:`/test/f1/t1` refers to the :term:`task` :code:`t1`.
#. :code:`/test/f1` refers to the :term:`family` :code:`f1`.

An example of a simple trigger expression is:

.. code-block:: shell

   trigger /test/f1/t1 == complete

Triggers expressions can be very complex, as ecFlow supports all kinds of conditions (not, and, or, ...).
In addition, trigger expressions can also reference Node attributes like :term:`event`, :term:`meter`, :term:`variable`,
:term:`repeat` and generated variables.

Update Suite Definition
-----------------------

Consider the following :term:`suite definition`, with a :term:`trigger` added to task **t2** to ensure that it only runs once **t1** is complete.

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
              endfamily
           endsuite

    .. tab:: Python

        The trigger expression can be checked, this is **especially** important when dealing with very large suites and **complex** triggers.

        .. literalinclude:: src/add-trigger.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Modify the suite definition to include the trigger, as shown above.
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

#. Inspect the tasks in :term:`ecflow_ui`.
#. Observe the triggers by selecting task :code:`t2`.
#. Observe the trigger relation by opening the Trigger tab.
#. Search any reference to :code:`t1` by using the search menu.
#. (Optional) Using the Python API, introduce an error in the trigger expression
   and observe that this error is detected. For example, change the trigger to:

   .. code-block:: shell

      Trigger("t == complete")  # Error: no node with name `t`
