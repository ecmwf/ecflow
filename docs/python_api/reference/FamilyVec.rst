ecflow.FamilyVec
////////////////


.. py:class:: FamilyVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of :term:`family` nodes


.. py:method:: FamilyVec.append(self: ecflow.FamilyVec, x: Family) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: FamilyVec.clear(self: ecflow.FamilyVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: FamilyVec.count(self: ecflow.FamilyVec, x: Family) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: FamilyVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.FamilyVec, L: ecflow.FamilyVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.FamilyVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: FamilyVec.insert(self: ecflow.FamilyVec, i: typing.SupportsInt | typing.SupportsIndex, x: Family) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: FamilyVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.FamilyVec) -> Family

Remove and return the last item

2. pop(self: ecflow.FamilyVec, i: typing.SupportsInt | typing.SupportsIndex) -> Family

Remove and return the item at index ``i``


.. py:method:: FamilyVec.remove(self: ecflow.FamilyVec, x: Family) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

