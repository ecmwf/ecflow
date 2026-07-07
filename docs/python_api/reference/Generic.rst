ecflow.Generic
//////////////


.. py:class:: Generic
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

A generic attribute, used to add new attributes for the future, without requiring a API change


.. py:method:: Generic.empty(self: ecflow.Generic) -> bool
   :module: ecflow

Return true if the Generic is empty. Used when returning a NULL Generic, from a find


.. py:method:: Generic.name(self: ecflow.Generic) -> str
   :module: ecflow

Return the generic name as string


.. py:property:: Generic.values
   :module: ecflow

The list of values for the generic

