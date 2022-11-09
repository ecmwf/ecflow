ecflow.Defs
///////////


.. py:class:: Defs
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

The Defs class holds the :term:`suite definition` structure.

It contains all the :py:class:`ecflow.Suite` and hence acts like the root for suite node tree hierarchy.
The definition can be kept as python code, alternatively it can be saved as a flat
ASCII definition file.
If a definition is read in from disk, it will by default, check the :term:`trigger` expressions.
If however the definition is created in python, then checking should be done explicitly:

   Defs(string)
      string - The Defs class take one argument which represents the file name
   Defs(Suite | Edit )
      :py:class:`ecflow.Suite`- One or more suites

      :py:class:`ecflow.Edit` - specifies user defined server variables

Example:

.. code-block:: python

 # Build definition using Constructor approach, This allows indentation, to show the structure
 # This is a made up example to demonstrate suite construction:
 defs = Defs(
     Edit(SLEEP=10,FRED='bill'),  # user defined server variables
     Suite('s1'
         Clock(1, 1, 2010, False),
         Autocancel(1, 10, True),
         Task('t1'
             Edit({'a':'12', 'b':'bb'}, c='v',d='b'),
             Edit(g='d'),
             Edit(h=1),
             Event(1),
             Event(11,'event'),
             Meter('meter',0,10,10),
             Label('label','c'),
             Trigger('1==1'),
             Complete('1==1'),
             Limit('limit',10),Limit('limit2',10),
             InLimit('limitName','/limit',2),
             Defstatus(DState.complete),
             Today(0,30),Today('00:59'),Today('00:00 11:30 00:01'),
             Time(0,30),Time('00:59'),Time('00:00 11:30 00:01'),
             Day('sunday'),Day(Days.monday),
             Date(1,1,0),Date(28,2,1960),
             Autocancel(3)
             ),
         [ Family('f{}'.format(i)) for i in range(1,6)]))

  defs.save_as_defs('filename.def')  # save defs into file

  defs = Defs()                      # create an empty defs
  suite = defs.add_suite('s1')
  family = suite.add_family('f1')
  for i in [ '_1', '_2', '_3' ]: family.add_task( 't' + i )
  defs.save_as_defs('filename.def')  # save defs into file

Create a Defs from an existing file on disk:

.. code-block:: python

  defs = Defs('filename.def')   #  Will open and parse the file and create the Definition
  print(defs)


.. py:method:: Defs.add
   :module: ecflow

object add(tuple args, dict kwds) :
    add(..) provides a way to append Nodes and attributes
    
    This is best illustrated with an example:
    
    .. code-block:: python
    
     defs = Defs().add(
         Suite('s1').add(
             Clock(1, 1, 2010, False),
             Autocancel(1, 10, True),
             Task('t1').add(
                 Edit({'a':'12', 'b':'bb'}, c='v',d='b'),
                 Edit(g='d'),
                 Edit(h=1),
                 Event(1),
                 Event(11,'event'),
                 Meter('meter',0,10,10),
                 Label('label','c'),
                 Trigger('1==1'),
                 Complete('1==1'),
                 Limit('limit',10),Limit('limit2',10),
                 InLimit('limitName','/limit',2),
                 Defstatus(DState.complete),
                 Today(0,30),Today('00:59'),Today('00:00 11:30 00:01'),
                 Time(0,30),Time('00:59'),Time('00:00 11:30 00:01'),
                 Day('sunday'),Day(Days.monday),
                 Date(1,1,0),Date(28,2,1960),
                 Autocancel(3)
                 ),
             [ Family('f{}'.format(i)) for i in range(1,6)]))
    
    We can also use '+=' with a list here are a few examples:
    
    .. code-block:: python
    
     defs = Defs();
     defs += [ Suite('s2'),Edit({ 'x1':'y', 'aa1':'bb'}, a='v',b='b') ]
    
    .. code-block:: python
    
     defs += [ Suite('s{}'.format(i)) for i in range(1,6) ]
    
    .. code-block:: python
    
     defs = Defs()
     defs += [ Suite('suite').add(
                  Task('x'),
                  Family('f').add( [ Task('t{}'.format(i)) for i in range(1,6)] ),
                  Task('y'),
                  [ Family('f{}'.format(i)) for i in range(1,6) ],
                  Edit(a='b'),
                  [ Task('t{}'.format(i)) for i in range(1,6) ],
                  )]
    
    It is also possible to use '+'
    
    .. code-block:: python
    
     defs = Defs() + Suite('s1')
     defs.s1 += Autocancel(1, 10, True)
     defs.s1 += Task('t1') + Edit({ 'e':1, 'f':'bb'}) +\ 
                Event(1) + Event(11,'event') + Meter('meter',0,10,10) + Label('label','c') + Trigger('1==1') +\ 
                Complete('1==1') + Limit('limit',10) + Limit('limit2',10) + InLimit('limitName','/limit',2) +\ 
                Defstatus(DState.complete) + Today(0,30) + Today('00:59') + Today('00:00 11:30 00:01') +\ 
                Time(0,30) + Time('00:59') + Time('00:00 11:30 00:01') + Day('sunday') + Day(Days.monday) +\ 
                Date(1,1,0) + Date(28,2,1960) + Autocancel(3)
    
    .. warning:: We can only use '+' when the left most object is a node, i.e Task('t1') in this case


