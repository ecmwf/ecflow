.. _error_handling:

Error Handling
////////////////

Any errors in the creation of suite are handled by throwing an exception.

.. code-block:: python

  try:
      defs = ecflow.Defs()  # create a empty definition
      s1 = defs.add_suite("s1")  # create a suite "s1" and add to defs
      s2 = defs.add_suite("s1")  # Exception thrown trying to add suite "s1" again
  except RuntimeError as e:
      print(e)
