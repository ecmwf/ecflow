ecflow.FlagTypeVec
//////////////////


.. py:class:: FlagTypeVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of flag types


.. py:method:: FlagTypeVec.append(self: ecflow.FlagTypeVec, x: ecflow.FlagType) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: FlagTypeVec.clear(self: ecflow.FlagTypeVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: FlagTypeVec.count(self: ecflow.FlagTypeVec, x: ecflow.FlagType) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: FlagTypeVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.FlagTypeVec, L: ecflow.FlagTypeVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.FlagTypeVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: FlagTypeVec.insert(self: ecflow.FlagTypeVec, i: typing.SupportsInt | typing.SupportsIndex, x: ecflow.FlagType) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: FlagTypeVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.FlagTypeVec) -> ecflow.FlagType

Remove and return the last item

2. pop(self: ecflow.FlagTypeVec, i: typing.SupportsInt | typing.SupportsIndex) -> ecflow.FlagType

Remove and return the item at index ``i``


.. py:method:: FlagTypeVec.remove(self: ecflow.FlagTypeVec, x: ecflow.FlagType) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

