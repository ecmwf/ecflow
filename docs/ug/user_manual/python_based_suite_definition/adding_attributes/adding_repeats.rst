.. _adding_repeats:

Adding Repeats
//////////////////

.. code-block:: python

   from ecflow import (
      Defs,
      Suite,
      Family,
      Task,
      RepeatDay,
      RepeatEnumerated,
      RepeatString,
      RepeatInteger,
      RepeatDateList,
   )

   def add_tasks(fam):
      for i in range(1, 3):
         fam.add_task(Task("t{}".format(i)))

   defs = Defs()
   s1 = defs.add_suite("s1")
   f1 = s1.add_family("f1")
   f1.add_repeat(RepeatDate("YMD", 20100111, 20100115, 2))
   add_tasks(f1)

   f2 = s1.add_family("f2")
   f2.add_repeat(RepeatInteger("count", 0, 100, 2))
   add_tasks(f2)

   f3 = s1.add_family("f3")
   f3.add_repeat(RepeatEnumerated("enum", ["red", "green", "blue"]))
   add_tasks(f3)

   f4 = s1.add_family("f4")
   f4.add_repeat(RepeatString("enum", ["a", "b", "c"]))
   add_tasks(f4)

   f5 = s1.add_family("f5")
   f5.add_repeat(RepeatDay(1))
   add_tasks(f5)

   f6 = s1.add_family("f6")
   f6.add_repeat(
      RepeatDateList("YMD", [20130101, 20130102, 20130103])
   )  # arbitary date list
   add_tasks(f6)

The following examples show alternative styles that produces the same definition:

.. code-block:: python

   defs = Defs(
      Suite(
         "s1",
         Family(
               "f1",
               RepeatDate("YMD", 20100111, 20100115, 2),
               [Task("t{}".format(i)) for i in range(1, 3)],
         ),
         Family(
               "f2",
               RepeatInteger("count", 0, 100, 2),
               [Task("t{}".format(i)) for i in range(1, 3)],
         ),
         Family(
               "f3",
               RepeatEnumerated("enum", ["red", "green", "blue"]),
               [Task("t{}".format(i)) for i in range(1, 3)],
         ),
         Family(
               "f4",
               RepeatString("enum", ["a", "b", "c"]),
               [Task("t{}".format(i)) for i in range(1, 3)],
         ),
         Family("f5", RepeatDay(1), [Task("t{}".format(i)) for i in range(1, 3)]),
         Family(
               "f6",
               RepeatDateList("YMD", [20130101, 20130102, 20130103]),
               [Task("t{}".format(i)) for i in range(1, 3)],
         ),
      )
   )

.. code-block:: python

   defs = Defs() + Suite("s1")
   defs.s1 += [
      Family("f{}".format(i)).add([Task("t{}".format(i)) for i in range(1, 3)])
      for i in range(1, 6)
   ]
   defs.s1.f1 += RepeatDate("YMD", 20100111, 20100115, 2)
   defs.s1.f2 += RepeatInteger("count", 0, 100, 2)
   defs.s1.f3 += RepeatEnumerated("enum", ["red", "green", "blue"])
   defs.s1.f4 += RepeatString("enum", ["a", "b", "c"])
   defs.s1.f5 += RepeatDay(1)
   defs.s1.f6 += RepeatDateList("YMD", [20130101, 20130102, 20130103])

.. warning::

   In the second example above we use 'defs.s1.*' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs IF the node names are changed.

