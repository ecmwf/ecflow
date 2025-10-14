.. index::
   single: query state (tutorial)

.. _tutorial-query_state:

Using query command
===================

The :term:`ecflow_client` command :code:`--query` can be used to determine the current value of several characteristics (e.g. node state, node default state, event, meter, variable, trigger).
The same capability is provided by the :ref:`python_api`.

The general format of the :term:`ecflow_client` command :code:`--query` is as follows:

.. code-block:: shell
    :caption: query command

    ecflow_client --query arg1 arg2 arg3

Where:

* arg1 = [ state | event | meter | label | variable | trigger | limit | limit_max | ... ]
* arg2 = <path> | <path>:name     where name is name of a event, meter,limit or variable
* arg3 = trigger expression (optional)  | prev | next    # prev,next only used when arg1 is repeat

Some examples using the query command:

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

Update Task Script
------------------

Create a :term:`task script <ecf script>` for a new :term:`task` named :code:`query`, as follows:

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

Update Suite Definition
-----------------------

Modify the :term:`suite definition` to add a new task :code:`query` to the family :code:`f1` as follows:

.. tabs::

    .. tab:: Text

        .. code-block:: shell

            # Definition of the suite test.
            suite test
                edit ECF_INCLUDE "{{HOME}}/course" # replace '{{HOME}}' appropriately
                edit ECF_HOME    "{{HOME}}/course"
                family f1
                    edit SLEEP 20

                    [... previously defined tasks omitted ...]

                    task query
                        label query ""
                endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/query-state.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Add the new task script :file:`query.ecf`, as shown above.
#. Modify suite definition to add a new task :code:`query` to the family :code:`f1`, as shown above.
#. Replace the :term:`suite`, using:

   .. tabs::

      .. tab:: Text

         .. code-block:: shell

            ecflow_client --suspend /test
            ecflow_client --replace /test test.def

      .. tab:: Python

         .. code-block:: shell

            python3 test.py
            python3 client.py

#. Observe the tasks in :term:`ecflow_ui`
#. Modify the task script to query variable :code:`SLEEP`, and add this variable to the query label.

.. note::

    Although a variable can be made accessible in the script by using :code:`%VAR%`, using the query command
    allows to dynamically access the current value or a value from a different server.
