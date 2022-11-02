.. _control_structures_and_looping:

Control Structures and Looping
////////////////////////////////

In Python there is not a switch/case statement, however, this can be worked around using nested if..elif..else commands.

.. code-block:: python

  var = "aa"
  if var in ("a", "aa", "aaa"):
      print("it is a kind of a")
  elif var in ("b", "bb", "bb"):
      print("it is a kind of b")
  else:
      print("it is something else")


Using for loops:

.. code-block:: python

  suite = ecflow.Suite("x")
  previous = 0
  for i in (0, 6, 12, 18, 24):
      fam = suite.add_family(str(i))
      if i != 0:
          fam += Trigger("./" + previous + " == complete ")
          fam += Task("t1")
          fam += Task("t2", Trigger("t1 == complete"))
          previous = str(i)
