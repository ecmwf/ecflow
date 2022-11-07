ecflow.Ecf
//////////


.. py:class:: Ecf
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Singleton used to control ecf debugging


.. py:method:: Ecf.debug_equality() -> bool :
   :module: ecflow
   :staticmethod:

Returns true if debugging of equality is enabled


.. py:method:: Ecf.debug_level() -> int :
   :module: ecflow
   :staticmethod:

Returns integer showing debug level. debug_level > 0 will disable some warning messages


.. py:method:: Ecf.set_debug_equality( (bool)arg1) -> None :
   :module: ecflow
   :staticmethod:

Set debugging for equality


.. py:method:: Ecf.set_debug_level( (int)arg1) -> None :
   :module: ecflow
   :staticmethod:

Set debug level. debug_level > 0 will disable some warning messages

