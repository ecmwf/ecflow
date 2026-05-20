ecflow.Flag
///////////


.. py:class:: Flag
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Represents additional state associated with a Node.


.. py:method:: Flag.clear(self: ecflow.Flag, arg0: ecflow.FlagType) -> None
   :module: ecflow

Clear the given flag. Used in test only


.. py:method:: Flag.is_set(self: ecflow.Flag, arg0: ecflow.FlagType) -> bool
   :module: ecflow

Queries if a given flag is set


.. py:method:: Flag.list() -> std::__1::vector<ecf::Flag::Type, std::__1::allocator<ecf::Flag::Type>>
   :module: ecflow
   :staticmethod:

Returns the list of all flag types. returns FlagTypeVec. Tests only


.. py:method:: Flag.reset(self: ecflow.Flag) -> None
   :module: ecflow

Clears all flags. Used in test only


.. py:method:: Flag.set(self: ecflow.Flag, arg0: ecflow.FlagType) -> None
   :module: ecflow

Sets the given flag. Used in test only


.. py:method:: Flag.type_to_string(arg0: ecflow.FlagType) -> str
   :module: ecflow
   :staticmethod:

Convert type to a string. Tests only

