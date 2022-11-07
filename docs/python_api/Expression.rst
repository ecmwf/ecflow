ecflow.Expression
/////////////////


.. py:class:: Expression
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Expression holds :term:`trigger` or :term:`complete expression`. Also see :py:class:`ecflow.Trigger`

Expressions can contain references to events, meters, user variables,repeat variables and generated variables.
Expressions hold a list of part expressions. This allows us to split a large trigger or complete
expression into smaller ones.

Constructor::

   Expression( expression )
      string expression  : This typically represents the complete expression
                           however part expression can still be added
   Expression( part )
      PartExpression part: The first part expression should have no 'and/or' set

Usage:
To add simple expression this class can be by passed, i.e. can use::

  task = Task('t1')
  task.add_trigger( 't2 == active' )
  task.add_complete( 't2 == complete' )

  task = Task('t2')
  task.add_trigger( 't1 == active' )
  task.add_part_trigger( 't3 == active', True)

To store and add large expressions use a Expression with PartExpression::

  big_expr = Expression( PartExpression('t1 == complete or t4 == complete') )
  big_expr.add( PartExpression('t5 == active',True) )
  big_expr.add( PartExpression('t7 == active',False) )
  task.add_trigger( big_expr)

In the example above the trigger for task is equivalent to
't1 == complete or t4 == complete and t5 == active or t7 == active'

::

  big_expr2 = Expression('t0 == complete'))
  big_expr2.add( PartExpression('t1 == complete or t4 == complete',True) )
  big_expr2.add( PartExpression('t5 == active',False) )
  task2.add_trigger( big_expr2)

Here the trigger for task2 is equivalent to
't0 == complete and t1 == complete or t4 == complete or t5 == active'


.. py:method:: Expression.add( (Expression)arg1, (PartExpression)arg2) -> None :
   :module: ecflow

Add a part expression, the second and subsequent part expressions must have 'and/or' set


.. py:method:: Expression.get_expression( (Expression)arg1) -> str :
   :module: ecflow

returns the complete expression as a string


.. py:property:: Expression.parts
   :module: ecflow

Returns a list of PartExpression's

