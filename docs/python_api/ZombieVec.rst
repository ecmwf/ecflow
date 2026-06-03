ecflow.ZombieVec
////////////////


.. py:class:: ZombieVec
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Hold a list of zombies


.. py:method:: ZombieVec.append(self: ecflow.ZombieVec, x: Zombie) -> None
   :module: ecflow

Add an item to the end of the list


.. py:method:: ZombieVec.clear(self: ecflow.ZombieVec) -> None
   :module: ecflow

Clear the contents


.. py:method:: ZombieVec.count(self: ecflow.ZombieVec, x: Zombie) -> int
   :module: ecflow

Return the number of times ``x`` appears in the list


.. py:method:: ZombieVec.extend(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. extend(self: ecflow.ZombieVec, L: ecflow.ZombieVec) -> None

Extend the list by appending all the items in the given list

2. extend(self: ecflow.ZombieVec, L: collections.abc.Iterable) -> None

Extend the list by appending all the items in the given list


.. py:method:: ZombieVec.insert(self: ecflow.ZombieVec, i: typing.SupportsInt | typing.SupportsIndex, x: Zombie) -> None
   :module: ecflow

Insert an item at a given position.


.. py:method:: ZombieVec.pop(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. pop(self: ecflow.ZombieVec) -> Zombie

Remove and return the last item

2. pop(self: ecflow.ZombieVec, i: typing.SupportsInt | typing.SupportsIndex) -> Zombie

Remove and return the item at index ``i``


.. py:method:: ZombieVec.remove(self: ecflow.ZombieVec, x: Zombie) -> None
   :module: ecflow

Remove the first item from the list whose value is x. It is an error if there is no such item.


.. py:function:: debug_build() -> bool
   :module: ecflow

