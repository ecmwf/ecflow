ecflow.SuiteVec
///////////////


.. py:class:: SuiteVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of :term:`suite` nodes's


.. py:method:: SuiteVec.append(self: ecflow.SuiteVec, x: Suite) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: SuiteVec.clear(self: ecflow.SuiteVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: SuiteVec.count(self: ecflow.SuiteVec, x: Suite) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: SuiteVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.SuiteVec, L: ecflow.SuiteVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.SuiteVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: SuiteVec.insert(self: ecflow.SuiteVec, i: typing.SupportsInt | typing.SupportsIndex, x: Suite) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: SuiteVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.SuiteVec) -> Suite

Remove and return the last item

2. pop(self: ecflow.SuiteVec, i: typing.SupportsInt | typing.SupportsIndex) -> Suite

Remove and return the item at index ``i``


.. py:method:: SuiteVec.remove(self: ecflow.SuiteVec, x: Suite) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

