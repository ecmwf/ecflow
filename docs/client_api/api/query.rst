
.. _query_cli:

query
/////

::

   
   query
   -----
   
   Query the status of attributes
    i.e state,dstate,repeat,event,meter,label,variable or trigger expression without blocking
    - state     return [unknown | complete | queued |             aborted | submitted | active] to standard out
    - dstate    return [unknown | complete | queued | suspended | aborted | submitted | active] to standard out
    - repeat    returns current value as a string to standard out
    - event     return 'set' | 'clear' to standard out
    - meter     return value of the meter to standard out
    - limit     return current value of limit to standard out
    - limit_max return limit max value to standard out
    - label     return new value otherwise the old value
    - variable  return value of the variable, repeat or generated variable to standard out,
                will search up the node tree
    - trigger   returns 'true' if the expression is true, otherwise 'false'
   
   If this command is called within a '.ecf' script we will additionally log the task calling this command
   This is required to aid debugging for excessive use of this command
   The command will fail if the node path to the attribute does not exist in the definition and if:
    - repeat   The repeat is not found
    - event    The event is not found
    - meter    The meter is not found
    - limit/limit_max The limit is not found
    - label    The label is not found
    - variable No user or generated variable or repeat of that name found on node, or any of its parents
    - trigger  Trigger does not parse, or reference to nodes/attributes in the expression are not valid
   Arguments:
     arg1 = [ state | dstate | repeat | event | meter | label | variable | trigger | limit | limit_max ]
     arg2 = <path> | <path>:name where name is name of a event, meter, label, limit or variable
     arg3 = trigger expression | prev | next # prev,next only used when arg1 is repeat
   
   Usage:
    ecflow_client --query state /                                     # return top level state to standard out
    ecflow_client --query state /path/to/node                         # return node state to standard out
    ecflow_client --query dstate /path/to/node                        # state that can included suspended
    ecflow_client --query repeat /path/to/node                        # return the current value as a string
    ecflow_client --query repeat /path/to/node prev                   # return the previous value as a string
    ecflow_client --query repeat /path/to/node next                   # return the next value as a string
    ecflow_client --query event /path/to/task/with/event:event_name   # return set | clear to standard out
    ecflow_client --query meter /path/to/task/with/meter:meter_name   # returns the current value of the meter to standard out
    ecflow_client --query limit /path/to/task/with/limit:limit_name   # returns the current value of the limit to standard out
    ecflow_client --query limit_max /path/to/task/with/limit:limit_name # returns the max value of the limit to standard out
    ecflow_client --query label /path/to/task/with/label:label_name   # returns the current value of the label to standard out
    ecflow_client --query variable /path/to/task/with/var:var_name    # returns the variable value to standard out
    ecflow_client --query trigger /path/to/node/with/trigger "/suite/task == complete" # return true if expression evaluates false otherwise
   
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable secure communication between client and server.            |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   