.. py:method:: Defs.add_extern( (Defs)arg1, (str)arg2) -> None :
   :module: ecflow

:term:`extern` refer to nodes that have not yet been defined typically due to cross suite :term:`dependencies`

:term:`trigger` and :term:`complete expression`\ s may refer to paths, and variables in other suites, that have not been
loaded yet. The references to node paths and variable must exist, or exist as externs
Externs can be added manually or automatically.

Manual Method:

.. code-block:: python

  void add_extern(string nodePath )

Usage:

.. code-block:: python

  defs = Defs('file.def')
  ....
  defs.add_extern('/temp/bill:event_name')
  defs.add_extern('/temp/bill:meter_name')
  defs.add_extern('/temp/bill:repeat_name')
  defs.add_extern('/temp/bill:edit_name')
  defs.add_extern('/temp/bill')

Automatic Method:
  This will scan all trigger and complete expressions, looking for paths and variables
  that have not been defined. The added benefit of this approach is that duplicates will not
  be added. It is the user's responsibility to check that extern's are eventually defined
  otherwise trigger expression will not evaluate correctly

.. code-block:: python

  void auto_add_externs(bool remove_existing_externs_first )

Usage:

.. code-block:: python

  defs = Defs('file.def')
  ...
  defs.auto_add_externs(True)   # remove existing extern first.


.. py:method:: Defs.add_suite( (Defs)arg1, (Suite)arg2) -> Suite :
   :module: ecflow

Add a :term:`suite` :term:`node`. See :py:class:`ecflow.Suite`
    
    If a new suite is added which matches the name of an existing suite, then an exception is thrown.
    
    Exception:
    
    - Throws RuntimeError is the suite name is not valid
    - Throws RuntimeError if duplicate suite is added
    
    Usage:
    
    .. code-block:: python
    
      defs = Defs()                # create a empty defs
      suite = Suite('suite')       # create a stand alone Suite 
      defs.add_suite(suite)        # add suite to defs
      s2 = defs.add_suite('s2')    # create a suite and add to defs
    
      # Alternatively we can create Suite in place
      defs = Defs(
               Suite('s1',
                  Family('f1',
                     Task('t1'))),
               Suite('s2',
                  Family('f1',
                     Task('t1'))))
    

add_suite( (Defs)arg1, (str)arg2) -> Suite :
    Create a empty Defs
    
    


.. py:method:: Defs.add_variable( (Defs)arg1, (str)arg2, (str)arg3) -> Defs :
   :module: ecflow

