.. _cli_scripting_in_batch:

Using :code:`ecflow_client` in Tasks
////////////////////////////////////

:code:`ecflow_client` can be used from within Task scripts. This enables some very
powerful tools for controlling suites, and allows to set up dynamic suites
(i.e. suites that manage themselves, by updating/adding removing nodes and attributes).

ecFlow variables can be updated (using the :code:`--alter` options), Tasks and Families can
be set to Complete (using :code:`--force`), and Suites can be dynamically generated.

Dynamic Suite generation can be easily achieved by modifying a Definition file template,
followed by replacing the modified part of the Suite.

.. code-block:: shell
   :caption: Examples of how to use :code:`--alter` command

    ecflow_client --alter=add variable GLOBAL "value" /           # add server variable
    ecflow_client --alter=add variable FRED "value" /path/to/node # add node variable
    ecflow_client --alter=add time "+00:20" /path/to/node
    ecflow_client --alter=add date "01.*.*" /path/to/node
    ecflow_client --alter=add day "sunday"  /path/to/node
    ecflow_client --alter=add label name "label_value" /path/to/node
    ecflow_client --alter=add late "-s 00:01 -a 14:30 -c +00:01" /path/to/node
    ecflow_client --alter=add limit mars "100" /path/to/node
    ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name "10" /s1
    
    # zombie attributes have the following structure:
        `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`
          zombie_type        = "user" | "ecf" | "path" | "ecf_pid" | "ecf_passwd" | "ecf_pid_passwd"
          client_side_action = "fob" | "fail" | "block"
          server_side_action = "adopt" | "delete" | "kill"
          child              = "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete" | "queue"
          zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60
    ecflow_client --alter=add zombie "ecf:fail::" /path/to/node     # ask system zombies to fail
    ecflow_client --alter=add zombie "user:fail::" /path/to/node    # ask user generated zombies to fail
    ecflow_client --alter=add zombie "path:fail::" /path/to/node    # ask path zombies to fail
    
    ecflow_client --alter=delete variable FRED /path/to/node    # delete variable FRED
    ecflow_client --alter=delete variable      /path/to/node    # delete *ALL* variables on the specified node
