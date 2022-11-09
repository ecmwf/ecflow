
.. _ecflow_cli:

Command line interface (CLI)
//////////////////////////// 

The command line interface is provided by the :term:`ecflow_client`
executable. Note that most of the commands that you
execute using :ref:`ecflow_ui` are actually CLI commands. 

The very first argument to :term:`ecflow_client` specifies the command and must be prefixed with ``--``, e.g. ``--load`` in the example below:

.. code-block:: shell

    ecflow_client --load=host1.3141.check


:numref:`Table %s <cli_command_table>` below shows the full list of the available commands. You can also get it via the ``--help`` switch:

.. code-block:: shell

    ecflow_client --help


Some pages about some specific details about the CLI can be find here:

.. toctree::
    :maxdepth: 1
    
    desc/index.rst


.. list-table:: The list of available CLI commands
    :header-rows: 1
    :widths: 10 10 80
    :name: cli_command_table

    * - Command
      - Type
      - Description
    
    * - :ref:`abort_cli` 
      - :term:`child command`
      - Mark task as aborted. For use in the '.ecf' script file *only*                
        
    * - :ref:`add_cli` 
      - :term:`user command`
      - add variables i.e name=value name1=value1                
        
    * - :ref:`alter_cli` 
      - :term:`user command`
      - Alter the node according to the options.                
        
    * - :ref:`archive_cli` 
      - :term:`user command`
      - Archives suite or family nodes *IF* they have child nodes(otherwise does nothing).                
        
    * - :ref:`begin_cli` 
      - :term:`user command`
      - Begin playing the definition in the server.                
        
    * - :ref:`ch_add_cli` 
      - :term:`user command`
      - Add a set of suites, to an existing handle.                
        
    * - :ref:`ch_auto_add_cli` 
      - :term:`user command`
      - Change an existing handle so that new suites can be added automatically.                
        
    * - :ref:`ch_drop_cli` 
      - :term:`user command`
      - Drop/de-register the client handle.                
        
    * - :ref:`ch_drop_user_cli` 
      - :term:`user command`
      - Drop/de-register all handles associated with the given user.                
        
    * - :ref:`ch_register_cli` 
      - :term:`user command`
      - Register interest in a set of suites.                
        
    * - :ref:`ch_rem_cli` 
      - :term:`user command`
      - Remove a set of suites, from an existing handle.                
        
    * - :ref:`ch_suites_cli` 
      - :term:`user command`
      - Shows all the client handles, and the suites they reference                
        
    * - :ref:`check_cli` 
      - :term:`user command`
      - Checks the expression and limits in the server. Will also check trigger references.                
        
    * - :ref:`checkJobGenOnly_cli` 
      - :term:`user command`
      - Test hierarchical Job generation only, for chosen Node.                
        
    * - :ref:`check_pt_cli` 
      - :term:`user command`
      - Forces the definition file in the server to be written to disk *or* allow mode,                
        
    * - :ref:`complete_cli` 
      - :term:`child command`
      - Mark task as complete. For use in the '.ecf' script file *only*                
        
    * - :ref:`debug_cli` 
      - :term:`user command`
      - Dump out client environment settings for debug                
        
    * - :ref:`debug_server_off_cli` 
      - :term:`user command`
      - Disables debug output from the server                
        
    * - :ref:`debug_server_on_cli` 
      - :term:`user command`
      - Enables debug output from the server                
        
    * - :ref:`delete_cli` 
      - :term:`user command`
      - Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.                
        
    * - :ref:`edit_history_cli` 
      - :term:`user command`
      - Returns the edit history associated with a Node.                
        
    * - :ref:`edit_script_cli` 
      - :term:`user command`
      - Allows user to edit, pre-process and submit the script.                
        
    * - :ref:`event_cli` 
      - :term:`child command`
      - Change event. For use in the '.ecf' script file *only*                
        
    * - :ref:`file_cli` 
      - :term:`user command`
      - Return the chosen file. Select from [ script<default> | job | jobout | manual | kill | stat ]                
        
    * - :ref:`force_cli` 
      - :term:`user command`
      - Force a node to a given state, or set its event.                
        
    * - :ref:`force-dep-eval_cli` 
      - :term:`user command`
      - Force dependency evaluation. Used for DEBUG only.                
        
    * - :ref:`free-dep_cli` 
      - :term:`user command`
      - Free dependencies for a node. Defaults to triggers                
        
    * - :ref:`get_cli` 
      - :term:`user command`
      - Get the suite definition or node tree in form that is re-parse able                
        
    * - :ref:`get_state_cli` 
      - :term:`user command`
      - Get state data. For the whole suite definition or individual nodes.                
        
    * - :ref:`group_cli` 
      - :term:`user command`
      - Allows a series of ';' separated commands to be grouped and executed as one.                
        
    * - :ref:`halt_cli` 
      - :term:`user command`
      - Stop server communication with jobs, and new job scheduling.                
        
    * - :ref:`help_cli` 
      - :term:`user command`
      - Produce help message                
        
    * - :ref:`host_cli` 
      - :term:`user command`
      - host: If specified will override the environment variable ECF_HOST and default host, localhost                
        
    * - :ref:`init_cli` 
      - :term:`child command`
      - Mark task as started(active). For use in the '.ecf' script file *only*                
        
    * - :ref:`job_gen_cli` 
      - :term:`user command`
      - Job submission for chosen Node *based* on dependencies.                
        
    * - :ref:`kill_cli` 
      - :term:`user command`
      - Kills the job associated with the node.                
        
    * - :ref:`label_cli` 
      - :term:`child command`
      - Change Label. For use in the '.ecf' script file *only*                
        
    * - :ref:`load_cli` 
      - :term:`user command`
      - Check and load definition or checkpoint file into server.                
        
    * - :ref:`log_cli` 
      - :term:`user command`
      - Get,clear,flush or create a new log file.                
        
    * - :ref:`meter_cli` 
      - :term:`child command`
      - Change meter. For use in the '.ecf' script file *only*                
        
    * - :ref:`migrate_cli` 
      - :term:`user command`
      - Used to print state of the definition returned from the server to standard output.                
        
    * - :ref:`msg_cli` 
      - :term:`user command`
      - Writes the input string to the log file.                
        
    * - :ref:`news_cli` 
      - :term:`user command`
      - Returns true if state of server definition changed.                
        
    * - :ref:`order_cli` 
      - :term:`user command`
      - Re-orders the nodes held by the server                
        
    * - :ref:`ping_cli` 
      - :term:`user command`
      - Check if server is running on given host/port. Result reported to standard output.                
        
    * - :ref:`plug_cli` 
      - :term:`user command`
      - Plug command is used to move nodes.                
        
    * - :ref:`port_cli` 
      - :term:`user command`
      - port: If specified will override the environment variable ECF_PORT and default port number of 3141                
        
    * - :ref:`query_cli` 
      - :term:`user command`
      - Query the status of attributes                
        
    * - :ref:`queue_cli` 
      - :term:`child command`
      - QueueCmd. For use in the '.ecf' script file *only*                
        
    * - :ref:`reloadcustompasswdfile_cli` 
      - :term:`user command`
      - Reload the server custom password file. For those user's who don't use login name                
        
    * - :ref:`reloadpasswdfile_cli` 
      - :term:`user command`
      - Reload the server password file. To be used when ALL users have a password                
        
    * - :ref:`reloadwsfile_cli` 
      - :term:`user command`
      - Reload the white list file.                
        
    * - :ref:`remove_cli` 
      - :term:`user command`
      - remove variables i.e name name2                
        
    * - :ref:`replace_cli` 
      - :term:`user command`
      - Replaces a node in the server, with the given path                
        
    * - :ref:`requeue_cli` 
      - :term:`user command`
      - Re queues the specified node(s)                
        
    * - :ref:`restart_cli` 
      - :term:`user command`
      - Start job scheduling, communication with jobs, and respond to all requests.                
        
    * - :ref:`restore_cli` 
      - :term:`user command`
      - Manually restore archived nodes.                
        
    * - :ref:`restore_from_checkpt_cli` 
      - :term:`user command`
      - Ask the server to load the definition from an check pt file.                
        
    * - :ref:`resume_cli` 
      - :term:`user command`
      - Resume the given node. This allows job generation for the given node, or any child node.                
        
    * - :ref:`rid_cli` 
      - :term:`user command`
      - rid: If specified will override the environment variable ECF_RID, Can only be used for child commands                
        
    * - :ref:`run_cli` 
      - :term:`user command`
      - Ignore triggers, limits, time or date dependencies, just run the Task.                
        
    * - :ref:`server_load_cli` 
      - :term:`user command`
      - Generates gnuplot files that show the server load graphically.                
        
    * - :ref:`server_version_cli` 
      - :term:`user command`
      - Returns the version number of the server                
        
    * - :ref:`show_cli` 
      - :term:`user command`
      - Used to print state of the definition returned from the server to standard output.                
        
    * - :ref:`shutdown_cli` 
      - :term:`user command`
      - Stop server from scheduling new jobs.                
        
    * - :ref:`ssl_cli` 
      - :term:`user command`
      - ssl: If specified will override the environment variable ECF_SSL                
        
    * - :ref:`stats_cli` 
      - :term:`user command`
      - Returns the server statistics as a string.                
        
    * - :ref:`stats_reset_cli` 
      - :term:`user command`
      - Resets the server statistics.                
        
    * - :ref:`stats_server_cli` 
      - :term:`user command`
      - Returns the server statistics as a struct and string. For test use only.                
        
    * - :ref:`status_cli` 
      - :term:`user command`
      - Shows the status of a job associated with a task, in %ECF_JOB%.stat file                
        
    * - :ref:`suites_cli` 
      - :term:`user command`
      - Returns the list of suites, in the order defined in the server.                
        
    * - :ref:`suspend_cli` 
      - :term:`user command`
      - Suspend the given node. This prevents job generation for the given node, or any child node.                
        
    * - :ref:`sync_cli` 
      - :term:`user command`
      - Incrementally synchronise the local definition with the one in the server.                
        
    * - :ref:`sync_clock_cli` 
      - :term:`user command`
      - Incrementally synchronise the local definition with the one in the server.                
        
    * - :ref:`sync_full_cli` 
      - :term:`user command`
      - Returns the full definition from the server.                
        
    * - :ref:`terminate_cli` 
      - :term:`user command`
      - Terminate the server.                
        
    * - :ref:`user_cli` 
      - :term:`user command`
      - user: The user name to be used when contacting the server. Can only be used when password is also specified                
        
    * - :ref:`version_cli` 
      - :term:`user command`
      - Show ecflow client version number, and version of the boost library used                
        
    * - :ref:`wait_cli` 
      - :term:`child command`
      - Evaluates an expression, and block while the expression is false.                
        
    * - :ref:`why_cli` 
      - :term:`user command`
      - Show the reason why a node is not running.                
        
    * - :ref:`zombie_adopt_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and sets to adopt.                
        
    * - :ref:`zombie_block_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and sets flags to block it.                
        
    * - :ref:`zombie_fail_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and sets to fail.                
        
    * - :ref:`zombie_fob_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and sets to fob.                
        
    * - :ref:`zombie_get_cli` 
      - :term:`user command`
      - Returns the list of zombies from the server.                
        
    * - :ref:`zombie_kill_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and sets flags to kill                
        
    * - :ref:`zombie_remove_cli` 
      - :term:`user command`
      - Locates the task in the servers list of zombies, and removes it.                
        

.. toctree::
    :maxdepth: 1
    :hidden:

    api/abort.rst
    api/add.rst
    api/alter.rst
    api/archive.rst
    api/begin.rst
    api/ch_add.rst
    api/ch_auto_add.rst
    api/ch_drop.rst
    api/ch_drop_user.rst
    api/ch_register.rst
    api/ch_rem.rst
    api/ch_suites.rst
    api/check.rst
    api/checkJobGenOnly.rst
    api/check_pt.rst
    api/complete.rst
    api/debug.rst
    api/debug_server_off.rst
    api/debug_server_on.rst
    api/delete.rst
    api/edit_history.rst
    api/edit_script.rst
    api/event.rst
    api/file.rst
    api/force.rst
    api/force-dep-eval.rst
    api/free-dep.rst
    api/get.rst
    api/get_state.rst
    api/group.rst
    api/halt.rst
    api/help.rst
    api/host.rst
    api/init.rst
    api/job_gen.rst
    api/kill.rst
    api/label.rst
    api/load.rst
    api/log.rst
    api/meter.rst
    api/migrate.rst
    api/msg.rst
    api/news.rst
    api/order.rst
    api/ping.rst
    api/plug.rst
    api/port.rst
    api/query.rst
    api/queue.rst
    api/reloadcustompasswdfile.rst
    api/reloadpasswdfile.rst
    api/reloadwsfile.rst
    api/remove.rst
    api/replace.rst
    api/requeue.rst
    api/restart.rst
    api/restore.rst
    api/restore_from_checkpt.rst
    api/resume.rst
    api/rid.rst
    api/run.rst
    api/server_load.rst
    api/server_version.rst
    api/show.rst
    api/shutdown.rst
    api/ssl.rst
    api/stats.rst
    api/stats_reset.rst
    api/stats_server.rst
    api/status.rst
    api/suites.rst
    api/suspend.rst
    api/sync.rst
    api/sync_clock.rst
    api/sync_full.rst
    api/terminate.rst
    api/user.rst
    api/version.rst
    api/wait.rst
    api/why.rst
    api/zombie_adopt.rst
    api/zombie_block.rst
    api/zombie_fail.rst
    api/zombie_fob.rst
    api/zombie_get.rst
    api/zombie_kill.rst
    api/zombie_remove.rst