Adds a name value :term:`variable`. Also see :py:class:`ecflow.Edit`
    
    This defines a variable for use in :term:`variable substitution` in a :term:`ecf script` file.
    There can be any number of variables. The variables are names inside a pair of
    '%' characters in an :term:`ecf script`. The name are case sensitive.
    Special character in the value, must be placed inside single quotes if misinterpretation
    is to be avoided.
    The value of the variable replaces the variable name in the :term:`ecf script` at `job creation` time.
    The variable names for any given node must be unique. If duplicates are added then the
    the last value added is kept.
    
    Exception:
    
    - Writes warning to standard output, if a duplicate variable name is added
    
    Usage:
    
    .. code-block:: python
    
      task.add_variable( Variable('ECF_HOME','/tmp/'))
      task.add_variable( 'TMPDIR','/tmp/')
      task.add_variable( 'COUNT',2)
      a_dict = { 'name':'value', 'name2':'value2', 'name3':'value3' }
      task.add_variable(a_dict)
    

add_variable( (Defs)arg1, (str)arg2, (int)arg3) -> Defs

add_variable( (Defs)arg1, (Variable)arg2) -> Defs

add_variable( (Defs)arg1, (dict)arg2) -> Defs


.. py:method:: Defs.auto_add_externs( (Defs)arg1, (bool)arg2) -> None :
   :module: ecflow

:term:`extern` refer to nodes that have not yet been defined typically due to cross suite :term:`dependencies`

:term:`trigger` and :term:`complete expression`\ s may refer to paths, and variables in other suites, that have not been
loaded yet. The references to node paths and variable must exist, or exist as externs
Externs can be added manually or automatically.

Manual Method:

.. code-block:: python

  void add_extern(string nodePath )

Usage:

.. code-block:: python

  defs = Defs('file.def')
  ....
  defs.add_extern('/temp/bill:event_name')
  defs.add_extern('/temp/bill:meter_name')
  defs.add_extern('/temp/bill:repeat_name')
  defs.add_extern('/temp/bill:edit_name')
  defs.add_extern('/temp/bill')

Automatic Method:
  This will scan all trigger and complete expressions, looking for paths and variables
  that have not been defined. The added benefit of this approach is that duplicates will not
  be added. It is the user's responsibility to check that extern's are eventually defined
  otherwise trigger expression will not evaluate correctly

.. code-block:: python

  void auto_add_externs(bool remove_existing_externs_first )

Usage:

.. code-block:: python

  defs = Defs('file.def')
  ...
  defs.auto_add_externs(True)   # remove existing extern first.


.. py:method:: Defs.check( (Defs)arg1) -> str :
   :module: ecflow

Check :term:`trigger` and :term:`complete expression`\ s and :term:`limit`\ s

* Client Side: The client side can specify externs. Hence all node path references
  in :term:`trigger` expressions, and :term:`inlimit` references to :term:`limit`\ s, that are
  unresolved and which do *not* appear in :term:`extern`\ s are reported as errors
* Server Side: The server does not store externs. Hence all unresolved references
  are reported as errors

Returns a non empty string for any errors or warning

Usage:

.. code-block:: python

   # Client side
   defs = Defs('my.def')        # Load my.def from disk
   ....
   print(defs.check()) # do the check

   # Server Side
   try:
       ci = Client()             # use default host(ECF_HOST) & port(ECF_PORT)
       print(ci.check('/suite'))
   except RuntimeError, e:
       print(str(e))


.. py:method:: Defs.check_job_creation( (Defs)arg1 [, (bool)throw_on_error=False [, (bool)verbose=False]]) -> str :
   :module: ecflow

Check `job creation` .
    
    Will check the following:
    
    - :term:`ecf script` files and includes files can be located
    - recursive includes
    - manual and comments :term:`pre-processing`
    - :term:`variable substitution`
    
    Some :term:`task`\ s are dummy tasks have no associated :term:`ecf script` file.
    To disable error message for these tasks please add a variable called ECF_DUMMY_TASK to them.
    Checking is done in conjunction with the class :py:class:`ecflow.JobCreationCtrl`.
    If no node path is set on class JobCreationCtrl then all tasks are checked.
    In the case where we want to check all tasks, use the convenience function that take no arguments.
    
    Usage:
    
    .. code-block:: python
    
       defs = Defs('my.def')                     # specify the defs we want to check, load into memory
       ...
       print(defs.check_job_creation())          # Check job generation for all tasks
       ...
    
       # throw on error and Output the tasks as they are being checked
       defs.check_job_creation(throw_on_error=TrueTrue,verbose=True)
    
       job_ctrl = JobCreationCtrl()
       job_ctrl.set_verbose(True)                # Output the tasks as they are being checked
       defs.check_job_creation(job_ctrl)         # Check job generation for all tasks, same as above
       print(job_ctrl.get_error_msg())
       ...
       job_ctrl = JobCreationCtrl()
       job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node
       defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB
       print(job_ctrl.get_error_msg())
       ...
       job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks
       job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory
       defs.check_job_creation(job_ctrl)
       print(job_ctrl.get_error_msg())
       ...
       job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks
       job_ctrl.generate_temp_dir()              # automatically generate directory for job file
       defs.check_job_creation(job_ctrl)
       print(job_ctrl.get_error_msg())
    

