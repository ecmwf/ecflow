ecflow.CheckPt
//////////////


.. py:class:: CheckPt
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

CheckPt is enum that is used to control check pointing in the :term:`ecflow_server`

- NEVER  : Switches of check pointing
- ON_TIME: :term:`check point` file is saved periodically, specified by checkPtInterval. This is the default.
- ALWAYS : :term:`check point` file is saved after any state change, *not* recommended for large definitions
- UNDEFINED : None of the the above, used to provide default argument


Members:

  NEVER

  ON_TIME

  ALWAYS

  UNDEFINED


.. py:attribute:: CheckPt.ALWAYS
   :module: ecflow
   :value: <CheckPt.ALWAYS: 2>


.. py:attribute:: CheckPt.NEVER
   :module: ecflow
   :value: <CheckPt.NEVER: 0>


.. py:attribute:: CheckPt.ON_TIME
   :module: ecflow
   :value: <CheckPt.ON_TIME: 1>


.. py:attribute:: CheckPt.UNDEFINED
   :module: ecflow
   :value: <CheckPt.UNDEFINED: 3>


.. py:property:: CheckPt.name
   :module: ecflow


.. py:property:: CheckPt.value
   :module: ecflow

