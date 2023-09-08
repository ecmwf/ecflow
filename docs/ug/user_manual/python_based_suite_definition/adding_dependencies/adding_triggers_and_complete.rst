.. _adding_triggers_and_complete:

Adding Triggers and Complete
////////////////////////////

Triggers define a dependency on a :term:`task` or :term:`family`.
There can be only one trigger dependency per :term:`node`, but that
can be a complex boolean expression of the :term:`status` of several nodes.
Triggers can not be added to the suite node. A node with a trigger can only
be activated when its trigger has expired. A trigger holds the node as long
as the trigger’s expression evaluation returns false.

Trigger evaluation occurs whenever the child :term:`task` communicates with the
server i.e. whenever there is a state change in the suite definition and at
least once every 60 seconds. The keywords in trigger expressions are:
:term:`unknown`, :term:`suspended`, :term:`complete`, :term:`queued`,
:term:`submitted`, :term:`active`, :term:`aborted` and **clear** and **set**
for :term:`event` status. Triggers can also reference Node attributes like
:term:`event`, :term:`meter`, :term:`variable`, :term:`repeat` and generated
variables and :term:`limits<limit>`.
Triggers can also reference the late flag on a node. Trigger evaluation
for node attributes uses integer arithmetic.
See the :ref:`Glossary` for more details.

.. code-block:: python 

   from ecflow import Defs, Suite, Task, Trigger, Complete

   defs = Defs(
      Suite(
         "s1",
         Task("t1"),
         Task(
               "t2", Trigger("t1 == active and t3 == aborted"), Complete("t3 == complete")
         ),
         Task("t3"),
      )
   )

The following examples show alternative styles that produce the same
definition:

.. code-block:: python 

   defs = Defs()
   s1 = defs.add_suite("s1")
   t1 = s1.add_task("t1")
   t2 = s1.add_task("t2")
   t2.add_trigger("t1 == active and t3 == aborted")
   t2.add_complete("t3 == complete")
   t3 = s1.add_task("t3")

.. code-block:: python 

   defs = Defs().add(
      Suite("s1").add(
         Task("t1"),
         Task("t2").add(
               Trigger("t1 == active and t3 == aborted"), Complete("t3 == complete")
         ),
         Task("t3"),
      )
   )


.. code-block:: python 

   defs = Defs(Suite("s1"))
   defs.s1 += [Task("t{}".format(i)) for i in range(1, 4)]
   defs.s1.t2 += [Trigger("t1 == active and t3 == aborted"), Complete("t3 == complete")]

Adding triggers like '<nodepath> == complete' is extremely common. Hence
there are a few short cuts;


.. code-block:: python 
   :caption: Short cut for <node> == complete

   task = Task("task")
   # Using a trigger with a 'list' argument, each string/node element converted to <name> == complete
   t = Trigger(["a","b",task])  # because Task("task") does *NOT* have a parent, we will use the name
   assert str(t) == "a == complete AND b == complete AND task == complete","Trigger not as expected: " + str(t))
   
   defs = Defs()
   task = defs.add_suite("s").add_family("f").add_task("task")
   t = Trigger(["a","b",task])      # Task('task') has a parent hierarchy, hence we use full path in trigger expression
   assert str(t) == "a == complete AND b == complete AND /s/f/task == complete", "Trigger not as expected: " + str(t))


Chaining Tasks
==============

There are many times where we want to add a chain of tasks, i.e. where
tasks must be run sequentially one after the other. The following
examples show different styles of chaining tasks which are identical:

.. code-block:: python 

   from ecflow import *

   defs = Defs(
      Suite(
         "s1",
         Task("t1"),
         Task("t2", Trigger("t1 == complete")),
         Task("t3", Trigger("t2 == complete")),
         Task("t4", Trigger("t3 == complete")),
      )
   )


.. code-block:: python 

   from ecflow import *

   defs = Defs() + Suite("s1")
   defs.s1 += [Task("t1"), Task("t2"), Task("t3"), Task("t4")]
   defs.s1.t2 += Trigger(["t1"])
   defs.s1.t3 += Trigger(["t2"])
   defs.s1.t4 += Trigger(["t3"])
   
.. code-block:: python 

   from ecflow import *

   defs = Defs() + Suite("s1")
   defs.s1 >> Task("t1") >> Task("t2") >> Task("t3") >> Task("t4")
   # >> relies on the leading node to be a Family or Suite

Reverse Chaining
================

It is also possible to << to reverse chain task.

The following suites are identical:

.. code-block:: python 

   defs = Defs() + Suite("s1")
   defs.s1 += [Task("t1"), Task("t2"), Task("t3"), Task("t4")]
   defs.s1.t1 += Trigger("t2 == complete")
   defs.s1.t2 += Trigger("t3 == complete")
   defs.s1.t3 += Trigger("t4 == complete")

.. code-block:: python 

   defs = Defs() + Suite("s1")
   defs.s1 << Task("t1") << Task("t2") << Task("t3") << Task("t4")
   # << relies on the leading node to be a Suite or Family

.. warning::

   In the examples above we use 'defs.s1.*' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs IF the node names are changed.
