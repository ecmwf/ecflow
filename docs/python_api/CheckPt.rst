ecflow.CheckPt
//////////////


.. py:class:: CheckPt
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

CheckPt is enum that is used to control check pointing in the :term:`ecflow_server`

- NEVER  : Switches of check pointing
- ON_TIME: :term:`check point` file is saved periodically, specified by checkPtInterval. This is the default.
- ALWAYS : :term:`check point` file is saved after any state change, *not* recommended for large definitions
- UNDEFINED : None of the the above, used to provide default argument


.. py:attribute:: CheckPt.ALWAYS
   :module: ecflow
   :value: ecflow.CheckPt.ALWAYS


.. py:attribute:: CheckPt.NEVER
   :module: ecflow
   :value: ecflow.CheckPt.NEVER


.. py:attribute:: CheckPt.ON_TIME
   :module: ecflow
   :value: ecflow.CheckPt.ON_TIME


.. py:attribute:: CheckPt.UNDEFINED
   :module: ecflow
   :value: ecflow.CheckPt.UNDEFINED


.. py:attribute:: CheckPt.names
   :module: ecflow
   :value: {'ALWAYS': ecflow.CheckPt.ALWAYS, 'NEVER': ecflow.CheckPt.NEVER, 'ON_TIME': ecflow.CheckPt.ON_TIME, 'UNDEFINED': ecflow.CheckPt.UNDEFINED}


.. py:attribute:: CheckPt.values
   :module: ecflow
   :value: {0: ecflow.CheckPt.NEVER, 1: ecflow.CheckPt.ON_TIME, 2: ecflow.CheckPt.ALWAYS, 3: ecflow.CheckPt.UNDEFINED}

