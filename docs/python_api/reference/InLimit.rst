ecflow.InLimit
//////////////


.. py:class:: InLimit
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

:term:`inlimit` is used in conjunction with :term:`limit` to provide simple load management:

.. code-block:: shell

   suite x
      limit fast 1
      family f
         inlimit /x:fast
         task t1
         task t2

Here 'fast' is the name of :py:class:`ecflow.Limit` and the number defines the maximum number of tasks
that can run simultaneously using this limit. That is why you do not need a :term:`trigger`
between tasks 't1' and 't2'. There is no need to change the tasks. The jobs are
created in the order they are defined

Constructor::

   InLimit(
        name,
        optional<path = ''>,
        optional<token = 1>,
        optional<limit_this_node_only = false>,
        optional<limit_submission_only = false
   )

      string name
          : The name of the associated Limit
      string path<optional>
          : The path to the node with associated Limit.
            If not provided, the Limit 'name' will be be found
            by traversing the node hierarchy towards the root.
      int value<optional>
          : The value used from the Limit.
            Each job submission will consume 'value' tokens from the Limit.
            Default value is to 1.
      bool limit_this_node_only<optional>
          : Only limits this node and *NOT* its children.
            Can be used to load balance families.
            Default value is false.
      bool limit_submission_only<optional>
          : The limit applies to number of submitted tasks
            Default value is false.

Usage:

.. code-block:: python

   inlimit = InLimit('fast','/x/f', 2)
    ...
   family = Family('f1',
                   InLimit('mars','/x/f', 2)) # create InLimit in Node constructor
   family.add_inlimit(inlimit)                # add existing inlimit using function


.. py:method:: InLimit.limit_submission(self: ecflow.InLimit) -> bool
   :module: ecflow

Limit submission only


.. py:method:: InLimit.limit_this_node_only(self: ecflow.InLimit) -> bool
   :module: ecflow

Only this node is limited. i.e. typically Family or Suite


.. py:method:: InLimit.name(self: ecflow.InLimit) -> str
   :module: ecflow

Return the :term:`inlimit` name as string


.. py:method:: InLimit.path_to_node(self: ecflow.InLimit) -> str
   :module: ecflow

Path to the node that holds the limit, can be empty


.. py:method:: InLimit.tokens(self: ecflow.InLimit) -> int
   :module: ecflow

The number of token to consume from the Limit

