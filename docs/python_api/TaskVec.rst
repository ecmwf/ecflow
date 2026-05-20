ecflow.TaskVec
//////////////


.. py:class:: TaskVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of :term:`task` nodes


.. py:method:: TaskVec.append(self: ecflow.TaskVec, x: Task) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: TaskVec.clear(self: ecflow.TaskVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: TaskVec.count(self: ecflow.TaskVec, x: Task) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: TaskVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.TaskVec, L: ecflow.TaskVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.TaskVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: TaskVec.insert(self: ecflow.TaskVec, i: typing.SupportsInt | typing.SupportsIndex, x: Task) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: TaskVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.TaskVec) -> Task

Remove and return the last item

2. pop(self: ecflow.TaskVec, i: typing.SupportsInt | typing.SupportsIndex) -> Task

Remove and return the item at index ``i``


.. py:method:: TaskVec.remove(self: ecflow.TaskVec, x: Task) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

