
.. _user_cli:

user
////

::

   
   Ecflow user client commands:
   
     add                     user   add variables i.e name=value name1=value1
     alter                   user   Alter the node according to the options.
     archive                 user   Archives suite or family nodes *IF* they have child nodes(otherwise does nothing).
     begin                   user   Begin playing the definition in the server.
     ch_add                  user   Add a set of suites, to an existing handle.
     ch_auto_add             user   Change an existing handle so that new suites can be added automatically.
     ch_drop                 user   Drop/de-register the client handle.
     ch_drop_user            user   Drop/de-register all handles associated with the given user.
     ch_register             user   Register interest in a set of suites.
     ch_rem                  user   Remove a set of suites, from an existing handle.
     ch_suites               user   Shows all the client handles, and the suites they reference
     check                   user   Checks the expression and limits in the server. Will also check trigger references.
     checkJobGenOnly         user   Test hierarchical Job generation only, for chosen Node.
     check_pt                user   Forces the definition file in the server to be written to disk *or* allow mode,
     debug                   user   Dump out client environment settings for debug
     debug_server_off        user   Disables debug output from the server
     debug_server_on         user   Enables debug output from the server
     delete                  user   Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.
     edit_history            user   Returns the edit history associated with a Node.
     edit_script             user   Allows user to edit, pre-process and submit the script.
     file                    user   Return the chosen file. Select from [ script<default> | job | jobout | manual | kill | stat ]
     force                   user   Force a node to a given state, or set its event.
     force-dep-eval          user   Force dependency evaluation. Used for DEBUG only.
     free-dep                user   Free dependencies for a node. Defaults to triggers
     get                     user   Get the suite definition or node tree in form that is re-parse able
     get_state               user   Get state data. For the whole suite definition or individual nodes.
     group                   user   Allows a series of ';' separated commands to be grouped and executed as one.
     halt                    user   Stop server communication with jobs, and new job scheduling.
     help                    user   Produce help message
     host                    user   host: If specified will override the environment variable ECF_HOST and default host, localhost
     job_gen                 user   Job submission for chosen Node *based* on dependencies.
     kill                    user   Kills the job associated with the node.
     load                    user   Check and load definition or checkpoint file into server.
     log                     user   Get,clear,flush or create a new log file.
     migrate                 user   Used to print state of the definition returned from the server to standard output.
     msg                     user   Writes the input string to the log file.
     news                    user   Returns true if state of server definition changed.
     order                   user   Re-orders the nodes held by the server
     ping                    user   Check if server is running on given host/port. Result reported to standard output.
     plug                    user   Plug command is used to move nodes.
     port                    user   port: If specified will override the environment variable ECF_PORT and default port number of 3141
     query                   user   Query the status of attributes
     reloadcustompasswdfile  user   Reload the server custom password file. For those user's who don't use login name
     reloadpasswdfile        user   Reload the server password file. To be used when ALL users have a password
     reloadwsfile            user   Reload the white list file.
     remove                  user   remove variables i.e name name2
     replace                 user   Replaces a node in the server, with the given path
     requeue                 user   Re queues the specified node(s)
     restart                 user   Start job scheduling, communication with jobs, and respond to all requests.
     restore                 user   Manually restore archived nodes.
     restore_from_checkpt    user   Ask the server to load the definition from an check pt file.
     resume                  user   Resume the given node. This allows job generation for the given node, or any child node.
     rid                     user   rid: If specified will override the environment variable ECF_RID, Can only be used for child commands
     run                     user   Ignore triggers, limits, time or date dependencies, just run the Task.
     server_load             user   Generates gnuplot files that show the server load graphically.
     server_version          user   Returns the version number of the server
     show                    user   Used to print state of the definition returned from the server to standard output.
     shutdown                user   Stop server from scheduling new jobs.
     ssl                     user   ssl: If specified will override the environment variable ECF_SSL
     stats                   user   Returns the server statistics as a string.
     stats_reset             user   Resets the server statistics.
     stats_server            user   Returns the server statistics as a struct and string. For test use only.
     status                  user   Shows the status of a job associated with a task, in %ECF_JOB%.stat file
     suites                  user   Returns the list of suites, in the order defined in the server.
     suspend                 user   Suspend the given node. This prevents job generation for the given node, or any child node.
     sync                    user   Incrementally synchronise the local definition with the one in the server.
     sync_clock              user   Incrementally synchronise the local definition with the one in the server.
     sync_full               user   Returns the full definition from the server.
     terminate               user   Terminate the server.
     user                    user   user: The user name to be used when contacting the server. Can only be used when password is also specified
     version                 user   Show ecflow client version number, and version of the boost library used
     why                     user   Show the reason why a node is not running.
     zombie_adopt            user   Locates the task in the servers list of zombies, and sets to adopt.
     zombie_block            user   Locates the task in the servers list of zombies, and sets flags to block it.
     zombie_fail             user   Locates the task in the servers list of zombies, and sets to fail.
     zombie_fob              user   Locates the task in the servers list of zombies, and sets to fob.
     zombie_get              user   Returns the list of zombies from the server.
     zombie_kill             user   Locates the task in the servers list of zombies, and sets flags to kill
     zombie_remove           user   Locates the task in the servers list of zombies, and removes it.
   
   
