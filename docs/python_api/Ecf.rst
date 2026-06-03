ecflow.Ecf
//////////


.. py:class:: Ecf
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Singleton used to control ecf debugging


.. py:method:: Ecf.debug_equality() -> bool
   :module: ecflow
   :staticmethod:

Returns true if debugging of equality is enabled


.. py:method:: Ecf.debug_level() -> int
   :module: ecflow
   :staticmethod:

Returns integer showing debug level. debug_level > 0 will disable some warning messages


.. py:method:: Ecf.set_debug_equality(arg0: bool) -> None
   :module: ecflow
   :staticmethod:

Set debugging for equality


.. py:method:: Ecf.set_debug_level(arg0: typing.SupportsInt | typing.SupportsIndex) -> None
   :module: ecflow
   :staticmethod:

Set debug level. debug_level > 0 will disable some warning messages

