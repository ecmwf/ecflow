ecflow.Limit
////////////


.. py:class:: Limit
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

:term:`limit` provides a simple load management

i.e. by limiting the number of :term:`task`\ s submitted by a server.
Limits are typically defined at the :term:`suite` level, or defined in a
separate suite, so that they can be used by multiple suites.
Once a limit is defined in a :term:`suite definition`, you must also assign families/tasks to use
this limit. See  :term:`inlimit` and :py:class:`ecflow.InLimit`

Constructor::

   Limit(name,value)
      string name: the name of the limit
      int   value: The value of the limit

Usage:

.. code-block:: python

   limit = Limit('fast', 10)
    ...
   suite = Suite('s1',
                 Limit('slow',10))  # create Limit in Node constructor
   suite.add_limit(limit)           # add existing limit using function


.. py:method:: Limit.decrement(self: ecflow.Limit, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: str) -> None
   :module: ecflow

used for test only


.. py:method:: Limit.increment(self: ecflow.Limit, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: str) -> None
   :module: ecflow

used for test only


.. py:method:: Limit.limit(self: ecflow.Limit) -> int
   :module: ecflow

The max value of the :term:`limit` as an integer


.. py:method:: Limit.name(self: ecflow.Limit) -> str
   :module: ecflow

Return the :term:`limit` name as string


.. py:method:: Limit.node_paths(self: ecflow.Limit) -> list
   :module: ecflow

List of nodes(paths) that have consumed a limit


.. py:method:: Limit.value(self: ecflow.Limit) -> int
   :module: ecflow

The :term:`limit` token value as an integer

