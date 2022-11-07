.. _adding_a_late_attribute:

Adding a Late attribute
/////////////////////////

The following show alternative styles of adding a late attribute. They produce the same definition.

.. code-block:: python

  defs = Defs()
  suite = defs.add_suite("s1")

  late = Late()
  late.submitted(20, 10)  # hour,min
  late.active(2, 10)  # hour,min
  late.complete(3, 10, True)  # hour,min,relative
  suite.add_task("t1").add_late(late)

.. code-block:: python

  defs = Defs(
      Suite("s1", 
        Task("t1", 
          Late(submitted="20:10", active="02:10", complete="+03:10")))
  )

.. code-block:: python

  defs = Defs() + (Suite("s1") + Task("t1"))
  defs.s1.t1 += Late(submitted="20:10", active="02:10", complete="+03:10")
