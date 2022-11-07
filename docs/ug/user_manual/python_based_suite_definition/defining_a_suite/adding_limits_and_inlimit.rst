.. _adding_limits_and_inlimits:

Adding Limits and Inlimit
////////////////////////////////////////////////

.. code-block:: python
   :caption: Add Limits/Inlimits

   from ecflow import Defs, Suite, Task, Limit, InLimit

   defs = Defs(
      Suite(
         "s1",
         Limit("limitName4", 10),  # name, maximum token
         Family(
               "f1",
               InLimit(
                  "limitName4", "/s1/f1", 2
               ),  # limit name, path to limit, tokens consumed from the Limit
               [Task("t{}".format(t)) for t in range(1, 4)],
         ),
      )
   )


The following show alternative styles that produce the same definition.

.. code-block:: python

   defs = Defs()
   s1 = defs.add_suite("s1")
   s1.add_limit("limitName4", 10)
   f1 = s1.add_family("f1")
   f1.add_inlimit("limitName4", "/s1/f1", 2)
   for i in range(1, 4):
      f1.add_task("t{}".format(i))

.. code-block:: python

   defs = Defs() + Suite("s1")
   defs.s1 += [Limit("limitName4", 10), Family("f1")]
   defs.s1.f1 += [
      InLimit("limitName4", "/s1/f1", 2),
      [Task("t{}".format(t)) for t in range(1, 4)],
   ]

.. code-block:: python

   with Defs() as defs:
      with defs.add_suite("s1") as s1:
         s1.add_limit("limitName4", 10)
         with s1.add_family("f1") as f1:
               f1.add_inlimit("limitName4", "/s1/f1", 2)
               f1 += [Task("t{}".format(t)) for t in range(1, 4)]

.. warning::

   In the second example above we use 'defs.s1.f1' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs IF the node names are changed.

