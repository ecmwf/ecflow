ecflow.VariableList
///////////////////


.. py:class:: VariableList
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of Variables


.. py:method:: VariableList.append(self: ecflow.VariableList, x: Variable) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: VariableList.clear(self: ecflow.VariableList) -> None
   :module: ecflow

Clear the contents


.. py:method:: VariableList.count(self: ecflow.VariableList, x: Variable) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: VariableList.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.VariableList, L: ecflow.VariableList) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.VariableList, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: VariableList.insert(self: ecflow.VariableList, i: typing.SupportsInt | typing.SupportsIndex, x: Variable) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: VariableList.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.VariableList) -> Variable

Remove and return the last item

2. pop(self: ecflow.VariableList, i: typing.SupportsInt | typing.SupportsIndex) -> Variable

Remove and return the item at index ``i``


.. py:method:: VariableList.remove(self: ecflow.VariableList, x: Variable) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

