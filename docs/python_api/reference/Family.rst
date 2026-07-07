ecflow.Family
/////////////


.. py:class:: Family
   :module: ecflow

   Bases: :py:class:`~ecflow.NodeContainer`

Create a :term:`family` :term:`node`.A Family node lives inside a :term:`suite` or another :term:`family`

A family is used to collect :term:`task`\ s together or to group other families.
Typically you place tasks that are related to each other inside the same family
analogous to the way you create directories to contain related files.
There are two ways of adding a family, see example below.

Constructor::

  Family(name, Nodes | Attributes)
      string name : The Family name. name must consist of alpha numeric characters or
                    underscore or dot. The first character cannot be dot, as this
                    will interfere with trigger expressions. Case is significant
      Nodes | Attributes: (optional)

Exception:

- Throws a RuntimeError if the name is not valid
- Throws a RuntimeError if a duplicate family is added

Usage:

.. code-block:: python

  suite = Suite('suite_1')       # create a suite
  family = Family('family_1')    # create a family
  suite.add_family(family)       # add created family to a suite
  f2 = suite.add_family('f2')    # create a family f2 and add to suite

  # create in place
  defs = Defs(
           Suite('s1',
              Family('f1',
                 Task('t1',
                     Edit(SLEEP='10')))))

