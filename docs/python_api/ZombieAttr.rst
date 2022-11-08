ecflow.ZombieAttr
/////////////////


.. py:class:: ZombieAttr
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

The :term:`zombie` attribute defines how a :term:`zombie` should be handled in an automated fashion

Very careful consideration should be taken before this attribute is added
as it may hide a genuine problem.
It can be added to any :term:`node`. But is best defined at the :term:`suite` or :term:`family` level.
If there is no zombie attribute the default behaviour is to block the init,complete,abort :term:`child command`.
and *fob* the event,label,and meter :term:`child command`
This attribute allows the server to make a automated response.
Please see: :py:class:`ecflow.ZombieType`, :py:class:`ecflow.ChildCmdType`, :py:class:`ecflow.ZombieUserActionType`

Constructor::

   ZombieAttr(ZombieType,ChildCmdTypes, ZombieUserActionType, lifetime)
      ZombieType            : Must be one of ZombieType.ecf, ZombieType.path, ZombieType.user
      ChildCmdType          : A list(ChildCmdType) of Child commands. Can be left empty in
                              which case the action affect all child commands
      ZombieUserActionType  : One of [ fob, fail, block, remove, adopt ]
      int lifetime<optional>: Defines the life time in seconds of the zombie in the server.
                              On expiration, zombie is removed automatically

Usage:

.. code-block:: python

   # Add a zombie attribute so that child commands(i.e ecflow_client --init)
   # will fail the job if it is a zombie process.
   s1 = Suite('s1')
   child_list = [ ChildCmdType.init, ChildCmdType.complete, ChildCmdType.abort ]
   s1.add_zombie( ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fob))

   # create the zombie as part of the node constructor
   s1 = Suite('s1',
              ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fail))


.. py:property:: ZombieAttr.child_cmds
   :module: ecflow

The list of child commands. If empty action applies to all child cmds


.. py:method:: ZombieAttr.empty( (ZombieAttr)arg1) -> bool :
   :module: ecflow

Return true if the attribute is empty


.. py:method:: ZombieAttr.user_action( (ZombieAttr)arg1) -> ZombieUserActionType :
   :module: ecflow

The automated action to invoke, when zombies arise


.. py:method:: ZombieAttr.zombie_lifetime( (ZombieAttr)arg1) -> int :
   :module: ecflow

Returns the lifetime in seconds of :term:`zombie` in the server


.. py:method:: ZombieAttr.zombie_type( (ZombieAttr)arg1) -> ZombieType :
   :module: ecflow

Returns the :term:`zombie type`

