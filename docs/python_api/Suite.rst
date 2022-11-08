ecflow.Suite
////////////


.. py:class:: Suite
   :module: ecflow

   Bases: :py:class:`~ecflow.NodeContainer`

A :term:`suite` is a collection of Families,Tasks,Variables, :term:`repeat` and :term:`clock` definitions

Suite is the only node that can be started using the begin API.
There are several ways of adding a suite, see example below and :py:class:`ecflow.Defs.add_suite`

Constructor::

  Suite(name, Nodes | attributes)
      string name : The Suite name. name must consist of alpha numeric characters or
                    underscore or dot. The first character cannot be a dot, as this
                    will interfere with trigger expressions. Case is significant
      Nodes | Attributes:(optional)

Exception:

- Throws a RuntimeError if the name is not valid
- Throws a RuntimeError if duplicate suite names added

Usage:

.. code-block:: python

  defs = Defs()                  # create a empty definition. Root of all Suites
  suite = Suite('suite_1')       # create a stand alone suite
  defs.add_suite(suite)          # add suite to definition
  suite2 = defs.add_suite('s2')  # create a suite and add it to the defs

  defs = Defs(
           Suite('s1',
              Family('f1',
                 Task('t1'))))   # create in in-place


.. py:method:: Suite.add_clock( (Suite)arg1, (Clock)arg2) -> Suite
   :module: ecflow


.. py:method:: Suite.add_end_clock( (Suite)arg1, (Clock)arg2) -> Suite :
   :module: ecflow

End clock, used to mark end of simulation


.. py:method:: Suite.begun( (Suite)arg1) -> bool :
   :module: ecflow

Returns true if the :term:`suite` has begun, false otherwise


.. py:method:: Suite.get_clock( (Suite)arg1) -> Clock :
   :module: ecflow

Returns the :term:`suite` :term:`clock`


.. py:method:: Suite.get_end_clock( (Suite)arg1) -> Clock :
   :module: ecflow

Return the suite's end clock. Can be NULL

