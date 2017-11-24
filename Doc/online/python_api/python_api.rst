.. index::
   single: python_api
   
.. _python_api:
   
=====================
**ecFlow Python Api**
=====================

.. _suite_definition_python_api:

Suite Definition API
--------------------
- :py:class:`ecflow.Autocancel`
- :py:class:`ecflow.Complete`
- :py:class:`ecflow.Clock`
- :py:class:`ecflow.Cron`
- :py:class:`ecflow.Date`
- :py:class:`ecflow.Day`
- :py:class:`ecflow.Defs`
- :py:class:`ecflow.Edit`
- :py:class:`ecflow.Event`
- :py:class:`ecflow.Expression`
- :py:class:`ecflow.InLimit`
- :py:class:`ecflow.Label`
- :py:class:`ecflow.Late`
- :py:class:`ecflow.Limit`
- :py:class:`ecflow.Meter`

- :py:class:`ecflow.Node`
- :py:class:`ecflow.Family`
- :py:class:`ecflow.Suite`
- :py:class:`ecflow.Task`

- :py:class:`ecflow.PartExpression`
- :py:class:`ecflow.Repeat`
- :py:class:`ecflow.RepeatDate`
- :py:class:`ecflow.RepeatDay`
- :py:class:`ecflow.RepeatEnumerated`
- :py:class:`ecflow.RepeatInteger`
- :py:class:`ecflow.RepeatString`
- :py:class:`ecflow.Time`
- :py:class:`ecflow.TimeSeries`
- :py:class:`ecflow.TimeSlot`
- :py:class:`ecflow.Trigger`
- :py:class:`ecflow.Today`
- :py:class:`ecflow.Variable`
- :py:class:`ecflow.Verify`
- :py:class:`ecflow.ZombieAttr`


Container API
-------------
- :py:class:`ecflow.NodeVec`
- :py:class:`ecflow.NodeContainer`
- :py:class:`ecflow.FamilyVec`
- :py:class:`ecflow.SuiteVec`
- :py:class:`ecflow.TaskVec`


Command API
-----------
- :py:class:`ecflow.UrlCmd`
- :py:class:`ecflow.WhyCmd`


.. _client_server_python_api:

Client Server API
-----------------
- :py:class:`ecflow.Client`
  
  * :py:class:`ecflow.Client.alter`
  * :py:class:`ecflow.Client.begin_all_suites`
  * :py:class:`ecflow.Client.begin_suite`
  * :py:class:`ecflow.Client.ch_add`
  * :py:class:`ecflow.Client.ch_auto_add`
  * :py:class:`ecflow.Client.ch_drop`
  * :py:class:`ecflow.Client.ch_drop_user`
  * :py:class:`ecflow.Client.ch_handle`
  * :py:class:`ecflow.Client.ch_register`
  * :py:class:`ecflow.Client.ch_remove`
  * :py:class:`ecflow.Client.checkpt`
  * :py:class:`ecflow.Client.clear_log`
  * :py:class:`ecflow.Client.delete`
  * :py:class:`ecflow.Client.delete_all`
  * :py:class:`ecflow.Client.flush_log`
  * :py:class:`ecflow.Client.force_event`
  * :py:class:`ecflow.Client.force_state`
  * :py:class:`ecflow.Client.force_state_recursive`
  * :py:class:`ecflow.Client.free_all_dep`
  * :py:class:`ecflow.Client.free_date_dep`
  * :py:class:`ecflow.Client.free_time_dep`
  * :py:class:`ecflow.Client.free_trigger_dep`
  * :py:class:`ecflow.Client.get_defs`
  * :py:class:`ecflow.Client.get_server_defs`
  * :py:class:`ecflow.Client.group`
  * :py:class:`ecflow.Client.halt_server`
  * :py:class:`ecflow.Client.in_sync`
  * :py:class:`ecflow.Client.job_generation`
  * :py:class:`ecflow.Client.kill`
  * :py:class:`ecflow.Client.load`
  * :py:class:`ecflow.Client.log_msg`
  * :py:class:`ecflow.Client.new_log`
  * :py:class:`ecflow.Client.news_local`
  * :py:class:`ecflow.Client.order`
  * :py:class:`ecflow.Client.ping`
  * :py:class:`ecflow.Client.plug`
  * :py:class:`ecflow.Client.reload_wl_file`
  * :py:class:`ecflow.Client.replace`
  * :py:class:`ecflow.Client.requeue`
  * :py:class:`ecflow.Client.restart_server`
  * :py:class:`ecflow.Client.restore_from_checkpt`
  * :py:class:`ecflow.Client.resume`
  * :py:class:`ecflow.Client.run`
  * :py:class:`ecflow.Client.set_connection_attempts`
  * :py:class:`ecflow.Client.set_host_port`
  * :py:class:`ecflow.Client.set_retry_connection_period`
  * :py:class:`ecflow.Client.shutdown_server`
  * :py:class:`ecflow.Client.stats`
  * :py:class:`ecflow.Client.status`
  * :py:class:`ecflow.Client.suites`
  * :py:class:`ecflow.Client.suspend`
  * :py:class:`ecflow.Client.sync_local`
  * :py:class:`ecflow.Client.terminate_server`
  

Common API
----------
- :py:class:`ecflow.PrintStyle`
- :py:class:`ecflow.JobCreationCtrl`
 

enum's
------
- :py:class:`ecflow.ChildCmdType`
- :py:class:`ecflow.DState`
- :py:class:`ecflow.Days`
- :py:class:`ecflow.State`
- :py:class:`ecflow.Style`
- :py:class:`ecflow.ZombieType`
- :py:class:`ecflow.ZombieUserActionType`


Api
---

.. automodule:: ecflow
   :members:   
   :undoc-members:
   :show-inheritance:
   
.. contents::
   :depth: 2
   :local:
   :backlinks: top
    