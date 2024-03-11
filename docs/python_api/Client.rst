ecflow.Client
/////////////


.. py:class:: Client
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Class client provides an interface to communicate with the :term:`ecflow_server`.:

.. code-block:: shell

   Client(
      string host, # The server name. Cannot be empty.
      string port  # The port on the server, must be unique to the server
   )

   Client(
      string host, # The server name. Cannot be empty.
      int port     # The port on the server, must be unique to the server
   )

   Client(
      string host_port, # Expects <host>:<port> || <host>@<port>
   )

The client reads in the following environment variables.
For child commands,(i.e these are commands called in the .ecf/jobs files), these variables are used.
For the python interface these environment variable are not really applicable but documented for completeness:

* ECF_NAME <string> : Full path name to the task
* ECF_PASS <string> : The jobs password, allocated by server, then used by server to authenticate client request
* ECF_TRYNO <int>   : The number of times the job has run. Used in file name generation. Set to 1 by begin() and re-queue commands.
* ECF_TIMEOUT <int> : Max time in seconds for client to deliver message to main server
* ECF_HOSTFILE <string> : File that lists alternate hosts to try, if connection to main host fails
* ECF_DENIED <any> : Provides a way for child to exit with an error, if server denies connection. Avoids 24hr wait. Note: when you have hundreds of tasks, using this approach requires a lot of manual intervention to determine job status
* NO_ECF <any> : If set exit's immediately with success. Used to test jobs without communicating with server

The following environment variables are used by the python interface and child commands

* ECF_HOST  <string>   : The host name of the main server. defaults to 'localhost'
* ECF_PORT  <int>      : The TCP/IP port to call on the server. Must be unique to a server

The ECF_HOST and ECF_PORT can be overridden by using the Constructor or set_host_port() member function.
For optimal usage it is best to reuse the same Client rather than recreating for each client server interaction
By default the Client interface will throw exceptions for error's.

Usage:

.. code-block:: python

   try:
       ci = Client('localhost:3150')   # for errors will throw RuntimeError
       ci.terminate_server()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.alter( (Client)arg1, (list)paths, (str)alter_type, (str)attribute_type [, (str)name='' [, (str)value='']]) -> None :
   :module: ecflow

Alter command is used to change the attributes of a node
    ::
    
       void alter(
          (list | string ) paths(s) : A single or list of paths. Path name to the node whose attributes are to be changed
          string alter_type         : This must be one of [ 'add' | 'change' | 'delete' | 'set_flag' | 'clear_flag' ]
          string attr_type          : This varies according to the 'alter_type'. valid strings are:
             add    : [ variable,time,today,date,day,label,zombie,late]
             delete : [ variable,time,today,date,day,label,cron,event,meter,trigger,complete,repeat,limit,inlimit,limit_path,zombie,late]
             change : [ variable,clock-type,clock-gain,event,meter,label,trigger,complete,repeat,limit-max,limit-value,late,time,today]
             set_flag and clear_flag:
                      [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed | statuscmd_failed | killcmd_failed |
                        no_script | killed | status | late | message | complete | queue_limit | task_waiting | locked | zombie ]
          string name               : used to locate the attribute, when multiple attributes of the same type,
                                      optional for some.i.e. when changing, attributes like variable,meter,event,label,limits
          string value              : Only used when 'changing' a attribute. provides a new value
       )
    
    Exceptions can be raised because:
    
    - absolute_node_path does not exist.
    - parsing fails
    
    The following describes the parameters in more detail:
    
    .. code-block:: shell
    
     add variable variable_name variable_value
     add time   format    # when format is +hh:mm | hh:mm | hh:mm(start) hh:mm(finish) hh:mm(increment)
     add today  format    # when format is +hh:mm | hh:mm | hh:mm(start) hh:mm(finish) hh:mm(increment)
     add date   format    # when format dd.mm.yyyy, can use '*' to indicate any day,month, or year
     add day    format    # when format is one of [ sunday,monday,tuesday,wednesday,friday,saturday ]
     add zombie format    # when format is one of <zombie-type>:<child>:<server-action>|<client-action>:<zombie-lifetime>
                          #  <zombie-type> := [ user | ecf | path ]
                          #  <child> := [ init, event, meter, label, wait, abort, complete ]
                          #  <server-action> := [ adopt | delete ]
                          #  <client-action> := [ fob | fail | block(default) ]
                          #  <zombie-lifetime>:= lifetime of zombie in the server
                          # example
                          # add zombie :label:fob:0   # fob all child label request, & remove zombie as soon as possible
    
     delete variable name # if name is empty will delete -all- variables on the node
     delete time name     # To delete a specific time, enter the time in same format as show above,
                          # or as specified in the defs file
                          # an empty name will delete all time attributes on the node
     delete today name    # To delete a specific today attribute, enter in same format as show above,
                          # or as specified in the defs file.
                          # an empty name will delete all today attributes on the node
     delete date name     # To delete a specific date attribute, enter in same format as show above,
                          # or as specified in the defs file
                          # an empty name will delete all date attributes on the node
     delete day name      # To delete a specific day attribute, enter in same format as show above,
                          # or as specified in the defs file
                          # an empty name will delete all day attributes on the node
     delete cron name     # To delete a specific cron attribute, enter in same as specified in the defs file
                          # an empty name will delete all cron attributes on the node
     delete event name    # To delete a specific event, enter name or number
                          # an empty name will delete all events on the node
     delete meter name    # To delete a specific meter , enter the meter name
                          # an empty name will delete all meter on the node 
     delete label name    # To delete a specific label , enter the label name
                          # an empty name will delete all labels on the node
     delete limit name    # To delete a specific limit , enter the limit name
                          # an empty name will delete all limits on the node
     delete inlimit name  # To delete a specific inlimit , enter the inlimit name
                          # an empty name will delete all inlimits on the node
     delete limit_path limit_name limit_path # To delete a specific limit path
     delete trigger       # A node can only have one trigger expression, hence the name is not required
     delete complete      # A node can only have one complete expression, hence the name is not required
     delete repeat        # A node can only have one repeat, hence the name is not required
    
     change variable name value    # Find the specified variable, and set the new value.
     change clock_type name        # The name must be one of 'hybrid' or 'real'.
     change clock_gain name        # The gain must be convertible to an integer.
     change clock_sync name        # Sync suite calendar with the computer.
     change event name(optional )  # if no name specified the event is set, otherwise name must be 'set' or 'clear'
     change meter name value       # The meter value must be convertible to an integer, and between meter min-max range.
     change label name value       # sets the label
     change trigger name           # The name must be expression. returns an error if the expression does not parse
     change complete name          # The name must be expression. returns an error if the expression does not parse
     change limit_max name value   # Sets the max value of the limit. The value must be convertible to an integer
     change limit_value name value # Sets the consumed tokens to value.The value must be convertible to an integer
     change repeat value           # If the repeat is a date, then the value must be a valid YMD ( ie. yyyymmdd)
                                   # and be convertible to an integer, additionally the value must be in range
                                   # of the repeat start and end dates. Like wise for repeat integer. For repeat
                                   # string and enum,  the name must either be an integer, that is a valid index or
                                   # if it is a string, it must correspond to one of enum's or strings list
    
    Usage:
    
    .. code-block:: python
    
      try:
         ci = Client()     # use default host(ECF_HOST) & port(ECF_PORT)
         ci.alter('/suite/task','change','trigger','b2 == complete')
      except RuntimeError, e:
         print(str(e))
    

alter( (Client)arg1, (str)abs_node_path, (str)alter_type, (str)attribute_type [, (str)name='' [, (str)value='']]) -> None


