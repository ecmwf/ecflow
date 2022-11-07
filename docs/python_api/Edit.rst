ecflow.Edit
///////////


.. py:class:: Edit
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Defines a :term:`variable` on a :term:`node` for use in :term:`ecf script`.

A Node can have a number of variables.
These variables can be added at any node level: Defs, :term:`suite`, :term:`family` or :term:`task`.
The variables are names inside a pair of '%' characters in an :term:`ecf script`.
The content of a variable replaces the variable name in the :term:`ecf script` at
job submission time. When a variable is needed at submission time, it is first
sought in the task itself. If it is not found, it is sought from the tasks parent
and so on, up through the node levels until found. See :term:`variable inheritance`
A undefined variable in a :term:`ecf script`, causes the :term:`task` to be :term:`aborted`,
without the job being submitted.

Constructor::

   Variable(name,value)
      string name: the name of the variable
      string value: The value of the variable

   Edit(dict,kwarg) # alternative that allows multiple variables

Usage::

   ...
   var = Variable('ECF_JOB_CMD','/bin/sh %ECF_JOB% &')
   task.add_variable(var)
   task.add_variable('JOE','90')

The following use example of using Edit, which allow multiple variables to added at the same time ::

   t = Task('t1',
             Edit({ 'a':'y', 'b':'bb'}, c='v',d='b'),
             Edit({ 'e':1100, 'f':'bb'}),
             Edit(g='d'),
             Edit(h='1'))

::

  defs = Defs(
            Suite('s1'),
            Edit(SLEEP='1')) # Add user variable to definition
  defs.s1 += [ Task('a') ]
  defs.s1.a += [ Edit({ 'x1':'y', 'aa1':'bb'}, a='v',b='b'),
                 Edit({ 'var':10, 'aa':'bb'}),
                 Edit(d='d') ]

