ecflow.FlagType
///////////////


.. py:class:: FlagType
   :module: ecflow

   Bases: :py:class:`~Boost.Python.enum`

Flags store state associated with a node

- FORCE_ABORT   - Node* do not run when try_no > ECF_TRIES, and task aborted by user
- USER_EDIT     - task
- TASK_ABORTED  - task*
- EDIT_FAILED   - task*
- JOBCMD_FAILED - task*
- KILLCMD_FAILED   - task*
- STATUSCMD_FAILED - task*
- NO_SCRIPT     - task*
- KILLED        - task* do not run when try_no > ECF_TRIES, and task killed by user
- STATUS        - task* indicates that the status command has been run
- LATE          - Node attribute, Task is late, or Defs checkpt takes to long
- MESSAGE       - Node
- BYRULE        - Node*, set if node is set to complete by complete trigger expression
- QUEUELIMIT    - Node
- WAIT          - task* 
- LOCKED        - Server
- ZOMBIE        - task*
- NO_REQUE      - task
- ARCHIVED      - Suite/Family
- RESTORED      - Family/Family
- THRESHOLD     - task
- SIGTERM       - Defs, records that server received a SIGTERM signal
- LOG_ERROR     - Error in opening or writing to log file
- CHECKPT_ERROR - Error in opening or writing to checkpt file 
- NOT_SET


.. py:attribute:: FlagType.archived
   :module: ecflow
   :value: ecflow.FlagType.archived


.. py:attribute:: FlagType.byrule
   :module: ecflow
   :value: ecflow.FlagType.byrule


.. py:attribute:: FlagType.checkpt_error
   :module: ecflow
   :value: ecflow.FlagType.checkpt_error


.. py:attribute:: FlagType.edit_failed
   :module: ecflow
   :value: ecflow.FlagType.edit_failed


.. py:attribute:: FlagType.force_abort
   :module: ecflow
   :value: ecflow.FlagType.force_abort


.. py:attribute:: FlagType.jobcmd_failed
   :module: ecflow
   :value: ecflow.FlagType.jobcmd_failed


.. py:attribute:: FlagType.killcmd_failed
   :module: ecflow
   :value: ecflow.FlagType.killcmd_failed


.. py:attribute:: FlagType.killed
   :module: ecflow
   :value: ecflow.FlagType.killed


.. py:attribute:: FlagType.late
   :module: ecflow
   :value: ecflow.FlagType.late


.. py:attribute:: FlagType.locked
   :module: ecflow
   :value: ecflow.FlagType.locked


.. py:attribute:: FlagType.log_error
   :module: ecflow
   :value: ecflow.FlagType.log_error


.. py:attribute:: FlagType.message
   :module: ecflow
   :value: ecflow.FlagType.message


.. py:attribute:: FlagType.names
   :module: ecflow
   :value: {'archived': ecflow.FlagType.archived, 'byrule': ecflow.FlagType.byrule, 'checkpt_error': ecflow.FlagType.checkpt_error, 'edit_failed': ecflow.FlagType.edit_failed, 'force_abort': ecflow.FlagType.force_abort, 'jobcmd_failed': ecflow.FlagType.jobcmd_failed, 'killcmd_failed': ecflow.FlagType.killcmd_failed, 'killed': ecflow.FlagType.killed, 'late': ecflow.FlagType.late, 'locked': ecflow.FlagType.locked, 'log_error': ecflow.FlagType.log_error, 'message': ecflow.FlagType.message, 'no_reque': ecflow.FlagType.no_reque, 'no_script': ecflow.FlagType.no_script, 'not_set': ecflow.FlagType.not_set, 'queuelimit': ecflow.FlagType.queuelimit, 'restored': ecflow.FlagType.restored, 'sigterm': ecflow.FlagType.sigterm, 'status': ecflow.FlagType.status, 'statuscmd_failed': ecflow.FlagType.statuscmd_failed, 'task_aborted': ecflow.FlagType.task_aborted, 'threshold': ecflow.FlagType.threshold, 'user_edit': ecflow.FlagType.user_edit, 'wait': ecflow.FlagType.wait, 'zombie': ecflow.FlagType.zombie}


.. py:attribute:: FlagType.no_reque
   :module: ecflow
   :value: ecflow.FlagType.no_reque


.. py:attribute:: FlagType.no_script
   :module: ecflow
   :value: ecflow.FlagType.no_script


.. py:attribute:: FlagType.not_set
   :module: ecflow
   :value: ecflow.FlagType.not_set


.. py:attribute:: FlagType.queuelimit
   :module: ecflow
   :value: ecflow.FlagType.queuelimit


.. py:attribute:: FlagType.restored
   :module: ecflow
   :value: ecflow.FlagType.restored


.. py:attribute:: FlagType.sigterm
   :module: ecflow
   :value: ecflow.FlagType.sigterm


.. py:attribute:: FlagType.status
   :module: ecflow
   :value: ecflow.FlagType.status


.. py:attribute:: FlagType.statuscmd_failed
   :module: ecflow
   :value: ecflow.FlagType.statuscmd_failed


.. py:attribute:: FlagType.task_aborted
   :module: ecflow
   :value: ecflow.FlagType.task_aborted


.. py:attribute:: FlagType.threshold
   :module: ecflow
   :value: ecflow.FlagType.threshold


.. py:attribute:: FlagType.user_edit
   :module: ecflow
   :value: ecflow.FlagType.user_edit


.. py:attribute:: FlagType.values
   :module: ecflow
   :value: {0: ecflow.FlagType.force_abort, 1: ecflow.FlagType.user_edit, 2: ecflow.FlagType.task_aborted, 3: ecflow.FlagType.edit_failed, 4: ecflow.FlagType.jobcmd_failed, 5: ecflow.FlagType.no_script, 6: ecflow.FlagType.killed, 7: ecflow.FlagType.late, 8: ecflow.FlagType.message, 9: ecflow.FlagType.byrule, 10: ecflow.FlagType.queuelimit, 11: ecflow.FlagType.wait, 12: ecflow.FlagType.locked, 13: ecflow.FlagType.zombie, 14: ecflow.FlagType.no_reque, 15: ecflow.FlagType.archived, 16: ecflow.FlagType.restored, 17: ecflow.FlagType.threshold, 18: ecflow.FlagType.sigterm, 19: ecflow.FlagType.not_set, 20: ecflow.FlagType.log_error, 21: ecflow.FlagType.checkpt_error, 22: ecflow.FlagType.killcmd_failed, 23: ecflow.FlagType.statuscmd_failed, 24: ecflow.FlagType.status}


.. py:attribute:: FlagType.wait
   :module: ecflow
   :value: ecflow.FlagType.wait


.. py:attribute:: FlagType.zombie
   :module: ecflow
   :value: ecflow.FlagType.zombie

