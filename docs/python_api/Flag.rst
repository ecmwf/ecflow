ecflow.Flag
///////////


.. py:class:: Flag
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Represents additional state associated with a Node.


.. py:method:: Flag.clear( (Flag)arg1, (FlagType)arg2) -> None :
   :module: ecflow

Clear the given flag. Used in test only


.. py:method:: Flag.is_set( (Flag)arg1, (FlagType)arg2) -> bool :
   :module: ecflow

Queries if a given flag is set


.. py:method:: Flag.list() -> FlagTypeVec :
   :module: ecflow
   :staticmethod:

Returns the list of all flag types. returns FlagTypeVec. Used in test only


.. py:method:: Flag.reset( (Flag)arg1) -> None :
   :module: ecflow

Clears all flags. Used in test only


.. py:method:: Flag.set( (Flag)arg1, (FlagType)arg2) -> None :
   :module: ecflow

Sets the given flag. Used in test only


.. py:method:: Flag.type_to_string( (FlagType)arg1) -> str :
   :module: ecflow
   :staticmethod:

Convert type to a string. Used in test only

