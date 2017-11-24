.. index::
   single: zombie

.. _zombie:

Zombie
======

A :term:`zombie` is a running job that fails authentication when communicating with the :term:`ecflow_server`
      
How are zombies created ?
-------------------------

| There are wide variety of reasons why a :term:`zombie` is created.
| The most common causes are due to user action:

- The :term:`node` tree is deleted, replaced or reloaded whilst jobs are running
- A :term:`task` is rerun, whilst in a :term:`submitted` or :term:`active` state
- A job is forced to new state, i.e :term:`complete`

More rarer causes might be:

- :term:`ecf script` errors, where we have multiple calls to init and complete :term:`child command` s
- The :term:`child command` s in the :term:`ecf script` are placed in the background.  
  In this case order in which the :term:`child command` contact the server, may be indeterminate. 
- Load leveler submitting a job twice
- Server crash and recovered :term:`check point` file is out of date
- Machine crash

How can zombie's be handled ?
-----------------------------

The default behaviour is to **block** the job. 

| The :term:`child command` continues attempting to contact the :term:`ecflow_server`. 
| This is done for period of 24 hours. (This period is configurable see ECF_TIMEOUT on :term:`ecflow_client`). 

| The jobs can also configured, so that if the server denies the communication, then
| the :term:`child command` can be set to fail immediately. (See ECF_DENIED on :term:`ecflow_client`)

:term:`ecflowview` provides a dialog which lists all the zombies and the actions that can be taken. These include:

- Terminate: 

  | The :term:`child command` is asked to **fail**. 
  | Depending on your scripts,this may cause the abort :term:`child command` to be called.
  | Which again will be flagged as a :term:`zombie`.

- Fob: 

  Allow the job to continue. The :term:`child command` completes and hence no longer blocks the job.
  
  | Great care should be taken when this action is chosen. 
  | If we have two jobs running, they may cause data corruption.
  | Even when we have a single job, issues can arise.
  | i.e if the associated command was an event :term:`child command`, then the
  | :term:`event` would not be set. If this :term:`event` was used in a :term:`trigger` expression,
  | it would never evaluate.
  
- Delete: 

  | Remove the :term:`zombie` from the server. The job will continue blocking, hence
  | when the :term:`child command` next contacts the :term:`ecflow_server`, the :term:`zombie` will re-appear. 
  | If the job is killed manually, then this option can be used.
  
- Rescue: 

  | **Adopt** the zombie and update the node tree.
  | The ECF_PASS on the zombie is copied over to the :term:`task`, so that the next
  | :term:`child command` will continue as normal.

- Kill:

  | Applies the kill command (ECF_KILL_CMD ) using the process id stored on the :term:`zombie`.
  | If the script has correct signal trapping, this should end up calling abort.
  | Note: path zombies will need to be killed manually.
  

.. Warning::

   Of the four action above, only Rescue will allow :term:`child command` to change the state of the node tree.

**What to do:**

#. Create a :term:`zombie` by starting a :term:`task`, and setting it to :term:`complete` immediately via :term:`ecflowview`
#. Inspect the log file, it will show you how the zombie has arisen.
#. Inspect the zombie dialog in :term:`ecflowview` (right mouse button selection on the host node)
#. Experiment with the different actions on the zombie
#. Select host node and invoke the **option...** menu selection.
   Select the Zombies button. This enables zombie notification via window pop up
