ecflow.ChildCmdType
///////////////////


.. py:class:: ChildCmdType
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

ChildCmdType represents the different :term:`child command` s.
This type is used as a parameter to the class :py:class:`ecflow.ZombieAttr`

Child commands are called within a :term:`job file`::

  ChildCmdType::init     corresponds to : ecflow_client --init=<process_id>
  ChildCmdType::event    corresponds to : ecflow_client --event=<event_name | number>
  ChildCmdType::meter    corresponds to : ecflow_client --meter=<meter_name>, <meter_value>
  ChildCmdType::label    corresponds to : ecflow_client --label=<label_name>. <label_value>
  ChildCmdType::wait     corresponds to : ecflow_client --wait=<expression>
  ChildCmdType::abort    corresponds to : ecflow_client --abort=<reason>
  ChildCmdType::complete corresponds to : ecflow_client --complete


.. py:attribute:: ChildCmdType.abort
   :module: ecflow
   :value: ecflow.ChildCmdType.abort


.. py:attribute:: ChildCmdType.complete
   :module: ecflow
   :value: ecflow.ChildCmdType.complete


.. py:attribute:: ChildCmdType.event
   :module: ecflow
   :value: ecflow.ChildCmdType.event


.. py:attribute:: ChildCmdType.init
   :module: ecflow
   :value: ecflow.ChildCmdType.init


.. py:attribute:: ChildCmdType.label
   :module: ecflow
   :value: ecflow.ChildCmdType.label


.. py:attribute:: ChildCmdType.meter
   :module: ecflow
   :value: ecflow.ChildCmdType.meter


.. py:attribute:: ChildCmdType.names
   :module: ecflow
   :value: {'abort': ecflow.ChildCmdType.abort, 'complete': ecflow.ChildCmdType.complete, 'event': ecflow.ChildCmdType.event, 'init': ecflow.ChildCmdType.init, 'label': ecflow.ChildCmdType.label, 'meter': ecflow.ChildCmdType.meter, 'queue': ecflow.ChildCmdType.queue, 'wait': ecflow.ChildCmdType.wait}


.. py:attribute:: ChildCmdType.queue
   :module: ecflow
   :value: ecflow.ChildCmdType.queue


.. py:attribute:: ChildCmdType.values
   :module: ecflow
   :value: {0: ecflow.ChildCmdType.init, 1: ecflow.ChildCmdType.event, 2: ecflow.ChildCmdType.meter, 3: ecflow.ChildCmdType.label, 4: ecflow.ChildCmdType.wait, 5: ecflow.ChildCmdType.queue, 6: ecflow.ChildCmdType.abort, 7: ecflow.ChildCmdType.complete}


.. py:attribute:: ChildCmdType.wait
   :module: ecflow
   :value: ecflow.ChildCmdType.wait