check_job_creation( (Defs)arg1, (JobCreationCtrl)arg2) -> None


.. py:method:: Defs.delete_variable( (Defs)arg1, (str)arg2) -> None :
   :module: ecflow

An empty string will delete all user variables


.. py:property:: Defs.externs
   :module: ecflow

Returns a list of :term:`extern`\ s


.. py:method:: Defs.find_abs_node( (Defs)arg1, (str)arg2) -> Node :
   :module: ecflow

Given a path, find the the :term:`node`


.. py:method:: Defs.find_node( (Defs)arg1, (str)arg2, (str)arg3) -> Node :
   :module: ecflow

Given a type(suite,family,task) and a path to a node, return the node.


.. py:method:: Defs.find_node_path( (Defs)arg1, (str)arg2, (str)arg3) -> str :
   :module: ecflow

Given a type(suite,family,task) and a name, return path of the first match, otherwise return an empty string


.. py:method:: Defs.find_suite( (Defs)arg1, (str)arg2) -> Suite :
   :module: ecflow

Given a name, find the corresponding :term:`suite`


.. py:method:: Defs.generate_scripts( (Defs)arg1) -> None :
   :module: ecflow

Automatically generate template :term:`ecf script`\ s for this definition
Will automatically add :term:`child command`\ s for :term:`event`\ s, :term:`meter`\ s and :term:`label`\ s.
This allows the definition to be refined with out worrying about the scripts.
However it should be noted that, this will create a lot of *duplicated* script contents
i.e in the absence of :term:`event`\ s, :term:`meter`\ s and :term:`label`\ s, most of generated :term:`ecf script` files will
be the same. Hence should only be used an aid to debugging the definition.
It uses the contents of the definition to parameterise what gets
generated, and the location of the files. Will throw Exceptions for errors.

Requires:

- ECF_HOME: specified and accessible for all Tasks, otherwise RuntimeError is raised
- ECF_INCLUDE: specifies location for head.h and tail.h includes, will use angle brackets,
               i.e %include <head.h>, if the head.h and tail.h already exist they are used otherwise
               they are generated

Optional:

- ECF_FILES: If specified, then scripts are generated under this directory otherwise ECF_HOME is used.
             The missing directories are automatically created.
- ECF_CLIENT_EXE_PATH: if specified child command will use this, otherwise will use ecflow_client
                       and assume this accessible on the path.
- ECF_DUMMY_TASK: Will not generated scripts for this task.
- SLEEP: Uses this variable to delay time between calls to child commands, if not specified uses delay of one second


Usage:

.. code-block:: python

   defs = ecflow.Defs()
   suite = defs.add_suite('s1')
   suite.add_variable('ECF_HOME','/user/var/home')
   suite.add_variable('ECF_INCLUDE','/user/var/home/includes')
   for i in range(1,7) :
      fam = suite.add_family('f' + str(i))
      for t in ( 'a', 'b', 'c', 'd', 'e' ) :
        fam.add_task(t);
   defs.generate_scripts()   # generate '.ecf' and head.h/tail.h if required


.. py:method:: Defs.get_all_nodes( (Defs)arg1) -> NodeVec :
   :module: ecflow

Returns all the :term:`node`\ s in the definition


.. py:method:: Defs.get_all_tasks( (Defs)arg1) -> TaskVec :
   :module: ecflow

