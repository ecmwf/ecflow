
.. _ecflow_cli:

Command line interface (CLI)
//////////////////////////// 

The command line interface is provided by the :term:`ecflow_client`
executable. Note that most of the commands that you
execute using :ref:`ecflow_ui` are actually CLI commands. 

The very first argument to :term:`ecflow_client` specifies the command and must be prefixed with ``--``, e.g. ``--load`` in the example below:

.. code-block:: shell

    ecflow_client --load=host1.3141.check

The comprehensive :ref:`list of ecflow_client commands <ecflow_client_commands>` is presented below.
These commands can be combined with :ref:`ecflow_client common options <ecflow_client_options>` to further customise the
:term:`ecflow_client` behaviour.

The list of commands, amongst other details, can be displayed by using the option ``--help``.

.. code-block:: shell

    ecflow_client --help


Some pages about CLI specific details can be found here:

.. toctree::
    :maxdepth: 1
    
    desc/index.rst


.. list-table:: List of :term:`ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 20 60
    :name: ecflow_client_commands

    * - Command
      - Type
      - Description
    
    * - :ref:`abort_cli` 
      - :term:`child command`
      - Mark task as aborted. For use in the '.ecf' script file *only*

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


.. list-table:: List of common options for `ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 80
    :name: ecflow_client_options

    * - Option
      - Description

    * - :ref:`add_cli`
      - Add variables e.g. name1=value1 name2=value2. Can only be used in combination with --init command.

    * - :ref:`debug_cli`
      - Enables the display of client environment settings and execution details.

    * - :ref:`host_cli`
      - When specified overrides the environment variable ECF_HOST and default host: 'localhost'

    * - :ref:`http_cli`
      - Enables communication over HTTP between client/server.

    * - :ref:`https_cli`
      - Enables communication over HTTPS between client/server.

    * - :ref:`password_cli`
      - Specifies the password used to contact the server. Must be used in combination with option --user.

    * - :ref:`port_cli`
      - When specified overrides the environment variable ECF_PORT and default port: '3141'

    * - :ref:`rid_cli`
      - When specified overrides the environment variable ECF_RID. Can only be used for child commands.

    * - :ref:`ssl_cli`
      - Enables the use of SSL when contacting the server.

    * - :ref:`user_cli`
      - Specifies the user name used to contact the server. Must be used in combination with option --password.


.. toctree::
    :maxdepth: 1
    :hidden:
    
    abort <api/abort.rst>
    alter <api/alter.rst>
    archive <api/archive.rst>
    begin <api/begin.rst>
    ch_add <api/ch_add.rst>
    ch_auto_add <api/ch_auto_add.rst>
    ch_drop <api/ch_drop.rst>
    ch_drop_user <api/ch_drop_user.rst>
    ch_register <api/ch_register.rst>
    ch_rem <api/ch_rem.rst>
    ch_suites <api/ch_suites.rst>
    check <api/check.rst>
    checkJobGenOnly <api/checkJobGenOnly.rst>
    check_pt <api/check_pt.rst>
    complete <api/complete.rst>
    debug_server_off <api/debug_server_off.rst>
    debug_server_on <api/debug_server_on.rst>
    delete <api/delete.rst>
    edit_history <api/edit_history.rst>
    edit_script <api/edit_script.rst>
    event <api/event.rst>
    file <api/file.rst>
    force <api/force.rst>
    force-dep-eval <api/force-dep-eval.rst>
    free-dep <api/free-dep.rst>
    get <api/get.rst>
    get_state <api/get_state.rst>
    group <api/group.rst>
    halt <api/halt.rst>
    help <api/help.rst>
    init <api/init.rst>
    job_gen <api/job_gen.rst>
    kill <api/kill.rst>
    label <api/label.rst>
    load <api/load.rst>
    log <api/log.rst>
    meter <api/meter.rst>
    migrate <api/migrate.rst>
    msg <api/msg.rst>
    news <api/news.rst>
    order <api/order.rst>
    ping <api/ping.rst>
    plug <api/plug.rst>
    query <api/query.rst>
    queue <api/queue.rst>
    reloadcustompasswdfile <api/reloadcustompasswdfile.rst>
    reloadpasswdfile <api/reloadpasswdfile.rst>
    reloadwsfile <api/reloadwsfile.rst>
    remove <api/remove.rst>
    replace <api/replace.rst>
    requeue <api/requeue.rst>
    restart <api/restart.rst>
    restore <api/restore.rst>
    restore_from_checkpt <api/restore_from_checkpt.rst>
    resume <api/resume.rst>
    run <api/run.rst>
    server_load <api/server_load.rst>
    server_version <api/server_version.rst>
    show <api/show.rst>
    shutdown <api/shutdown.rst>
    stats <api/stats.rst>
    stats_reset <api/stats_reset.rst>
    stats_server <api/stats_server.rst>
    status <api/status.rst>
    suites <api/suites.rst>
    suspend <api/suspend.rst>
    sync <api/sync.rst>
    sync_clock <api/sync_clock.rst>
    sync_full <api/sync_full.rst>
    terminate <api/terminate.rst>
    version <api/version.rst>
    wait <api/wait.rst>
    why <api/why.rst>
    zombie_adopt <api/zombie_adopt.rst>
    zombie_block <api/zombie_block.rst>
    zombie_fail <api/zombie_fail.rst>
    zombie_fob <api/zombie_fob.rst>
    zombie_get <api/zombie_get.rst>
    zombie_kill <api/zombie_kill.rst>
    zombie_remove <api/zombie_remove.rst>
    add (option) <api/add.rst>
    debug (option) <api/debug.rst>
    host (option) <api/host.rst>
    http (option) <api/http.rst>
    https (option) <api/https.rst>
    password (option) <api/password.rst>
    port (option) <api/port.rst>
    rid (option) <api/rid.rst>
    ssl (option) <api/ssl.rst>
    user (option) <api/user.rst>
