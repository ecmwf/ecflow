.. _add_suite_family_task:

Add suite, family and task
////////////////////////////////////////////////////////////////////////////////////////

The Defs, Suite, Family, and Task form a node hierarchy: Every Suite, Family, and Task must have a name. This name must be unique between the peers.

The following example shows different ways of adding node hierarchy(they produce identical definitions):

.. code-block:: python
   :caption: Old style

   import ecflow

   defs = ecflow.Defs()
   s = ecflow.Suite("s1")
   f = ecflow.Family("f1")
   t = ecflow.Task("t1")
   defs.add_suite(s)
   s.add_family(f)
   f.add_task(t)


.. code-block:: python
   :caption: Functional style

   from ecflow import *

   defs = Defs()
   defs.add_suite('s1').add_family('f1').add_task('t1')

.. code-block:: python
   :caption: Constructor(Preferred)

   from ecflow import *

   defs = Defs(
      Suite("s1", 
         Family("f1", 
            Task("t1"))))


.. code-block:: python
   :caption: Using add

   from ecflow import *

   defs = Defs().add(
      Suite("s1").add(
         Family("f1").add(
            Task("t1"))))


.. code-block:: python
   :caption: Using + with parenthesis

   from ecflow import *

   defs = Defs() + (Suite("s1") + (Family("f1") + Task("t1")))

.. code-block:: python
   :caption: Hybrid. mix and match. Uses += and + 

   from ecflow import *

   defs = Defs(Suite("s1"))
   defs.s1 += Family("f1") + Task("t1")


The following example shows how a group of tasks could be created in a Python definition file.

.. code-block:: python

   import ecflow

   if _name_ == "_main_":
      defs = ecFlow.Defs()  # create an empty definition
      suite = defs.add_suite("s1")
      # create a suite and add it to the defs
      family = suite.add_family("f1")  # create a family and add it to suite
      for i in ["a", "b", "c"]:  # create task ta,tb,tc
         family.add_task("t" + i)  # create a task and add to family
   defs.save_as_defs("test.def")  # save defs to file "test.def"


The following examples show alternative styles of adding suites, families, and tasks: They produce exactly the same suite as above.

.. code-block:: python
      
   from ecflow import *
   
   defs = Defs(
         Suite("s1",
            Family("f1",
               [Task("t{}".format(t)) for t in ("a", "b", "c")])))
   defs.save_as_defs("test.def")

.. code-block:: python
         
   from ecflow import *

   defs = Defs().add(
            Suite("s1").add(
               Family("f1").add(
                  [Task("t{}".format(t))
                  for t in ("a", "b", "c")])))    
   defs.save_as_defs("test.def")


.. code-block:: python
            
   from ecflow import *

   defs = Defs()
   defs += Suite("s1")
   defs.s1 += Family("f1")
   defs.s1.f1 += [Task("t{}".format(t))
                  for t in ("a", "b", "c")]
   defs.save_as_defs("test.def")

.. warning::

   In the third example above we use 'defs.s1.f1' to reference a node by name. This is useful in small designs but will produce maintenance issues in large designs **IF** the node names are changed.

The following example adds 5 suites, with 5 families with 5 tasks.
However, care needs to be taken, to ensure that python is readable. It
is recommended that you check your results.

.. code-block:: python

   from ecflow import *

   defs = Defs(
      [
         Suite(
               "s{0}".format(i),
               [
                  Family("f{0}".format(i), [Task("t{0}".format(i)) for i in range(1, 6)])
                  for i in range(1, 6)
               ],
         )
         for i in range(1, 6)
      ]
   )
   assert (len(defs) == 5, " expected 5 suites but found " + str(len(defs)))
   for suite in defs:
      assert (len(suite) == 5, " expected 5 families but found " + str(len(suite)))
      for fam in suite:
         assert (len(fam) == 5, " expected 5 tasks but found " + str(len(fam)))
