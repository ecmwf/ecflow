.. _adding_defstatus_autocancel:

Adding Defstatus, Autocancel
//////////////////////////////////////////////////////////

Adding defstatus
=======================

The defstatus sets the default state of a node when the begin or re-queue commands are run.

.. code-block:: python

   from ecflow import Defs, Suite, Task, Defstatus, AutoCancel

   defs = Defs(
      Suite(
         "s1", 
            Task("t1", Defstatus("complete")), 
            Task("t2", Defstatus(DState.complete))
      )
   )


The following shows alternative styles that produce the same definition:

.. code-block:: python

   defs = Defs()
   s1 = defs.add_suite("s1")
   s1.add_task("t1").add_defstatus(Defstatus("complete"))
   s1.add_task("t2").add_defstatus(DState.complete)

.. code-block:: python

   defs = Defs().add(
      Suite("s1").add(
         Task("t1").add(Defstatus("complete")),
         Task("t2").add(Defstatus(DState.complete)))) 

.. code-block:: python

   defs = Defs() + (Suite("s1") + Task("t1") + Task("t2"))
   defs.s1.t1 += Defstatus("complete")
   defs.s1.t2 += Defstatus(DState.complete)


Adding autocancel
====================

.. code-block:: python

   from ecflow import Defs, Suite, Task, AutoCancel, TimeSlot

   defs = Defs(
      Suite(
         "s1",
         Task("t1", Autocancel(3)),  # delete task after 3 days after completion
         Task(
               "t2", Autocancel(1, 10, True)
         ),  # delete task 1hr 10 min after task completion
         Task(
               "t3", Autocancel(TimeSlot(2, 10), True)
         ),  # delete task 2hr 10 min after task completion
         Task("t4", Autocancel(1)),  # delete task after 1 day after task completion
         Task(
               "t5", Autocancel(18, 10, False)
         ),  # delete task at 6:10pm once it has completed
         Task("t6", Autocancel(2, 10, False)),
      )
   )  # delete task at 2:10am once it has completed


The following examples show alternative styles of adding Autocancel which produce the same definition.

.. code-block:: python

   defs = Defs()
   s1 = defs.add_suite("s1")
   s1.add_task("t1").add_autocancel(3)
   s1.add_task("t2").add_autocancel(1, 10, True)
   s1.add_task("t3").add_autocancel(TimeSlot(2, 10), True)
   s1.add_task("t4").add_autocancel(Autocancel(1))
   s1.add_task("t5").add_autocancel(Autocancel(18, 10, False))
   s1.add_task("t6").add_autocancel(Autocancel(TimeSlot(2, 10), False))

.. code-block:: python

   defs = Defs().add(
      Suite("s1").add(
         Task("t1").add(Autocancel(3)),
         Task("t2").add(Autocancel(1, 10, True)),
         Task("t3").add(Autocancel(TimeSlot(2, 10), True)),
         Task("t4").add(Autocancel(1)),
         Task("t5").add(Autocancel(18, 10, False)),
         Task("t6").add(Autocancel(2, 10, False)),
      )
   )

.. code-block:: python

   defs = Defs() + (Suite("s1") + [Task("t{0}".format(i)) for i in range(1, 7)])
   defs.s1.t1 += Autocancel(3)
   defs.s1.t2 += Autocancel(1, 10, True)
   defs.s1.t3 += Autocancel(TimeSlot(2, 10), True)
   defs.s1.t4 += Autocancel(1)
   defs.s1.t5 += Autocancel(18, 10, False)
   defs.s1.t6 += Autocancel(2, 10, False)

.. warning::

   In the example above we use 'defs.s1.t1' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs if the node names are changed.
