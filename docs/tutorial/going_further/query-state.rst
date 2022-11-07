.. index::
   single: query state (tutorial)

.. _tutorial-query_state:

Query state
=============

In this exercise, we will use one of the command-line features of ecflow. We will use **query** command, to determine the state of a node, event, meter.  An alternative to the query command would be to use the python API.

The general format is:

.. code-block:: shell
    :caption: query command

    ecflow_client --query arg1 arg2 arg3

Where:

* arg1 = [ state | event | meter | label | variable | trigger | limit | limit_max ]
* arg2 = <path> | <path>:name     where name is name of a event, meter,limit or variable
* arg3 = trigger expression (optional)  | prev | next    # prev,next only used when arg1 is repeat

Here are some examples using the query command:

.. code-block:: shell
    
    # return node state 
    state=$(ecflow_client --query state /path/to/node)

    # state that can includes suspended
    dstate=$(ecflow_client --query dstate /path/to/node) 
    
    # return the current value as a string
    value=$(ecflow_client --query repeat /path/to/node )

    # return the previous value as a string, does not modify real repeat
    value=$(ecflow_client --query repeat /path/to/node   prev ) 

    # return the next value as a string, does not modify real repeat
    value=$(ecflow_client --query repeat /path/to/node   next) 

    # return set | clear to standard out
    event=$(ecflow_client --query event /path/to/task/with/event:event_name) 
    
    # returns the current value of the meter 
    meter=$(ecflow_client --query meter /path/to/task/with/meter:meter_name) 
    
    # returns the variable value
    value=$(ecflow_client --query variable /path/to/task/with/var:var_name)   
    
    # returns the current value of the limit 
    limit_value=$(ecflow_client --query limit  /path/to/task/with/limit:limit_name) 
    
    # returns the max value of the limit 
    limit_max=$(ecflow_client --query limit_max /path/to/task/with/limit:limit_name)
    
    # returns the current value of the label 
    label_value=$(ecflow_client --query label %ECF_NAME%:label_name) 
    
    # return true if expression evaluates false otherwise
    value=$(ecflow_client --query trigger /path/to/node/with/trigger \"/suite/task == complete\") 

Ecf Script
---------------

We will create a new query task.

.. code-block:: shell
    :caption: $HOME/course/f1/query.ecf
        
    %include <head.h>
    
    meter=$(ecflow_client --query meter /test/f1/t1:progress)
    while [[ $meter -lt 100 ]]
    do
        sleep 2
        meter=$(ecflow_client --query meter /test/f1/t1:progress)
        eventa=$(ecflow_client --query event /test/f1/t2:a)
        eventb=$(ecflow_client --query event /test/f1/t2:b)
        t5_state=$(ecflow_client --query state /test/f1/t5)
        ecflow_client --label=query "meter($meter) eventa($eventa) eventb($eventb) t5_state($t5_state)"
    done
    
    %include <tail.h>

Text
------

.. code-block:: shell

    # Definition of the suite test.
    suite test
        edit ECF_INCLUDE "$HOME/course"    # replace '$HOME' with the path to your home directory
        edit ECF_HOME    "$HOME/course"
        family f1
            edit SLEEP 20
            task t1
                meter progress 1 100 90
            task t2
                trigger t1 eq complete
                event a
                event b
            task t3
                trigger t2:a
            task t4
                trigger t2 eq complete
                complete t2:b
            task t5
                trigger t1:progress ge 30
            task t6
                trigger t1:progress ge 60
            task t7
                trigger t1:progress ge 90
            task query
                label query ""
        endfamily
    endsuite

Python
------

.. literalinclude:: src/query-state.py
   :language: python
   :caption: $HOME/course/test.py

**What to do**

#. Go back to the previous exercise where we finished adding a meter.
#. Edit the definition file or python to add the modifications. You should only need to add a task query.
#. create file :file:`query.ecf` to call :term:`ecflow_client` –query
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. Observe the tasks in :term:`ecflow_ui`
#. Modify :file:`query.ecf`, to use ecflow_client --query variable, and show this variable in the query label. 

.. note::

    Although a variable is accessible in the script by using %VAR%, ecflow_client --query variable might be more useful in an interactive shell or a different server
