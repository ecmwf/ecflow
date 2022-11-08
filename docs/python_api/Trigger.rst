ecflow.Trigger
//////////////


.. py:class:: Trigger
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Add a :term:`trigger` or :term:`complete expression`.

This defines a dependency for a :term:`node`.
There can only be one :term:`trigger` or :term:`complete expression` dependency per node.
A :term:`node` with a trigger can only be activated when the trigger has expired.
Triggers can reference nodes, events, meters, variables, repeats, limits and late flag
A trigger holds a node as long as the expression returns false.

Exception:

- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression
  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set

Usage:

Multiple trigger will automatically be *anded* together, If *or* is required please
use bool 'False' as the last argument i.e. :

.. code-block:: python

  task1.add( Trigger('t2 == active' ),
             Trigger('t1 == complete or t4 == complete' ),
             Trigger('t5 == active',False))

The trigger for task1 is equivalent to :

.. code-block:: python

  t2 == active and t1 == complete or t4 == complete or t5 == active

Since a large number of triggers are of the form `<node> == complete` there are
are short cuts, these involves a use of a list :

.. code-block:: python

  task1.add( Trigger( ['t2','t3'] )) #  This is same as t2 == complete and t3 == complete

You can also use a node :

.. code-block:: python

  task1.add( Trigger( ['t2',taskx] ))

If the node 'taskx' has a parent, we use the full hierarchy, hence we will get a trigger
of the form :

.. code-block:: python

  t2 ==complete and /suite/family/taskx == complete

If however node taskx has not yet been added to its parent, we use a relative name in the trigger :

.. code-block:: python

  t2 ==complete and taskx == complete


.. py:method:: Trigger.get_expression( (Trigger)arg1) -> str :
   :module: ecflow

returns the trigger expression as a string

