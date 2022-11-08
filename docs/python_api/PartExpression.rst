ecflow.PartExpression
/////////////////////


.. py:class:: PartExpression
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

PartExpression holds part of a :term:`trigger` or :term:`complete expression`.

Expressions can contain references to :term:`event`, :term:`meter` s, user variables,
:term:`repeat` variables and generated variables. The part expression allows us
to split a large trigger or complete expression into smaller ones

Constructor::

  PartExpression(exp )
      string   exp: This represents the *first* expression

  PartExpression(exp, bool and_expr)
      string   exp: This represents the expression
      bool and_exp: If true the expression is to be anded, with a previously added expression
                    If false the expression is to be 'ored', with a previously added expression

Usage:
To add simple expression this class can be by-passed, i.e. can use:

.. code-block:: python

  task = Task('t1')
  task.add_trigger( 't2 == active' )
  task.add_complete( 't2 == complete' )

To add large triggers and complete expression:

.. code-block:: python

  exp1 = PartExpression('t1 == complete')
  # a simple expression can be added as a string
  ....
  task2.add_part_trigger( PartExpression('t1 == complete or t4 == complete') ) 
  task2.add_part_trigger( PartExpression('t5 == active',True) )    # anded with first expression
  task2.add_part_trigger( PartExpression('t7 == active',False) )   # or'ed with last expression added

The trigger for task2 is equivalent to
't1 == complete or t4 == complete and t5 == active or t7 == active'


.. py:method:: PartExpression.and_expr( (PartExpression)arg1) -> bool
   :module: ecflow


.. py:method:: PartExpression.get_expression( (PartExpression)arg1) -> str :
   :module: ecflow

returns the part expression as a string


.. py:method:: PartExpression.or_expr( (PartExpression)arg1) -> bool
   :module: ecflow

