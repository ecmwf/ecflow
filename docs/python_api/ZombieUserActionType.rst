ecflow.ZombieUserActionType
///////////////////////////


.. py:class:: ZombieUserActionType
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

ZombieUserActionType is used define an automated response. See class :py:class:`ZombieAttr`

This can be either on the client side or on the server side

client side:

- fob:    The :term:`child command` always succeeds, i.e allows job to complete without blocking
- fail:   The :term:`child command` is asked to fail.
- block:  The :term:`child command` is asked to block.
  This is the default action for init,complete and abort child commands

server side:

- adopt:  Allows the password supplied with the :term:`child command` s, to be adopted by the server
- kill:   Kills the zombie process associated with the :term:`child command` using ECF_KILL_CMD.
  path zombies will need to be killed manually. If kill is specified for path zombies
  they will be fobed, i.e allowed to complete without blocking the job.
- remove: :term:`ecflow_server` removes the :term:`zombie` from the zombie list.
  The child continues blocking. If the process is still running, the
  :term:`zombie` may well re-appear

Note: Only adopt will allow the :term:`child command` to continue and change the :term:`node` tree


.. py:attribute:: ZombieUserActionType.adopt
   :module: ecflow
   :value: ecflow.ZombieUserActionType.adopt


.. py:attribute:: ZombieUserActionType.block
   :module: ecflow
   :value: ecflow.ZombieUserActionType.block


.. py:attribute:: ZombieUserActionType.fail
   :module: ecflow
   :value: ecflow.ZombieUserActionType.fail


.. py:attribute:: ZombieUserActionType.fob
   :module: ecflow
   :value: ecflow.ZombieUserActionType.fob


.. py:attribute:: ZombieUserActionType.kill
   :module: ecflow
   :value: ecflow.ZombieUserActionType.kill


.. py:attribute:: ZombieUserActionType.names
   :module: ecflow
   :value: {'adopt': ecflow.ZombieUserActionType.adopt, 'block': ecflow.ZombieUserActionType.block, 'fail': ecflow.ZombieUserActionType.fail, 'fob': ecflow.ZombieUserActionType.fob, 'kill': ecflow.ZombieUserActionType.kill, 'remove': ecflow.ZombieUserActionType.remove}


.. py:attribute:: ZombieUserActionType.remove
   :module: ecflow
   :value: ecflow.ZombieUserActionType.remove


.. py:attribute:: ZombieUserActionType.values
   :module: ecflow
   :value: {0: ecflow.ZombieUserActionType.fob, 1: ecflow.ZombieUserActionType.fail, 2: ecflow.ZombieUserActionType.adopt, 3: ecflow.ZombieUserActionType.remove, 4: ecflow.ZombieUserActionType.block, 5: ecflow.ZombieUserActionType.kill}

