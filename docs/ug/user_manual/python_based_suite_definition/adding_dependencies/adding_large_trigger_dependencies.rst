.. _adding_large_trigger_dependencies:

Adding Large Trigger Dependencies
////////////////////////////////////////////////

.. code-block:: python 

   from ecflow import Defs, Suite, Task, Trigger, Complete

   defs = Defs(
      Suite(
         "s1",
         Task("t1"),
         Task("t2"),
         Task(
               "t3",
               Trigger("t1 == complete"),
               Trigger("t2 == active"),  # added as a AND
               Trigger("t2 == aborted", False),
         ),
      )
   )  # False mean add with OR


.. code-block:: python 
   :caption: t3 TRIGGER

   (t1 == complete and t2 == active or t2 == active)

The following show alternative styles that produce the same definition:

.. code-block:: python 

   defs = Defs()
   s1 = defs.add_suite("s1")
   t1 = s1.add_task("t1")
   t2 = s1.add_task("t2")
   t3 = s1.add_task("t3")
   t3.add_part_trigger("t1 == complete")
   t3.add_part_trigger("t2 == active", True)
   # here True means add as 'AND'
   t3.add_part_trigger("t2 == aborted", False)
   # here False means add as 'OR'

.. code-block:: python 

   defs = Defs().add(
      Suite("s1").add(
         Task("t1"),
         Task("t2"),
         Task("t3").add(
               Trigger("t1 == complete"),
               Trigger("t2 == active"),
               Trigger("t2 == aborted", False),
         ),
      )
   )


.. code-block:: python 

   defs = Defs() + Suite("s1")
   defs.s1 += [Task("t{}".format(i)) for i in range(1, 4)]
   defs.s1.t3 += [
      Trigger("t1 == complete"),
      Trigger("t2 == active"),
      Trigger("t2 == aborted", False),
   ]

.. warning::

   In the example above we use 'defs.s1.t3' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs if the node names are changed.
