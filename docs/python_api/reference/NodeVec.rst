ecflow.NodeVec
//////////////


.. py:class:: NodeVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of Nodes (i.e :term:`suite`, :term:`family` or :term:`task`\ s)


.. py:method:: NodeVec.append(self: ecflow.NodeVec, x: Node) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: NodeVec.clear(self: ecflow.NodeVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: NodeVec.count(self: ecflow.NodeVec, x: Node) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: NodeVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.NodeVec, L: ecflow.NodeVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.NodeVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: NodeVec.insert(self: ecflow.NodeVec, i: typing.SupportsInt | typing.SupportsIndex, x: Node) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: NodeVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.NodeVec) -> Node

Remove and return the last item

2. pop(self: ecflow.NodeVec, i: typing.SupportsInt | typing.SupportsIndex) -> Node

Remove and return the item at index ``i``


.. py:method:: NodeVec.remove(self: ecflow.NodeVec, x: Node) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.

