.. _cli_scripting_in_batch:

CLI scripting in batch
//////////////////////

You can use the CLI from within your tasks. This gives you some very
powerful tools for controlling your suite and can even allow you to
set up dynamic suites.

You can alter ecFlow variables (using alter), set particular tasks or
families complete (using force), and even generate dynamic suites. To
do this you could modify a definition file template and replace the
modified part of the suite.

Alter examples:

.. code-block:: shell
   :caption: alter

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