.. py:method:: Client.archive( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Archives suite or family nodes. Saves the suite/family nodes to disk, and then removes then from the definition
    This saves memory in the server, when dealing with huge definitions that are not needed.
    If the node is re-queued or begun, it is automatically restored
    Use --restore to reload the archived nodes manually
    The nodes are saved to ECF_HOME/ECF_NAME.check
    Usage::
    
       string archive(
          list paths # List of paths.
       )
       string archive(
          string absolute_node_path
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           print ci.archive('/suite1')
       except RuntimeError, e:
           print str(e)
    

archive( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.begin_all_suites( (Client)arg1 [, (bool)force=False]) -> int :
   :module: ecflow

Begin playing all the :term:`suite`\ s in the :term:`ecflow_server`

.. Note:: using the force option may cause :term:`zombie`\ s if suite has running jobs

::

   void begin_all_suites(
      [(bool)force=False] : bypass the checks for submitted and active jobs
   )

Usage:

.. code-block:: python

   try:
       ci = Client()             # use default host(ECF_HOST) & port(ECF_PORT)
       ci.begin_all_suites()     # begin playing all the suites
       ci.begin_all_suites(True) # begin playing all the suites, by passing checks
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.begin_suite( (Client)arg1, (str)suite_name [, (bool)force=False]) -> int :
   :module: ecflow

Begin playing the chosen :term:`suite`\ s in the :term:`ecflow_server`

.. Note:: using the force option may cause :term:`zombie`\ s if suite has running jobs

::

   void begin_suite
      string suite_name     : begin playing the given suite
      [(bool)force=False]   : bypass the checks for submitted and active jobs
   )

Usage:

.. code-block:: python

   try:
       ci = Client()                  # use default host(ECF_HOST) & port(ECF_PORT)
       ci.begin_suite('/suite1')      # begin playing suite '/suite1'
       ci.begin_suite('/suite1',True) # begin playing suite '/suite1' bypass any checks   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.ch_add( (Client)arg1, (int)arg2, (list)arg3) -> None :
   :module: ecflow

Add a set of suites, to an existing registered handle
    
    When dealing with large definitions, where a user is only interested in a small subset
    of suites, registering them, improves download performance from the server.
    Registered suites have an associated handle.
    ::
    
      integer ch_add(
         integer handle   : the handle obtained after ch_register
         list suite_names : list of strings representing suite names
      )
      integer ch_add(
         list suite_names : list of strings representing suite names
      )
    
    Usage:
    
    .. code-block:: python
    
       try:
           with Client() as ci:       # use default host(ECF_HOST) & port(ECF_PORT)
              ci.ch_register(True,[]) # register interest in any new suites
              ci.ch_add(['s1','s2'])  # add suites s1,s2 to the last added handle
       except RuntimeError, e:
           print(str(e))
    
    

ch_add( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.ch_auto_add( (Client)arg1, (int)arg2, (bool)arg3) -> int :
   :module: ecflow

Change an existing handle so that new suites can be added automatically
    
    When dealing with large definitions, where a user is only interested in a small subset
    of suites, registering them, improves download performance from the server.
    Registered suites have an associated handle.
    ::
    
       void ch_auto_add(
          integer handle,         : the handle obtained after ch_register
          bool auto_add_new_suite : automatically add new suites, this handle when they are created
       )
       void ch_auto_add(
          bool auto_add_new_suite : automatically add new suites using handle on the client
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           with Client() as ci:                     # use default host(ECF_HOST) & port(ECF_PORT)
              ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
              ci.ch_auto_add( False )               # disable adding newly created suites to my handle
       except RuntimeError, e:
           print(str(e))
    
    

ch_auto_add( (Client)arg1, (bool)arg2) -> int


.. py:method:: Client.ch_drop( (Client)arg1, (int)arg2) -> int :
   :module: ecflow

Drop/de-register the client handle.
    
    When dealing with large definitions, where a user is only interested in a small subset
    of suites, registering them, improves download performance from the server.
    Registered suites have an associated handle.
    Client must ensure un-used handle are dropped otherwise they will stay, in the :term:`ecflow_server`
    ::
    
       void ch_drop(
          int client_handle : The handle must be an integer that is > 0
       )
       void ch_drop()       : Uses the local handle stored on the client, from last call to ch_register()
    
    Exception:
    
    - RunTimeError thrown if handle has not been previously registered
    
    Usage:
    
    .. code-block:: python
    
        try:
          ci = Client()                     # use default host(ECF_HOST) & port(ECF_PORT)
          ci.ch_register(False,['s1','s2'])
          while( 1 ):
             # get incremental changes to suites s1 & s2, uses data stored on ci/defs
             ci.sync_local()                # will only retrieve data for suites s1 & s2
             update(ci.get_defs())
        finally:
          ci.ch_drop()
    
    To automatically drop the handle(Preferred) use with
    :
    
    .. code-block:: python
    
       try:
           with Client() as ci:
              ci.ch_register(False,['s1','s2'])
              while( 1 ):
                  # get incremental changes to suites s1 & s2, uses data stored on ci/defs
                  ci.sync_local()                # will only retrieve data for suites s1 & s2
                  update(ci.get_defs())
           ....                                  # will automatically drop last handle
       except RuntimeError, e:
           print(str(e))
    

ch_drop( (Client)arg1) -> int


.. py:method:: Client.ch_drop_user( (Client)arg1, (str)arg2) -> int :
   :module: ecflow

Drop/de-register all handles associated with user.

When dealing with large definitions, where a user is only interested in a small subset
of suites, registering them, improves download performance from the server.
Registered suites have an associated handle.
Client must ensure un-used handle are dropped otherwise they will stay, in the :term:`ecflow_server`
::

   void ch_drop_user(
        string user   # If empty string will drop current user
   )

Exception:

- RunTimeError thrown if handle has not been previously registered

Usage:

.. code-block:: python

  try:
      ci = Client()                     # use default host(ECF_HOST) & port(ECF_PORT)
      ci.ch_register(False,['s1','s2'])
      while( 1 ):
         # get incremental changes to suites s1 & s2, uses data stored on ci/defs
         update(ci.get_defs())
  finally:
      ci.ch_drop_user('') # drop all handles associated with current user



.. py:method:: Client.ch_handle( (Client)arg1) -> int :
   :module: ecflow

Register interest in a set of :term:`suite`\ s.

If a definition has lots of suites, but the client is only interested in a small subset.
Then using this command can reduce network bandwidth and synchronisation will be quicker.
This command will create a client handle. This handle is held locally on the :py:class:`ecflow.Client`, and
can be used implicitly by ch_drop(),ch_add(),ch_remove() and ch_auto_add().
Registering a client handle affects the news() and sync() commands::

   void ch_register(
      bool auto_add_new_suites : true means add new suites to my list, when they are created
      list suite_names         : should be a list of suite names, names not in the definition are ignored
   )

Usage:

.. code-block:: python

   try:
       ci = Client()
       suite_names = [ 's1', 's2', 's3' ]
       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites
       ci.ch_register(False,suite_names)   # register interest in suites s1,s2,s3 only
   except RuntimeError, e:
       print(str(e))

The client 'ci' will hold locally the client handle. Since we have made multiple calls to register
a handle, the variable 'ci' will hold the handle for the last call only.
The handle associated with the suite can be manually retrieved:

.. code-block:: python

   try:
       ci = Client()
       ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
       client_handle = ci.ch_handle()        # get the handle associated with last call to ch_register
       ....                                  # after a period of time
   except RuntimeError, e:
       print(str(e))
   finally:
       ci.ch_drop( client_handle )           # de-register the handle

To automatically drop the handle(preferred) use with:

.. code-block:: python

   try:
       with Client() as ci:
          ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
          client_handle = ci.ch_handle()        # get the handle associated with last call to ch_register
       ....                                     # will automatically drop last handle
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.ch_register( (Client)arg1, (bool)arg2, (list)arg3) -> None :
   :module: ecflow

Register interest in a set of :term:`suite`\ s.

If a definition has lots of suites, but the client is only interested in a small subset.
Then using this command can reduce network bandwidth and synchronisation will be quicker.
This command will create a client handle. This handle is held locally on the :py:class:`ecflow.Client`, and
can be used implicitly by ch_drop(),ch_add(),ch_remove() and ch_auto_add().
Registering a client handle affects the news() and sync() commands::

   void ch_register(
      bool auto_add_new_suites : true means add new suites to my list, when they are created
      list suite_names         : should be a list of suite names, names not in the definition are ignored
   )

Usage:

.. code-block:: python

   try:
       ci = Client()
       suite_names = [ 's1', 's2', 's3' ]
       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites
       ci.ch_register(False,suite_names)   # register interest in suites s1,s2,s3 only
   except RuntimeError, e:
       print(str(e))

The client 'ci' will hold locally the client handle. Since we have made multiple calls to register
a handle, the variable 'ci' will hold the handle for the last call only.
The handle associated with the suite can be manually retrieved:

.. code-block:: python

   try:
       ci = Client()
       ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
       client_handle = ci.ch_handle()        # get the handle associated with last call to ch_register
       ....                                  # after a period of time
   except RuntimeError, e:
       print(str(e))
   finally:
       ci.ch_drop( client_handle )           # de-register the handle

To automatically drop the handle(preferred) use with:

.. code-block:: python

   try:
       with Client() as ci:
          ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
          client_handle = ci.ch_handle()        # get the handle associated with last call to ch_register
       ....                                     # will automatically drop last handle
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.ch_remove( (Client)arg1, (int)arg2, (list)arg3) -> None :
   :module: ecflow

Remove a set of suites, from an existing handle
    
    When dealing with large definitions, where a user is only interested in a small subset
    of suites, registering them, improves download performance from the server.
    Registered suites have an associated handle.
    ::
    
      integer ch_remove(
         integer handle   : the handle obtained after ch_register
         list suite_names : list of strings representing suite names
      )
      integer ch_remove(
         list suite_names : list of strings representing suite names
      )
    
    Usage:
    
    .. code-block:: python
    
       try:
           with Client() as ci:                     # use default host(ECF_HOST) & port(ECF_PORT)
              ci.ch_register(True,['s1','s2','s3']) # register interest in suites s1,s2,s3 and any new suites
              ci.ch_remove( ['s1'] )                # remove suites s1 from the last added handle
       except RuntimeError, e:
           print(str(e))
    
    

ch_remove( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.ch_suites( (Client)arg1) -> None :
   :module: ecflow

Writes to standard out the list of registered handles and the suites they reference.

When dealing with large definitions, where a user is only interested in a small subset
of suites, registering them, improves download performance from the server.
Registered suites have an associated handle.


.. py:property:: Client.changed_node_paths
   :module: ecflow

After a call to sync_local() we can access the list of nodes that changed

The returned list consists of node paths. *IF* the list is empty assume that
whole definition changed. This should be expected after the first call to sync_local()
since that always retrieves the full definition from the server::

   void changed_node_paths()


Usage:

.. code-block:: python

   try:
       ci = Client()                          # use default host(ECF_HOST) & port(ECF_PORT)
       if ci.news_local():                    # has the server changed
          print('Server Changed')             # server changed bring client in sync with server
          ci.sync_local()                     # get the full definition from the server if first time
                                              # otherwise apply incremental changes to Client definition,
                                              # bringing it in sync with the server definition
          defs = ci.get_defs()                # get the updated/synchronised definition
          for path in ci.changed_node_paths:
              if path == '/':                 # path '/' represent change to server node/defs
                 print('defs changed')        # defs state change or user variables changed
              else:
                 node = defs.find_abs_node(path)

         # if changed_node_paths is empty, then assume entire definition changed
         print(defs)                         # print the synchronised definition. Should be same as server
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.check( (Client)arg1, (str)arg2) -> str :
   :module: ecflow

Check :term:`trigger` and :term:`complete expression`\ s and :term:`limit`\ s
    
    The :term:`ecflow_server` does not store :term:`extern`\ s. Hence all unresolved references
    are reported as errors.
    Returns a non empty string for any errors or warning
    ::
    
       string check(
          list paths # List of paths.
       )
       string check(
          string absolute_node_path
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           print(ci.check('/suite1'))
       except RuntimeError, e:
           print(str(e))
    

check( (Client)arg1, (list)arg2) -> str


.. py:method:: Client.checkpt( (Client)arg1 [, (CheckPt)mode=ecflow.CheckPt.UNDEFINED [, (int)check_pt_interval=0 [, (int)check_pt_save_alarm_time=0]]]) -> int :
   :module: ecflow

Request the :term:`ecflow_server` :term:`check point`\ s the definition held in the server immediately

This effectively saves the definition held in the server to disk,
in a platform independent manner. This is the default when no arguments are specified.
The saved file will include node state, passwords, etc.
The default file name is <host>.<port>.ecf.check and is saved in ECF_HOME directory.
The :term:`check point` file name can be overridden via ECF_CHECK server environment variable.
The back up :term:`check point` file name can be overridden via ECF_CHECKOLD server environment variable::

   void checkpt(
     [(CheckPt::Mode)mode=CheckPt.UNDEFINED]
                         : Must be one of [ NEVER, ON_TIME, ALWAYS, UNDEFINED ]
                           NEVER  :  Never check point the definition in the server
                           ON_TIME:  Turn on automatic check pointing at interval stored on server
                                     or with interval specified as the second argument
                           ALWAYS:   Check point at any change in node tree, *NOT* recommended for large definitions
                           UNDEFINED:The default, which allows for immediate check pointing, or alarm setting
     [(int)interval=120] : This specifies the interval in seconds when server should automatically check pt.
                           This will only take effect if mode is on_time/CHECK_ON_TIME
                           Should ideally be a value greater than 60 seconds, default is 120 seconds
     [(int)alarm=30]     : Specifies check pt save alarm time. If saving the check pt takes longer than
                           the alarm time, then the late flag is set on the server.
                           This flag will need to be cleared manually.
   )

.. Note:: When the time taken to save the check pt is excessive, it can interfere with job scheduling.
          It may be an indication of the following:

          * slow disk
          * file system full
          * The definition is very large and needs to split


Usage:

.. code-block:: python

   try:
       ci = Client()                      # use default host(ECF_HOST) & port(ECF_PORT)
       ci.checkpt()                       # Save the definition held in the server to disk
       ci.checkpt(CheckPt.NEVER)          # Switch off check pointing
       ci.checkpt(CheckPt.ON_TIME)        # Start automatic check pointing at the interval stored in the server
       ci.checkpt(CheckPt.ON_TIME,180)    # Start automatic check pointing every 180 seconds
       ci.checkpt(CheckPt.ALWAYS)         # Check point at any state change in node tree. *not* recommended for large defs
       ci.checkpt(CheckPt.UNDEFINED,0,35) # Change check point save time alarm to 35 seconds
                                          # With these arguments mode and interval remain unchanged
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.child_abort( (Client)arg1 [, (str)reason='']) -> None :
   :module: ecflow

Child command,notify server job has aborted, can provide an optional reason


.. py:method:: Client.child_complete( (Client)arg1) -> None :
   :module: ecflow

Child command,notify server job has complete


.. py:method:: Client.child_event( (Client)arg1, (str)event_name [, (bool)value=True]) -> None :
   :module: ecflow

Child command,notify server event occurred, requires the event name


.. py:method:: Client.child_init( (Client)arg1) -> None :
   :module: ecflow

Child command,notify server job has started


.. py:method:: Client.child_label( (Client)arg1, (str)arg2, (str)arg3) -> None :
   :module: ecflow

Child command,notify server label changed, requires label name, and new value


.. py:method:: Client.child_meter( (Client)arg1, (str)arg2, (int)arg3) -> None :
   :module: ecflow

Child command,notify server meter changed, requires meter name and value


.. py:method:: Client.child_queue( (Client)arg1, (str)queue_name, (str)action [, (str)step='' [, (str)path_to_node_with_queue='']]) -> str :
   :module: ecflow

Child command,active:return current step as string, then increment index, requires queue name, and optionally path to node with the queue


.. py:method:: Client.child_wait( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Child command,wait for expression to come true


.. py:method:: Client.clear_log( (Client)arg1) -> int :
   :module: ecflow

Request the :term:`ecflow_server` to clear log file.

Log file will be empty after this call.


Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       ci.clear_log()   # log file is now empty
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.debug( (Client)arg1, (bool)arg2) -> None :
   :module: ecflow

enable/disable client api debug


.. py:method:: Client.debug_server_off( (Client)arg1) -> int :
   :module: ecflow

Disable server debug


.. py:method:: Client.debug_server_on( (Client)arg1) -> int :
   :module: ecflow

Enable server debug, Will dump to standard out on server host.


.. py:method:: Client.delete( (Client)arg1, (str)abs_node_path [, (bool)force=False]) -> int :
   :module: ecflow

Delete the :term:`node` (s) specified.
    
    If a node is :term:`submitted` or :term:`active`, then a Exception will be raised.
    To force the deletion at the expense of :term:`zombie` creation, then set
    the force parameter to true
    ::
    
       void delete(
          list paths          : List of paths.
          [(bool)force=False] : If true delete even if in 'active' or 'submitted' states
                                Which risks creating zombies.
       )
       void delete(
          string absolute_node_path: Path name of node to delete.
          [(bool)force=False]       : If true delete even if in 'active' or 'submitted' states
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()                     # use default host(ECF_HOST) & port(ECF_PORT)
           ci.delete('/s1/f1/task1')
    
           paths = ['/s1/f1/t1','/s2/f1/t2']
           ci.delete(paths)                  # delete all tasks specified in the paths
       except RuntimeError, e:
           print(str(e))
    

delete( (Client)arg1, (list)paths [, (bool)force=False]) -> None


.. py:method:: Client.delete_all( (Client)arg1 [, (bool)force=False]) -> int :
   :module: ecflow

Delete all the :term:`node`\ s held in the :term:`ecflow_server`.

The :term:`suite definition` in the server will be empty, after this call. **Use with care**
If a node is :term:`submitted` or :term:`active`, then a Exception will be raised.
To force the deletion at the expense of :term:`zombie` creation, then set
the force parameter to true
::

   void delete_all(
      [(bool)force=False] : If true delete even if in 'active' or 'submitted' states
                            Which risks creating zombies.
   )

Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       ci.delete_all()
       ci.get_server_defs()
   except RuntimeError, e:
       print(str(e));    # expect failure since all nodes deleted


.. py:method:: Client.disable_ssl( (Client)arg1) -> None :
   :module: ecflow

ecFlow client and server are SSL enabled. To use SSL choose between:
  1. export ECF_SSL=1              # search for server.crt otherwise <host>.<port>.crt
  2. export ECF_SSL=<host>.<port>  # Use server specific certificates <host>.<port>.***
  3. use --ssl           # argument on ecflow_client/ecflow_server, same as option 1.
                         # Typically ssl server can be started with ecflow_start.sh -s
  4. Client.enable_ssl() # for python client

ecFlow expects the certificates to be in directory $HOME/.ecflowrc/ssl
The certificates can be shared if you have multiple servers running on
the same machine. In this case use ECF_SSL=1, then
ecflow_server expects the following files in $HOME/.ecflowrc/ssl

   - dh2048.pem
   - server.crt
   - server.key
   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key

ecflow_client expects the following files in : $HOME/.ecflowrc/ssl

   - server.crt (this must be the same as server)

Alternatively you can have different setting for each server ECF_SSL=<host>.<port>
Then server expect files of the type:

   - <host>.<port>.pem
   - <host>.<port>.crt
   - <host>.<port>.key
   - <host>.<port>.passwd (optional)

and client expect files of the type:

   - <host>.<port>.crt  # as before this must be same as the server

The server/client will automatically check existence of both variants,
but will give preference to NON <host>.<port>.*** variants first, when ECF_SSL=1
The following steps, show you how to create the certificate files.
This may need to be adapted if you want to use <host>.<port>.***

- Generate a password protected private key. This will request a pass phrase.
  This key is a 1024 bit RSA key which is encrypted using Triple-DES and stored
  in a PEM format so that it is readable as ASCII text

     > openssl genrsa -des3 -out server.key 1024   # Password protected private key
- Additional security.
  If you want additional security, create a file called 'server.passwd' and add the pass phrase to the file.
  Then set the file permission so that file is only readable by the server process.
  Or you can choose to remove password requirement. In that case we don't need server.passwd file.

     > cp server.key server.key.secure
     > openssl rsa -in server.key.secure -out server.key  # remove password requirement
- Sign certificate with private key (self signed certificate).Generate Certificate Signing Request(CSR).
  This will prompt with a number of questions.
  However please ensure 'common name' matches the host where your server is going to run.

     > openssl req -new -key server.key -out server.csr # Generate Certificate Signing Request(CSR)
- Generate a self signed certificate CRT, by using the CSR and private key.

     > openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt

- Generate dhparam file. ecFlow expects 2048 key.
     > openssl dhparam -out dh2048.pem 2048


.. py:method:: Client.edit_script_edit( (Client)arg1, (str)arg2) -> str :
   :module: ecflow

get script for Edit
    


.. py:method:: Client.edit_script_preprocess( (Client)arg1, (str)arg2) -> str :
   :module: ecflow

get script for Edit Preprocess
    


.. py:method:: Client.edit_script_submit( (Client)arg1, (str)arg2, (list)arg3, (list)arg4, (bool)arg5, (bool)arg6) -> int :
   :module: ecflow

submit script from Edit/Preprocess 
to run as alias or not:

.. code-block:: python

 ci = Client()
 ci.edit_script_submit(path_to_task,
                       used_variables, # array name=value
                       file_contents,  # strings array
                       alias, # bool False,
                       run  # bool true
                      )


.. py:method:: Client.enable_ssl( (Client)arg1) -> None :
   :module: ecflow

ecFlow client and server are SSL enabled. To use SSL choose between:
  1. export ECF_SSL=1              # search for server.crt otherwise <host>.<port>.crt
  2. export ECF_SSL=<host>.<port>  # Use server specific certificates <host>.<port>.***
  3. use --ssl           # argument on ecflow_client/ecflow_server, same as option 1.
                         # Typically ssl server can be started with ecflow_start.sh -s
  4. Client.enable_ssl() # for python client

ecFlow expects the certificates to be in directory $HOME/.ecflowrc/ssl
The certificates can be shared if you have multiple servers running on
the same machine. In this case use ECF_SSL=1, then
ecflow_server expects the following files in $HOME/.ecflowrc/ssl

   - dh2048.pem
   - server.crt
   - server.key
   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key

ecflow_client expects the following files in : $HOME/.ecflowrc/ssl

   - server.crt (this must be the same as server)

Alternatively you can have different setting for each server ECF_SSL=<host>.<port>
Then server expect files of the type:

   - <host>.<port>.pem
   - <host>.<port>.crt
   - <host>.<port>.key
   - <host>.<port>.passwd (optional)

and client expect files of the type:

   - <host>.<port>.crt  # as before this must be same as the server

The server/client will automatically check existence of both variants,
but will give preference to NON <host>.<port>.*** variants first, when ECF_SSL=1
The following steps, show you how to create the certificate files.
This may need to be adapted if you want to use <host>.<port>.***

- Generate a password protected private key. This will request a pass phrase.
  This key is a 1024 bit RSA key which is encrypted using Triple-DES and stored
  in a PEM format so that it is readable as ASCII text

     > openssl genrsa -des3 -out server.key 1024   # Password protected private key
- Additional security.
  If you want additional security, create a file called 'server.passwd' and add the pass phrase to the file.
  Then set the file permission so that file is only readable by the server process.
  Or you can choose to remove password requirement. In that case we don't need server.passwd file.

     > cp server.key server.key.secure
     > openssl rsa -in server.key.secure -out server.key  # remove password requirement
- Sign certificate with private key (self signed certificate).Generate Certificate Signing Request(CSR).
  This will prompt with a number of questions.
  However please ensure 'common name' matches the host where your server is going to run.

     > openssl req -new -key server.key -out server.csr # Generate Certificate Signing Request(CSR)
- Generate a self signed certificate CRT, by using the CSR and private key.

     > openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt

- Generate dhparam file. ecFlow expects 2048 key.
     > openssl dhparam -out dh2048.pem 2048


.. py:method:: Client.flush_log( (Client)arg1) -> int :
   :module: ecflow

Request the :term:`ecflow_server` to flush and then close log file

It is best that the server is :term:`shutdown` first, as log file will be reopened
whenever a command wishes to log any changes.

Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       ci.flush_log()   # Log can now opened by external program
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.force_event( (Client)arg1, (str)arg2, (str)arg3) -> None :
   :module: ecflow

Set or clear a :term:`event`
    ::
    
       void force_event(
          string absolute_node_path:event: Path name to node: < event name | number>
                                           The paths must begin with a leading '/'
          string signal                  : [ set | clear ]
       )
       void force_event(
          list paths    : A list of absolute node paths. Each path must include a event name
                          The paths must begin with a leading '/'
          string signal : [ set | clear ]
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.force_event('/s1/f1:event_name','set')
    
           # Set or clear a event for a list of events
           paths = [ '/s1/t1:ev1', '/s2/t2:ev2' ]
           ci.force_event(paths,'clear')
       except RuntimeError, e:
           print(str(e))
    

force_event( (Client)arg1, (list)arg2, (str)arg3) -> None


.. py:method:: Client.force_state( (Client)arg1, (str)arg2, (State)arg3) -> None :
   :module: ecflow

Force a node(s) to a given state
    
    When a :term:`task` is set to :term:`complete`, it may be automatically re-queued if it has
    multiple time :term:`dependencies`. In the specific case where a task has a single
    time dependency and we want to interactively set it to :term:`complete`
    a flag is set so that it is not automatically re-queued when set to complete.
    The flag is applied up the node hierarchy until reach a node with a :term:`repeat`
    or :term:`cron` attribute. This behaviour allow :term:`repeat` values to be incremented interactively.
    A :term:`repeat` attribute is incremented when all the child nodes are :term:`complete`
    in this case the child nodes are automatically re-queued
    ::
    
       void force_state(
          string absolute_node_path: Path name to node. The path must begin with a leading '/'
          State::State state       : [ unknown | complete | queued | submitted | active | aborted ]
       )
       void force_state(
          list paths         : A list of absolute node paths. The paths must begin with a leading '/'
          State::State state : [ unknown | complete | queued | submitted | active | aborted ]
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           # force a single node to complete
           ci.force_state('/s1/f1',State.complete)
    
           # force a list of nodes to complete
           paths = [ '/s1/t1', '/s1/t2', '/s1/f1/t1' ]
           ci.force_state(paths,State.complete)
       except RuntimeError, e:
           print(str(e))
    
    Effect:
    
    Lets see the effect of forcing complete on the following defs
    
    .. code-block:: shell
    
       suite s1
          task t1; time 10:00             # will complete straight away
          task t2; time 10:00 13:00 01:00 # will re-queue 3 times and complete on fourth 
    
    In the last case (task t2) after each force complete, the next time slot is incremented.
    This can be seen by calling the Why command.

force_state( (Client)arg1, (list)arg2, (State)arg3) -> None


.. py:method:: Client.force_state_recursive( (Client)arg1, (str)arg2, (State)arg3) -> None :
   :module: ecflow

Force node(s) to a given state recursively
    ::
    
       void force_state_recursive(
          string absolute_node_path: Path name to node.The paths must begin with a leading '/'
          State::State state       : [ unknown | complete | queued | submitted | active | aborted ]
       )
       void force_state_recursive(
          list  paths         : A list of absolute node paths.The paths must begin with a leading '/'
          State::State state  : [ unknown | complete | queued | submitted | active | aborted ]
       )
    
    Usage:
    
    .. code-block:: python
    
      try:
          ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
          ci.force_state_recursive('/s1/f1',State.complete)
    
          # recursively force a list of nodes to complete
          paths = [ '/s1', '/s2', '/s1/f1/t1' ]
          ci.force_state_recursive(paths,State.complete)
      except RuntimeError, e:
          print(str(e))
    

force_state_recursive( (Client)arg1, (list)arg2, (State)arg3) -> None


.. py:method:: Client.free_all_dep( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Free all :term:`trigger`, :term:`date` and all time(:term:`day`, :term:`today`, :term:`cron`,etc) :term:`dependencies`
    ::
    
       void free_all_dep(
          string absolute_node_path : Path name to node
       )
    
    After freeing the time related dependencies (i.e time,today,cron)
    the next time slot will be missed.
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           ci.free_all_dep('/s1/task')
       except RuntimeError, e:
           print(str(e))
    

free_all_dep( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.free_date_dep( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Free :term:`date` :term:`dependencies` for a :term:`node`
    ::
    
       void free_date_dep(
          string absolute_node_path : Path name to node
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           ci.free_date_dep('/s1/task')
       except RuntimeError, e:
           print(str(e))
    

free_date_dep( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.free_time_dep( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Free all time :term:`dependencies`. i.e :term:`time`, :term:`day`, :term:`today`, :term:`cron`
    ::
    
       void free_time_dep(
          string absolute_node_path : Path name to node
       )
    
    After freeing the time related dependencies (i.e time,today,cron)
    the next time slot will be missed.
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           ci.free_time_dep('/s1/task')
       except RuntimeError, e:
           print(str(e))
    

free_time_dep( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.free_trigger_dep( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Free :term:`trigger` :term:`dependencies` for a :term:`node`
    ::
    
       void free_trigger_dep(
          string absolute_node_path : Path name to node
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()         # use default host(ECF_HOST) & port(ECF_PORT)
           ci.free_trigger_dep('/s1/f1/task')
       except RuntimeError, e:
           print(str(e))
    

free_trigger_dep( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.get_defs( (Client)arg1) -> Defs :
   :module: ecflow

Returns the :term:`suite definition` stored on the Client.

Use :py:class:`ecflow.Client.sync_local()` to retrieve the definition from the server first.
The definition is *retained* in memory until the next call to sync_local().

Usage:

.. code-block:: python

   try:
       ci = Client()         # use default host(ECF_HOST) & port(ECF_PORT)
       ci.sync_local()       # get the definition from the server and store on 'ci'
       print(ci.get_defs())  # print out definition stored in the client
       print(ci.get_defs())  # print again, this shows that defs is retained on ci
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.get_file( (Client)arg1, (str)task [, (str)type='script' [, (str)max_lines='10000' [, (bool)as_bytes=False]]]) -> object :
   :module: ecflow

The File command is used to request the various file types associated with a :term:`node`.

By default, the output is composed of the last 10000 lines of the file. The number of lines can be customised via the :code:`max_lines` parameter.

The content can be retrieved as a sequence of 'bytes'. This allows to download a file that contains invalid Unicode sequence, without causing an :code:`UnicodeDecodeError` to be raised.
::

   string get_file(
      string absolute_node_path    : Path name to node
      [(string)file_type='script'] : file_type = [ script<default> | job | jobout | manual | kill | stat ]
      [(string)max_lines='10000'] : The number of lines in the file to return
      [(bool)as_bytes=False] : A flag indicating if the output should be 'bytes'; by default the output is of type 'str'
   )

Usage:

.. code-block:: python

   try:
       ci = Client()        # use default host(ECF_HOST) & port(ECF_PORT)
       for file in [ 'script', 'job', 'jobout', 'manual', 'kill', 'stat' ]:
           print(ci.get_file('/suite/f1/t1',file))  # print the contents of the file
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.get_host( (Client)arg1) -> str :
   :module: ecflow

Return the host, assume set_host_port() has been set, otherwise return localhost


.. py:method:: Client.get_log( (Client)arg1, (int)arg2) -> str :
   :module: ecflow

Request the :term:`ecflow_server` to return the log file contents as a string

Use with caution as the returned string could be several megabytes.
Only enabled in the debug build of ECF.

Usage:

.. code-block:: python

   try:
       ci = Client()          # use default host(ECF_HOST) & port(ECF_PORT)
       print(ci.get_log(100)  # get the 100 last lines from server log file
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.get_port( (Client)arg1) -> str :
   :module: ecflow

Return the port, assume set_host_port() has been set. otherwise returns 3141


.. py:method:: Client.get_server_defs( (Client)arg1) -> int :
   :module: ecflow

Get all suite Node tree's from the :term:`ecflow_server`.

The definition is *retained* in memory until the next call to get_server_defs().
This is important since get_server_defs() could return several megabytes of data.
Hence we only want to call it once, and then access it locally with get_defs().
If you need to access the server definition in a loop use :py:class:`ecflow.Client.sync_local` instead
since this is capable of returning incremental changes, and thus considerably
reducing the network load.

Usage:

.. code-block:: python

   try:
       ci = Client()         # use default host(ECF_HOST) & port(ECF_PORT)
       ci.get_server_defs()  # get the definition from the server and store on 'ci'
       print(ci.get_defs())  # print out definition stored in the client
       print(ci.get_defs())  # print again, this shows that defs is retained on ci
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.group( (Client)arg1, (str)arg2) -> int :
   :module: ecflow

Allows a series of commands to be executed in the :term:`ecflow_server`
::

   void group(
       string cmds : a list of ';' separated commands 
   )

Usage:

.. code-block:: python

   try:
       ci = Client()               # use default host(ECF_HOST) & port(ECF_PORT)
       ci.group('get; show')
       ci.group('get; show state') # show node states and trigger abstract syntax trees
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.halt_server( (Client)arg1) -> int :
   :module: ecflow

Halt the :term:`ecflow_server`

Stop server communication with jobs, and new job scheduling, and stops check pointing.
See :term:`server states`

Usage:

.. code-block:: python

   try:
       ci = Client()            # use default host(ECF_HOST) & port(ECF_PORT)
       ci.halt_server()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.in_sync( (Client)arg1) -> bool :
   :module: ecflow

Returns true if the definition on the client is in sync with the :term:`ecflow_server`

.. Warning:: Calling in_sync() is **only** valid after a call to sync_local().

Usage:

.. code-block:: python

   try:
      ci = Client()                       # use default host(ECF_HOST) & port(ECF_PORT)
      ci.sync_local()                     # very first call gets the full Defs
      client_defs = ci.get_defs()         # End user access to the returned Defs
      ... after a period of time
      ci.sync_local()                     # Subsequent calls to sync_local() users the local Defs to sync incrementally
      if ci.in_sync():                    # returns true  changed and changes applied to client
         print('Client is now in sync with server')
      client_defs = ci.get_defs()         # End user access to the returned Defs
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.is_auto_sync_enabled( (Client)arg1) -> bool :
   :module: ecflow

Returns true if automatic syncing enabled


.. py:method:: Client.job_generation( (Client)arg1, (str)arg2) -> int :
   :module: ecflow

Job submission for chosen Node *based* on :term:`dependencies`

The :term:`ecflow_server` traverses the :term:`node` tree every 60 seconds, and if the dependencies are free
does `job creation` and submission. Sometimes the user may free time/date dependencies
to avoid waiting for the server poll, this commands allows early job generation
::

   void job_generation(
      string absolute_node_path: Path name for job generation to start from
   )
   If empty string specified generates for full definition.

Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       ci.job_generation('/s1')  # generate jobs for suite '/s1 
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.kill( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Kills the job associated with the :term:`node`
    ::
    
       void kill(
          list paths: List of paths. Paths must begin with a leading '/' character
       )
       void kill(
          string absolute_node_path: Path name to node to kill.
       )
    
    If a :term:`family` or :term:`suite` is selected, will kill hierarchically.
    Kill uses the ECF_KILL_CMD variable. After :term:`variable substitution` it is invoked as a command.
    The ECF_KILL_CMD variable should be written in such a way that the output is written to %ECF_JOB%.kill, i.e:
    
    .. code-block:: shell
    
       kill -15 %ECF_RID% > %ECF_JOB%.kill 2>&1
       /home/ma/emos/bin/ecfkill %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1
    
    
    Exceptions can be raised because:
    
    - The absolute_node_path does not exist in the server
    - ECF_KILL_CMD variable is not defined
    - :term:`variable substitution` fails
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.kill('/s1/f1')
           time.sleep(2)
           print(ci.file('/s1/t1','kill')) # request kill output
       except RuntimeError, e:
           print(str(e))
    

kill( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.load( (Client)arg1, (str)path_to_defs [, (bool)force=False [, (bool)check_only=False [, (bool)print=False [, (bool)stats=False]]]]) -> int :
   :module: ecflow

Load a :term:`suite definition` or checkpoint file given by the file_path argument into the :term:`ecflow_server`
    ::
    
       void load(
          string file_path     : path name to the definition file
          [(bool)force=False]  : If true overwrite suite of same name
          [(bool)print=False]  : print parsed defs to standard out
       )
    
    By default throws a RuntimeError exception for errors.
    If force is not used and :term:`suite` of the same name already exists in the server,
    then a error is thrown
    
    Usage:
    
    .. code-block:: python
    
       defs_file = 'Hello.def' 
       defs = Defs()
       suite = def.add_suite('s1')
       family = suite.add_family('f1')
       for i in [ '_1', '_2', '_3' ]:
          family.add_task( 't' + i )
       defs.save_as_defs(defs_file)  # write out in memory defs into the file 'Hello.def'
       ...
       try:
           ci = Client()       # use default host(ECF_HOST) & port(ECF_PORT)
           ci.load(defs_file)  # open and parse defs or checkpoint file, and load into server.
       except RuntimeError, e:
           print(str(e))
    

load( (Client)arg1, (Defs)defs [, (bool)force=False]) -> int :
    Load a in memory :term:`suite definition` into the :term:`ecflow_server`
    ::
    
       void load(
          Defs defs           : A in memory definition
          [(bool)force=False] : for true overwrite suite of same name
       )
    
    If force is not used and :term:`suite` already exists in the server, then a error is thrown.
    
    Usage:
    
    .. code-block:: python
    
       defs = Defs()
       suite = defs.add_suite('s1')
       family = suite.add_family('f1')
       for i in [ '_1', '_2', '_3' ]: 
           family.add_task( Task( 't' + i) )
       ...
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.load(defs)    # Load in memory defs, into the server
       except RuntimeError, e:
           print(str(e))
    


.. py:method:: Client.log_msg( (Client)arg1, (str)arg2) -> int
   :module: ecflow


.. py:method:: Client.new_log( (Client)arg1 [, (str)path='']) -> int :
   :module: ecflow

Request the :term:`ecflow_server` to use the path provided, as the new log file

The old log file is released.

Usage:

.. code-block:: python

   try:
       ci = Client()               # use default host(ECF_HOST) & port(ECF_PORT)
       ci.new_log('/path/log.log') # use '/path/log,log' as the new log file
                                   # To keep track of log file Can change ECF_LOG
       ci.alter('','change','variable','ECF_LOG','/new/path.log')
       ci.new_log()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.news_local( (Client)arg1) -> bool :
   :module: ecflow

Query the :term:`ecflow_server` to detect any changes.

This returns a simple bool, if there has been changes, the user should call :py:class:`ecflow.Client.sync_local`.
This will bring the client in sync with changes in the server. If sync_local() is not called
then calling news_local() will always return true.
news_local() uses the definition stored on the client::

   bool news_local()


Usage:

.. code-block:: python

   try:
       ci = Client()                  # use default host(ECF_HOST) & port(ECF_PORT)
       if ci.news_local():            # has the server changed
          print('Server Changed')     # server changed bring client in sync with server
          ci.sync_local()             # get the full definition from the server if first time
                                      # otherwise apply incremental changes to Client definition,
                                      # bringing it in sync with the server definition
          print(ci.get_defs())        # print the synchronised definition. Should be same as server
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.order( (Client)arg1, (str)arg2, (str)arg3) -> None :
   :module: ecflow

Re-orders the :term:`node`\ s in the :term:`suite definition` held by the :term:`ecflow_server`

It should be noted that in the absence of :term:`dependencies`,
the order in which :term:`task`\ s are :term:`submitted`, depends on the order in the definition.
This changes the order and hence affects the submission order
::

   void order(
      string absolute_node_path: Path name to node.
      string order_type        : Must be one of [ top | bottom | alpha | order | up | down ]
   )
   o top     raises the node within its parent, so that it is first
   o bottom  lowers the node within its parent, so that it is last
   o alpha   Arranges for all the peers of selected note to be sorted alphabetically
   o order   Arranges for all the peers of selected note to be sorted in reverse alphabet
   o up      Moves the selected node up one place amongst its peers
   o down    Moves the selected node down one place amongst its peers

Exceptions can be raised because:

- The absolute_node_path does not exist in the server
- The order_type is not the right type

Usage:

.. code-block:: python

   try:
       ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
       ci.order('/s1/f1','top')
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.ping( (Client)arg1) -> int :
   :module: ecflow

Checks if the :term:`ecflow_server` is running
::

   void ping()

The default behaviour is to check on host 'localhost' and port 3141
It should be noted that any Client function will fail if the server is
is not running. Hence ping() is not strictly required. However its main
distinction from other Client function is that it is quite fast.

Usage:

.. code-block:: python

   try:
       ci = Client('localhost','3150')
       ci.ping()
       print('------- Server already running------')
       do_something_with_server(ci)
   except RuntimeError, e:
       print('------- Server *NOT* running------' + str(e))


.. py:method:: Client.plug( (Client)arg1, (str)arg2, (str)arg3) -> int :
   :module: ecflow

Plug command is used to move :term:`node`\ s

The destination node can be on another :term:`ecflow_server`.
In which case the destination path should be of the form '//<host>:<port>/suite/family/task
::

   void plug(
      string source_absolute_node_path       : Path name to source node
      string destination_absolute_node_path  : Path name to destination node. Note if only
                                               '//host:port' is specified the whole suite can be moved
   )

By default throws a RuntimeError exception for errors.

Exceptions can be raised because:

- Source :term:`node` is in a :term:`active` or :term:`submitted` state.
- Another user already has an lock.
- source/destination paths do not exist on the corresponding servers
- If the destination node path is empty, i.e. only host:port is specified,
  then the source :term:`node` must correspond to a :term:`suite`.
- If the source node is added as a child, then its name must be unique

Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       ci.plug('/suite','host3:3141')
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.query( (Client)arg1, (str)arg2, (str)arg3 [, (str)arg4]) -> str :
   :module: ecflow

Query the status of event, meter, state, variable, limit, limit_max or trigger expression without blocking

 - state     return [unknown | complete | queued |             aborted | submitted | active] to standard out
 - dstate    return [unknown | complete | queued | suspended | aborted | submitted | active] to standard out
 - event     return 'set' | 'clear' to standard out
 - meter     return value of the meter to standard out
 - limit     return value of the limit to standard out
 - limit_max return max value of the limit to standard out
 - variable  return value to standard out
 - trigger   returns 'true' if the expression is true, otherwise 'false'

:

.. code-block:: shell

  string query(
     string query_type        # [ event | meter | variable | trigger | limit | limit_max ]
     string path_to_attribute # path to the attribute
     string attribute         # name of the attribute or trigger expression
  )

By default throws a exception for errors.

Exceptions can be raised if the path to the attribute does not exist and because:

- No event of the given name exists on the specified node
- No meter of the given name exists on the specified node
- No limit of the given name exists on the specified node
- No variable of the given name (repeat or generated variable) exists on the
  specified node or any of its parent
- trigger expression does not parse, or if references to node/attributes are not defined

Usage:

.. code-block:: python

   try:
       ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
       res = ci.query('event','/path/to/node','event_name') # returns 'SET' | 'CLEAR'
       res = ci.query('meter','/path/to/node','meter_name') # returns meter value as a string
       res = ci.query('limit','/path/to/node','limit_name') # returns limit value as a string
       res = ci.query('limit_max','/path/to/node','limit_name') # returns max limit value as a string
       res = ci.query('variable','/path/to/node,'var')      # returns variable value as a string
       res = ci.query('trigger','/path/to/node','/joe90 == complete') # return 'true' | 'false' as a string
       res = ci.query('state','/path/to/node') # return node state as a string
       res = ci.query('dstate','/path/to/node') # return node state as a string,can include suspended
   except RuntimeError, e:
       print str(e)


.. py:method:: Client.reload_custom_passwd_file( (Client)arg1) -> int :
   :module: ecflow

reload the custom passwd file. <host>.<port>.ecf.custom_passwd. For users using ECF_USER or --user or set_user_name()


.. py:method:: Client.reload_passwd_file( (Client)arg1) -> int :
   :module: ecflow

reload the passwd file. <host>.<port>.ecf.passwd


.. py:method:: Client.reload_wl_file( (Client)arg1) -> int :
   :module: ecflow

Request that the :term:`ecflow_server` reload the white list file.

The white list file if present, can be used to control who has read/write
access to the :term:`ecflow_server`::

   void reload_wl_file()

Usage:

.. code-block:: python

   try:
       ci = Client()            # use default host(ECF_HOST) & port(ECF_PORT)
       ci.reload_wl_file()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.replace( (Client)arg1, (str)arg2, (str)arg3, (bool)arg4, (bool)arg5) -> int :
   :module: ecflow

Replaces a :term:`node` in a :term:`suite definition` with the given path. The definition is in the :term:`ecflow_server`
    ::
    
       void replace(
          string absolute_node_path: Path name to node in the client defs.
                                     This is also the node we want to replace in the server.
          string client_defs_file  : File path to defs files, that provides the definition of the new node
          [(bool)parent=False]     : create parent families or suite as needed,
                                     when absolute_node_path does not exist in the server
          [(bool)force=False]      : check for zombies, if force = true, bypass checks
       )
    
       void replace(
          string absolute_node_path: Path name to node in the client defs.
                                     This is also the node we want to replace in the server.
          Defs client_defs         : In memory client definition that provides the definition of the new node
          [(bool)parent=False]     : create parent families or suite as needed,
                                     when absolute_node_path does not exist in the server
          [(bool)force=False]      : check for zombies, force = true, bypass checks
       )
    
    Exceptions can be raised because:
    
    - The absolute_node_path does not exist in the provided definition
    - The provided client definition must be free of errors
    - If the third argument is not provided, then the absolute_node_path must exist in the server defs
    - replace will fail, if child task nodes are in :term:`active` / :term:`submitted` state
    
    After replace is done, we check trigger expressions. These are reported to standard output.
    It is up to the user to correct invalid trigger expressions, otherwise the tasks will *not* run.
    Please note, you can use check() to check trigger expression and limits in the server.
    
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.replace('/s1/f1','/tmp/defs.def')
       except RuntimeError, e:
           print(str(e))
    
       try:
           ci.replace('/s1',client_defs) # replace suite 's1' in the server, with 's1' in the client_defs
       except RuntimeError, e:
           print(str(e))
    

replace( (Client)arg1, (str)arg2, (Defs)arg3, (bool)arg4, (bool)arg5) -> int

replace( (Client)arg1, (str)arg2, (Defs)arg3) -> None

replace( (Client)arg1, (str)arg2, (str)arg3) -> None


.. py:method:: Client.requeue( (Client)arg1, (str)abs_node_path [, (str)option='']) -> None :
   :module: ecflow

Re queues the specified :term:`node` (s)
    ::
    
       void requeue(
          list paths     : A list of paths. Node paths must begin with a leading '/' character
          [(str)option=''] : option = ('' | 'abort' | 'force')
              ''   : empty string, the default, re-queue the node
              abort: means re-queue only aborted tasks below node
              force: means re-queueing even if there are nodes that are active or submitted
       )
       void requeue(
          string absolute_node_path : Path name to node
          [(string)option='']       : option = ('' | 'abort' | 'force')
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()                   # use default host(ECF_HOST) & port(ECF_PORT)
           ci.requeue('/s1','abort')       # re-queue aborted tasks below suite /s1
    
           path_list = ['/s1/f1/t1','/s2/f1/t2']
           ci.requeue(path_list)
       except RuntimeError, e:
           print(str(e))
    

requeue( (Client)arg1, (list)paths [, (str)option='']) -> None


.. py:method:: Client.reset( (Client)arg1) -> None :
   :module: ecflow

reset client definition, and handle number


.. py:method:: Client.restart_server( (Client)arg1) -> int :
   :module: ecflow

Restart the :term:`ecflow_server`

Start job scheduling, communication with jobs, and respond to all requests.
See :term:`server states`

Usage:

.. code-block:: python

   try:
       ci = Client()            # use default host(ECF_HOST) & port(ECF_PORT)
       ci.restart_server()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.restore( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Restore archived nodes.
    Usage::
    
       string restore(
          list paths # List of paths.
       )
       string restore(
          string absolute_node_path
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           print ci.restore('/suite1')
       except RuntimeError, e:
           print str(e)
    

restore( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.restore_from_checkpt( (Client)arg1) -> int :
   :module: ecflow

Request the :term:`ecflow_server` loads the :term:`check point` file from disk

The server will first try to open file at ECF_HOME/ECF_CHECK if that fails it will
then try path ECF_HOME/ECF_CHECKOLD.
An error is returned if the server has not been :term:`halted` or contains a :term:`suite definition`

Usage:

.. code-block:: python

   try:
       ci = Client()             # use default host(ECF_HOST) & port(ECF_PORT)
       ci.halt_server()          # server must be halted, otherwise restore_from_checkpt will throw
       ci.restore_from_checkpt() # restore the definition from the check point file
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.resume( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Resume `job creation` / generation for the given :term:`node`
    ::
    
       void resume(
          list paths: List of paths. Paths must begin with a leading '/' character
       )
       void resume(
          string absolute_node_path: Path name to node to resume.
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
           ci.resume('/s1/f1/task1')
           paths = ['/s1/f1/t1','/s2/f1/t2']
           ci.resume(paths)
       except RuntimeError, e:
           print(str(e))
    

resume( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.run( (Client)arg1, (str)arg2, (bool)arg3) -> None :
   :module: ecflow

Immediately run the jobs associated with the input :term:`node`.
    
    Ignore :term:`trigger`\ s, :term:`limit`\ s, :term:`suspended`, :term:`time` or :term:`date` dependencies,
    just run the :term:`task`.
    When a job completes, it may be automatically re-queued if it has
    multiple time :term:`dependencies`. In the specific case where a :term:`task` has a SINGLE
    time dependency and we want to avoid re running the :term:`task` then
    a flag is set so that it is not automatically re-queued when set to :term:`complete`.
    The flag is applied up the :term:`node` hierarchy until we reach a node with a :term:`repeat`
    or :term:`cron` attribute. This behaviour allow :term:`repeat` values to be incremented interactively.
    A :term:`repeat` attribute is incremented when all the child nodes are :term:`complete`
    in this case the child nodes are automatically re-queued
    ::
    
       void run(
          string absolute_node_path : Path name to node. If the path is suite/family will recursively
                                      run all child tasks
          [(bool)force=False]       : If true, run even if there are nodes that are active or submitted.
       )
       void run(
          list  paths               : List of paths. If the path is suite/family will recursively run all child tasks
          [(bool)force=False]       : If true, run even if there are nodes that are active or submitted.
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()                          # use default host(ECF_HOST) & port(ECF_PORT)
           ci.run('/s1')                          # run all tasks under suite /s1
    
           ci.run(['/s1/f1/t1','/s2/f1/t2'])      # run all tasks specified
       except RuntimeError, e:
           print(str(e))
    
    Effect:
    
       Lets see the effect of run command on the following defs:
    
    .. code-block:: shell
    
       suite s1
          task t1; time 10:00             # will complete straight away
          task t2; time 10:00 13:00 01:00 # will re-queue 3 times and complete on fourth run
    
    In the last case (task t2) after each run the next time slot is incremented.
    This can be seen by calling the Why command.

run( (Client)arg1, (list)arg2, (bool)arg3) -> None


.. py:method:: Client.server_version( (Client)arg1) -> str :
   :module: ecflow

Returns the server version, can throw for old servers, that did not implement this request.


.. py:method:: Client.set_auto_sync( (Client)arg1, (bool)arg2) -> None :
   :module: ecflow

If true automatically sync with local definition after each call.


.. py:method:: Client.set_child_complete_del_vars( (Client)arg1, (list)arg2) -> None :
   :module: ecflow

Set the list of variables to be deleted when a task becomes complete
Needs a list of strings, representing the variable names.


.. py:method:: Client.set_child_init_add_vars( (Client)arg1, (dict)arg2) -> None :
   :module: ecflow

Set the list of variables to be added when a task becomes active
    Needs a dictionary of name/value pairs, or a list of ecflow Variables

set_child_init_add_vars( (Client)arg1, (list)arg2) -> None :
    Set the list of variables to be added when a task becomes active
    Needs a dictionary of name/value pairs, or a list of ecflow Variables


.. py:method:: Client.set_child_password( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Set the password, needed for authentication, provided by the server using %ECF_PASS%

By default the environment variable ECF_PASS is read for the jobs password
This can be overridden for the python child api


.. py:method:: Client.set_child_path( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Set the path to the task, obtained from server using %ECF_NAME%

By default the environment variable ECF_NAME is read for the task path
This can be overridden for the python child api


.. py:method:: Client.set_child_pid( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Set the process id of this job
    
    By default the environment variable ECF_RID is read for the jobs process or remote id
    This can be overridden for the python child api

set_child_pid( (Client)arg1, (int)arg2) -> None :
    Set the process id of this job
    
    By default the environment variable ECF_RID is read for the jobs process or remote id
    This can be overridden for the python child api


.. py:method:: Client.set_child_timeout( (Client)arg1, (int)arg2) -> None :
   :module: ecflow

Set timeout if child cannot connect to server, default is 24 hours. The input is required to be in seconds

By default the environment variable  ECF_TIMEOUT is read to control how long child command should attempt to connect to the server
This can be overridden for the python child api


.. py:method:: Client.set_child_try_no( (Client)arg1, (int)arg2) -> None :
   :module: ecflow

Set the try no, i.e the number of times this job has run, obtained from the server, using %ECF_TRYNO%

By default the environment variable ECF_TRYNO is read to record number of times job has been run
This can be overridden for the python child api


.. py:method:: Client.set_connection_attempts( (Client)arg1, (int)arg2) -> None :
   :module: ecflow

Set the number of times to connect to :term:`ecflow_server`, in case of connection failures

The period between connection attempts is handled by Client.set_retry_connection_period().
If the network is unreliable the connection attempts can be be increased, likewise
when the network is stable this number could be reduced to one.
This can increase responsiveness and reduce latency.
Default value is set as 2.
Setting a value less than one is ignored, will default to 1 in this case::

   set_connection_attempts(
      int attempts # must be an integer >= 1
   )

Exceptions:

- None

Usage:

.. code-block:: python

   ci = Client()
   ci.set_connection_attempts(3)     # make 3 attempts for server connection
   ci.set_retry_connection_period(1) # wait 1 second between each attempt


.. py:method:: Client.set_host_port( (Client)arg1, (str)arg2, (str)arg3) -> None :
   :module: ecflow

Explicitly set the host and port to be used by the client, overriding the default host name (localhost) and port (3141) and the environment variables: ECF_HOST and ECF_PORT.
    
    .. code-block:: shell
    
       set_host_port(
          string host, # The server name. Cannot be empty.
          string port  # The port on the server, must be unique to the server
       )
    
       set_host_port(
          string host, # The server name. Cannot be empty.
          int port     # The port on the server, must be unique to the server
       )
    
       set_host_port(
          string host_port, # Expects <host>:<port> or <host>@<port>
       )
    
    Exceptions:
    
    - Raise a RuntimeError if the host or port is empty
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()
           ci.set_host_port('localhost','3150')
           ci.set_host_port('avi',3150)
           ci.set_host_port('avi:3150')
       except RuntimeError, e:
           print(str(e))
    
    

set_host_port( (Client)arg1, (str)arg2) -> None

set_host_port( (Client)arg1, (str)arg2, (int)arg3) -> None


.. py:method:: Client.set_retry_connection_period( (Client)arg1, (int)arg2) -> None :
   :module: ecflow

Set the sleep period between connection attempts

Whenever there is a connection failure we wait a number of seconds before trying again.
i.e. to get round glitches in the network.
For the ping command this is hard wired as 1 second.
This wait between connection attempts can be configured here.
i.e This could be reduced to increase responsiveness.
Default: In debug this period is 1 second and in release mode 10 seconds.

.. code-block:: shell

   set_retry_connection_period(
      int period # must be an integer >= 0
   )

Exceptions:

- None

Usage:

.. code-block:: python

   ci = Client()
   ci.set_connection_attempts(3)     # make 3 attempts for server connection
   ci.set_retry_connection_period(1) # wait 1 second between each attempt


.. py:method:: Client.set_user_name( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

set user name. A password must be provided in the file <host>.<port>.ecf.custom_passwd


.. py:method:: Client.set_zombie_child_timeout( (Client)arg1, (int)arg2) -> None :
   :module: ecflow

Set timeout for zombie child commands,that cannot connect to server, default is 24 hours. The input is required to be in seconds


.. py:method:: Client.shutdown_server( (Client)arg1) -> int :
   :module: ecflow

Shut down the :term:`ecflow_server`

Stop server from scheduling new jobs.
See :term:`server states`

Usage:

.. code-block:: python

   try:
       ci = Client()            # use default host(ECF_HOST) & port(ECF_PORT)
       ci.shutdown_server()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.sort_attributes( (Client)arg1, (str)abs_node_path, (str)attribute_name [, (bool)recursive=True]) -> None
   :module: ecflow

sort_attributes( (Client)arg1, (list)paths, (str)attribute_name [, (bool)recursive=True]) -> None


.. py:method:: Client.stats( (Client)arg1 [, (bool)to_stdout]) -> str :
   :module: ecflow

Returns the :term:`ecflow_server` statistics as a string

.. warning::

    When called without arguments, this function will print the statistics to :code:`stdout`, before returning the information as a string.
    To avoid printing the output, set the boolean flag :code:`to_stdout` to :code:`False`.

Usage:

.. code-block:: python

   try:
       ci = Client()  # use default host(ECF_HOST) & port(ECF_PORT)
       stats = ci.stats()      # prints to stdout
       stats = ci.stats(True)  # prints to stdout
       stats = ci.stats(False) # does not print to stdout
       print(stats)
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.stats_reset( (Client)arg1) -> None :
   :module: ecflow

Resets the statistical data in the server
::

   void stats_reset()

Usage:

.. code-block:: python

   try:
       ci = Client()  # use default host(ECF_HOST) & port(ECF_PORT)
       ci.stats_reset()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.status( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Shows the status of a job associated with a :term:`task`
    ::
    
       void status(
          list paths: List of paths. Paths must begin with a leading '/' character
       )
       void status(
          string absolute_node_path
       )
    
    If a :term:`family` or :term:`suite` is selected, will invoke status command hierarchically.
    Status uses the ECF_STATUS_CMD variable. After :term:`variable substitution` it is invoked as a command.
    The command should be written in such a way that the output is written to %ECF_JOB%.stat, i.e:
    
    .. code-block:: shell
    
       /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1
    
    Exceptions can be raised because:
    
    - The absolute_node_path does not exist in the server
    - ECF_STATUS_CMD variable is not defined
    - :term:`variable substitution` fails
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.status('/s1/t1')
           time.sleep(2)
           print(ci.file('/s1/t1','stats')) # request status output
       except RuntimeError, e:
           print(str(e))
    

status( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.suites( (Client)arg1) -> list :
   :module: ecflow

Returns a list strings representing the :term:`suite` names
::

   list(string) suites()

Usage:

.. code-block:: python

   try:
       ci = Client()  # use default host(ECF_HOST) & port(ECF_PORT)
       suites = ci.suites()
       print(suites)
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.suspend( (Client)arg1, (str)arg2) -> None :
   :module: ecflow

Suspend `job creation` / generation for the given :term:`node`
    ::
    
       void suspend(
          list paths: List of paths. Paths must begin with a leading '/' character
       )
       void suspend(
          string absolute_node_path: Path name to node to suspend.
       )
    
    Usage:
    
    .. code-block:: python
    
       try:
           ci = Client()    # use default host(ECF_HOST) & port(ECF_PORT)
           ci.suspend('/s1/f1/task1')
           paths = ['/s1/f1/t1','/s2/f1/t2']
           ci.suspend(paths)
       except RuntimeError, e:
           print(str(e))
    

suspend( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.sync_local( (Client)arg1 [, (bool)sync_suite_clock=False]) -> int :
   :module: ecflow

Requests that :term:`ecflow_server` returns the full definition or incremental change made and applies them to the client Defs

When there is a very large definition, calling :py:class:`ecflow.Client.get_server_defs` each time can be *very* expensive
both in terms of memory, speed, and network bandwidth. The alternative is to call
this function, which will get the incremental changes, and apply them local client :term:`suite definition`
effectively synchronising the client and server Defs.
If the period of time between two sync() calls is too long, then the full server definition
is returned and assigned to the client Defs.
We can determine if the changes were applied by calling in_sync() after the call to sync_local()::

   void sync_local();                     # The very first call, will get the full Defs.


Exceptions:

- raise a RuntimeError if the delta change cannot be applied.
- this could happen if the client Defs bears no resemblance to server Defs

Usage:

.. code-block:: python

   try:
       ci = Client()                       # use default host(ECF_HOST) & port(ECF_PORT)
       ci.sync_local()                     # Very first call gets the full Defs
       client_defs = ci.get_defs()         # End user access to the returned Defs
       ... after a period of time
       ci.sync_local()                     # Subsequent calls to sync_local() users the local Defs to sync incrementally
       if ci.in_sync():                    # returns true server changed and changes applied to client
          print('Client is now in sync with server')
       client_defs = ci.get_defs()         # End user access to the returned Defs
   except RuntimeError, e:
       print(str(e))

Calling sync_local() is considerably faster than calling get_server_defs() for large Definitions


.. py:method:: Client.terminate_server( (Client)arg1) -> int :
   :module: ecflow

Terminate the :term:`ecflow_server`


Usage:

.. code-block:: python

   try:
       ci = Client()            # use default host(ECF_HOST) & port(ECF_PORT)
       ci.terminate_server()
   except RuntimeError, e:
       print(str(e))


.. py:method:: Client.version( (Client)arg1) -> str :
   :module: ecflow

Returns the current client version


.. py:method:: Client.wait_for_server_reply( (Client)arg1 [, (int)time_out=60]) -> bool :
   :module: ecflow

Wait for a response from the :term:`ecflow_server`::

   void wait_for_server_reply(
      int time_out     : (default = 60) 
   )

This is used to check if server has started. Typically for tests.
Returns true if server(ping) replies before time out, otherwise false

Usage:

.. code-block:: python

   ci = Client()   # use default host(ECF_HOST) & port(ECF_PORT)
   if ci.wait_for_server_reply(30):
      print('Server is alive')
   else:
      print('Timed out after 30 second wait for server response.?')


.. py:method:: Client.zombie_adopt( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_adopt( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.zombie_block( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_block( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.zombie_fail( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_fail( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.zombie_fob( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_fob( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.zombie_get( (Client)arg1, (int)arg2) -> ZombieVec
   :module: ecflow


.. py:method:: Client.zombie_kill( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_kill( (Client)arg1, (list)arg2) -> None


.. py:method:: Client.zombie_remove( (Client)arg1, (str)arg2) -> int
   :module: ecflow

zombie_remove( (Client)arg1, (list)arg2) -> None