Returns all the :term:`task` nodes


.. py:method:: Defs.get_server_state( (Defs)arg1) -> SState :
   :module: ecflow

Returns the :term:`ecflow_server` state: See :term:`server states`


Usage:

.. code-block:: python

   try:
       ci = Client()           # use default host(ECF_HOST) & port(ECF_PORT)
       ci.shutdown_server()
       ci.sync_local()
       assert ci.get_defs().get_server_state() == SState.SHUTDOWN, 'Expected server to be shutdown'
   except RuntimeError, e:
       print(str(e))


.. py:method:: Defs.get_state( (Defs)arg1) -> State
   :module: ecflow


.. py:method:: Defs.has_time_dependencies( (Defs)arg1) -> bool :
   :module: ecflow

returns True if the :term:`suite definition` has any time :term:`dependencies`


.. py:method:: Defs.restore_from_checkpt( (Defs)arg1, (str)arg2) -> None :
   :module: ecflow

Restore the :term:`suite definition` from a :term:`check point` file stored on disk


.. py:method:: Defs.save_as_checkpt( (Defs)arg1, (str)arg2) -> None :
   :module: ecflow

Save the in memory :term:`suite definition` as a :term:`check point` file. This includes all node state.


.. py:method:: Defs.save_as_defs( (Defs)arg1, (str)arg2 [, (Style)arg3]) -> None :
   :module: ecflow

Save the in memory :term:`suite definition` into a file. The file name must be passed as an argument
    
    


.. py:property:: Defs.server_variables
   :module: ecflow

Returns a list of server :term:`variable`\ s


.. py:method:: Defs.simulate( (Defs)arg1) -> str :
   :module: ecflow

Simulates a suite definition, allowing you predict/verify the behaviour of your suite in few seconds

The simulator will analyse the definition, and simulate the ecflow server.
Allowing time dependencies that span several months, to be simulated in a few seconds.
Ecflow allows the use of verify attributes. This example show how we can verify the number of times
a task should run, given a start(optional) and end time(optional):

.. code-block:: shell

  suite cron3              # use real clock otherwise clock starts when the simulations starts.
     clock real  1.1.2006  # define a start date for deterministic behaviour
     endclock   13.1.2006  # When to finish. end clock is *only* used for the simulator
     family cronFamily
        task t
           cron -d 10,11,12   10:00 11:00 01:00  # run on 10,11,12 of the month at 10am and 11am
           verify complete:6                     # task should complete 6 times between 1.1.2006 -> 13.1.2006
     endfamily
  endsuite

Please note, for deterministic behaviour, the start and end clock should be specified.
However if no 'endclock' is specified the simulation will assume the following defaults.

- No time dependencies: 24 hours
- time || today       : 24 hours
- day                 : 1 week
- date                : 1 month
- cron                : 1 year
- repeat              : 1 year

If there no time dependencies with an minute resolution, then the simulator will by default
use 1 hour resolution. This needs to be taken into account when specifying the verify attribute
If the simulation does not complete it creates  defs.flat and  defs.depth files.
This provides clues as to the state of the definition at the end of the simulation

Usage:

.. code-block:: python

   defs = Defs('my.def')        # specify the defs we want to simulate
   ....
   theResults = defs.simulate()
   print(theResults)


.. py:method:: Defs.sort_attributes( (Defs)arg1, (AttrType)arg2) -> None
   :module: ecflow

sort_attributes( (Defs)arg1, (AttrType)arg2, (bool)arg3) -> None

sort_attributes( (Defs)arg1, (AttrType)arg2, (bool)arg3, (list)arg4) -> None

sort_attributes( (Defs)arg1, (str)attribute_type [, (bool)recursive=True [, (list)no_sort=[]]]) -> None

sort_attributes( (Defs)arg1, (AttrType)arg2, (bool)attribute_type [, (object)recursive=True]) -> None


.. py:property:: Defs.suites
   :module: ecflow

Returns a list of :term:`suite`\ s


.. py:property:: Defs.user_variables
   :module: ecflow

Returns a list of user defined :term:`variable`\ s

