ecflow.Node
///////////


.. py:class:: Node
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

A Node class is the abstract base class for Suite, Family and Task

Every Node instance has a name, and a path relative to a suite


.. py:method:: Node.add(self: ecflow.Node, *args, **kwargs) -> object
   :module: ecflow

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


.. py:method:: Node.add_autoarchive(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_autoarchive(self: ecflow.Node, days: typing.SupportsInt | typing.SupportsIndex, idle: bool = False) -> ecflow.Node

Add a `autoarchive` attribute. See :py:class:`ecflow.Autoarchive`

Provides a way to automatically archive a suite/family which has completed.(i.e remove children)
This is required when dealing with super large suite/families, they can be archived off, and then restored later.
The node can be recovered using 'autorestore',begin,re-queue and manually via ecflow_client --restore.
The archived node is written to disk, as ECF_HOME/<host>.<port>.ECF_NAME.check,
where '/' is replaced with ':' in ECF_NAME.
The removal may be delayed by an amount of time in hours and minutes or expressed as days
Node removal is not immediate. The nodes are checked once a minute
A Node may only have one autoarchive attribute

Exception:

- Throws a RuntimeError if more than one auto archive is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_autoarchive( Autoarchive(20,10,False) )  # hour,min, relative
  t2 = Task('t2')
  t2.add_autoarchive( 3 )                        # 3 days 
  t3 = Task('t3')
  t3.add_autoarchive( 20,10,True )               # hour,minutes,relative
  t4 = Task('t4')
  t4.add_autoarchive( TimeSlot(20,10),True )     # hour,minutes,relative

  # we can also create a Autoarchive in the Task constructor like any other attribute
  t2 = Task('t2',
            Autoarchive(20,10,False))


2. add_autoarchive(self: ecflow.Node, hour: typing.SupportsInt | typing.SupportsIndex, min: typing.SupportsInt | typing.SupportsIndex, relative: bool, idle: bool = False) -> ecflow.Node

3. add_autoarchive(self: ecflow.Node, TimeSlot: ecflow.TimeSlot, relative: bool, idle: bool = False) -> ecflow.Node

4. add_autoarchive(self: ecflow.Node, arg0: ecflow.Autoarchive) -> ecflow.Node


.. py:method:: Node.add_autocancel(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_autocancel(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

Add a `autocancel` attribute. See :py:class:`ecflow.Autocancel`

This will delete the node on completion. The deletion may be delayed by
an amount of time in hours and minutes or expressed as days
Node deletion is not immediate. The nodes are checked once a minute
and expired auto cancel nodes are deleted
A node may only have one auto cancel attribute

Exception:

- Throws a RuntimeError if more than one auto cancel is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_autocancel( Autocancel(20,10,False) )  # hour,min, relative
  t2 = Task('t2')
  t2.add_autocancel( 3 )                        # 3 days 
  t3 = Task('t3')
  t3.add_autocancel( 20,10,True )               # hour,minutes,relative
  t4 = Task('t4')
  t4.add_autocancel( TimeSlot(20,10),True )     # hour,minutes,relative

  # we can also create a Autocancel in the Task constructor like any other attribute
  t2 = Task('t2',
            Autocancel(20,10,False))


2. add_autocancel(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: bool) -> ecflow.Node

3. add_autocancel(self: ecflow.Node, arg0: ecflow.TimeSlot, arg1: bool) -> ecflow.Node

4. add_autocancel(self: ecflow.Node, arg0: ecflow.Autocancel) -> ecflow.Node


.. py:method:: Node.add_autorestore(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_autorestore(self: ecflow.Node, arg0: ecflow.Autorestore) -> ecflow.Node

Add a `autorestore` attribute. See :py:class:`ecflow.Autorestore`

Auto-restore is used to automatically restore a previously auto-archived node.
The restore will fail if:

 - The node has not been archived
 - The node has children.
 - The file ECF_HOME/<host>.<port>.ECF_NAME.check does not exist

Exception:

- Throws a RuntimeError if more than one autorestore is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_autorestore( ['/s1/f1'] )   
  t2 = Task('t2')
  t2.add_autorestore( Autorestore(['/s2/f1','/s1/f2']) )  
  # we can also create a Autorestore in the Task constructor like any other attribute
  t2 = Task('t2', Autorestore(['/s2/f1','/s1/f2'] ))


2. add_autorestore(self: ecflow.Node, arg0: list) -> ecflow.Node


.. py:method:: Node.add_aviso(self: ecflow.Node, arg0: ecflow.AvisoAttr) -> ecflow.Node
   :module: ecflow

Adds an :term:`aviso` to a :term:`node`. See :py:class:`ecflow.Aviso`


.. py:method:: Node.add_complete(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_complete(self: ecflow.Node, arg0: str) -> ecflow.Node

Add a :term:`trigger` or :term:`complete expression`.Also see :py:class:`ecflow.Trigger`

This defines a dependency for a :term:`node`.
There can only be one :term:`trigger` or :term:`complete expression` dependency per node.
A :term:`node` with a trigger can only be activated when the trigger has expired.
A trigger holds a node as long as the expression returns false.

Exception:

- Will throw RuntimeError if multiple trigger or complete expression are added
- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression
  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set

Usage:

Note we cannot make multiple add_trigger(..) calls on the same :term:`task`!
to add a simple trigger:

.. code-block:: python

  task1.add_trigger( 't2 == active' )
  task2.add_trigger( 't1 == complete or t4 == complete' )
  task3.add_trigger( 't5 == active' )

Long expression can be broken up using add_part_trigger:

.. code-block:: python

  task2.add_part_trigger( 't1 == complete or t4 == complete')
  task2.add_part_trigger( 't5 == active',True)  # True means  AND
  task2.add_part_trigger( 't7 == active',False) # False means OR

The trigger for task2 is equivalent to:
't1 == complete or t4 == complete and t5 == active or t7 == active'

2. add_complete(self: ecflow.Node, arg0: ecflow.Expression) -> ecflow.Node


.. py:method:: Node.add_cron(self: ecflow.Node, arg0: ecflow.Cron) -> ecflow.Node
   :module: ecflow

Add a :term:`cron` time dependency. See :py:class:`ecflow.Cron`


Usage:

.. code-block:: python

  start = TimeSlot(0,0)
  finish = TimeSlot(23,0)
  incr = TimeSlot(0,30)
  time_series = TimeSeries( start, finish, incr, True)
  cron = Cron()
  cron.set_week_days( [0,1,2,3,4,5,6] )
  cron.set_days_of_month( [1,2,3,4,5,6] )
  cron.set_months( [1,2,3,4,5,6] )
  cron.set_time_series( time_series )
  t1 = Task('t1')
  t1.add_cron( cron )

  # we can also create a Cron in the Task constructor like any other attribute
  t2 = Task('t2',
            Cron('+00:00 23:00 00:30',days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6],months=[1,2,3,4,5,6]))


.. py:method:: Node.add_date(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_date(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

Add a :term:`date` time dependency. See :py:class:`ecflow.Date`

A value of zero for day,month,year means every day, every month, every year

Exception:

- Throws RuntimeError if an invalid date is added

Usage:

.. code-block:: python

  t1 = Task('t1',
            Date('1.*.*'),
            Date(1,1,2010)))    # Create Date in place

  t1.add_date( Date(1,1,2010) ) # day,month,year
  t1.add_date( 2,1,2010)        # day,month,year
  t1.add_date( 1,0,0)           # day,month,year, the first of each month for every year


2. add_date(self: ecflow.Node, arg0: ecflow.Date) -> ecflow.Node


.. py:method:: Node.add_day(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_day(self: ecflow.Node, arg0: ecflow.Days) -> ecflow.Node

Add a :term:`day` time dependency. See :py:class:`ecflow.Day`


Usage:

.. code-block:: python

  t1 = Task('t1',
            Day('sunday'))  # Create Day on Task creation

  t1.add_day( Day(Days.sunday) )
  t1.add_day( Days.monday)
  t1.add_day( 'tuesday' )


2. add_day(self: ecflow.Node, arg0: str) -> ecflow.Node

3. add_day(self: ecflow.Node, arg0: ecflow.Day) -> ecflow.Node


.. py:method:: Node.add_defstatus(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_defstatus(self: ecflow.Node, arg0: ecflow.DState) -> ecflow.Node

Set the default status( :term:`defstatus` ) of node at begin or re queue. See :py:class:`ecflow.Defstatus`

A :term:`defstatus` is useful in preventing suites from running automatically
once begun, or in setting Task's complete so they can be run selectively

Usage:

.. code-block:: python

  t1 = Task('t1') + Defstatus('complete')
  t2 = Task('t2').add_defstatus( DState.suspended )

  # we can also create a Defstatus in the Task constructor like any other attribute
  t2 = Task('t3',
            Defstatus('complete'))


2. add_defstatus(self: ecflow.Node, arg0: ecflow.Defstatus) -> ecflow.Node

Set the default status( :term:`defstatus` ) of node at begin or re queue. See :py:class:`ecflow.Defstatus`

A :term:`defstatus` is useful in preventing suites from running automatically
once begun, or in setting Task's complete so they can be run selectively

Usage:

.. code-block:: python

  t1 = Task('t1') + Defstatus('complete')
  t2 = Task('t2').add_defstatus( DState.suspended )

  # we can also create a Defstatus in the Task constructor like any other attribute
  t2 = Task('t3',
            Defstatus('complete'))


.. py:method:: Node.add_event(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_event(self: ecflow.Node, arg0: ecflow.Event) -> ecflow.Node

Add a :term:`event`. See :py:class:`ecflow.Event`
Events can be referenced in :term:`trigger` and :term:`complete expression`\ s


Exception:

- Throws RuntimeError if a duplicate is added

Usage:

.. code-block:: python

  t1 = Task('t1',
            Event(12),
            Event(11,'eventx'))             # Create events on Task creation

  t1.add_event( Event(10) )                 # Create with function on Task
  t1.add_event( Event(11,'Eventname') )
  t1.add_event( 12 )
  t1.add_event( 13, 'name')

To reference event 'flag' in a trigger:

.. code-block:: python

  t1.add_event('flag')
  t2 = Task('t2',
            Trigger('t1:flag == set'))

2. add_event(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

3. add_event(self: ecflow.Node, arg0: str) -> ecflow.Node

4. add_event(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: str) -> ecflow.Node


.. py:method:: Node.add_generic(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_generic(self: ecflow.Node, arg0: ecflow.Generic) -> ecflow.Node

2. add_generic(self: ecflow.Node, arg0: str, arg1: list) -> ecflow.Node


.. py:method:: Node.add_inlimit(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_inlimit(self: ecflow.Node, limit_name: str, path_to_node_containing_limit: str = '', tokens: typing.SupportsInt | typing.SupportsIndex = 1, limit_this_node_only: bool = False) -> ecflow.Node

Adds a :term:`inlimit` to a :term:`node`. See :py:class:`ecflow.InLimit`

InLimit reference a :term:`limit`/:py:class:`ecflow.Limit`. Duplicate InLimits are not allowed

Exception:

- Throws RuntimeError if a duplicate is added

Usage:

.. code-block:: python

  task2.add_inlimit( InLimit('limitName','/s1/f1',2) )
  task2.add_inlimit( 'limitName','/s1/f1',2 )


2. add_inlimit(self: ecflow.Node, arg0: ecflow.InLimit) -> ecflow.Node


.. py:method:: Node.add_label(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_label(self: ecflow.Node, arg0: str, arg1: str) -> ecflow.Node

Adds a :term:`label` to a :term:`node`. See :py:class:`ecflow.Label`

Labels can be updated from the jobs files, via :term:`child command`

Exception:

- Throws RuntimeError if a duplicate label name is added

Usage:

.. code-block:: python

  task.add_label( Label('TEA','/me/'))
  task.add_label( 'Joe','/me/')

The corresponding child command in the .ecf script file might be:

.. code-block:: shell

  ecflow_client --label=TEA time
  ecflow_client --label=Joe ninety


2. add_label(self: ecflow.Node, arg0: ecflow.Label) -> ecflow.Node


.. py:method:: Node.add_late(self: ecflow.Node, arg0: ecflow.Late) -> ecflow.Node
   :module: ecflow

Add a :term:`late` attribute. See :py:class:`ecflow.Late`


Exception:

- Throws a RuntimeError if more than one late is added

Usage:

.. code-block:: python

  late = Late()
  late.submitted( 20,10 )     # hour,minute
  late.active(    20,10 )     # hour,minute
  late.complete(  20,10,True) # hour,minute,relative
  t1 = Task('t1')
  t1.add_late( late )

  # we can also create a Late in the Task constructor like any other attribute
  t2 = Task('t2',
            Late(submitted='20:10',active='20:10',complete='+20:10'))


.. py:method:: Node.add_limit(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_limit(self: ecflow.Node, arg0: str, arg1: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

Adds a :term:`limit` to a :term:`node` for simple load management. See :py:class:`ecflow.Limit`

Multiple limits can be added, however the limit name must be unique.
For a node to be in a limit, a :term:`inlimit` must be used.

Exception:

- Throws RuntimeError if a duplicate limit name is added

Usage:

.. code-block:: python

  family.add_limit( Limit('load',12) )
  family.add_limit( 'load',12 )


2. add_limit(self: ecflow.Node, arg0: ecflow.Limit) -> ecflow.Node


.. py:method:: Node.add_meter(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_meter(self: ecflow.Node, arg0: ecflow.Meter) -> ecflow.Node

Add a :term:`meter`. See :py:class:`ecflow.Meter`
Meters can be referenced in :term:`trigger` and :term:`complete expression`\ s


Exception:

- Throws RuntimeError if a duplicate is added

Usage:

.. code-block:: python

  t1 = Task('t1',
            Meter('met',0,50))                   # create Meter on Task creation
  t1.add_meter( Meter('metername',0,100,50) )  # create Meter using function
  t1.add_meter( 'meter',0,200)

To reference in a trigger:

.. code-block:: python

  t2 = Task('t2')
  t2.add_trigger('t1:meter >= 10')


2. add_meter(self: ecflow.Node, arg0: str, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: typing.SupportsInt | typing.SupportsIndex, arg3: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

3. add_meter(self: ecflow.Node, arg0: str, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node


.. py:method:: Node.add_mirror(self: ecflow.Node, arg0: ecflow.MirrorAttr) -> ecflow.Node
   :module: ecflow

Adds a :term:`mirror` to a :term:`node`. See :py:class:`ecflow.Mirror`


.. py:method:: Node.add_part_complete(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_part_complete(self: ecflow.Node, arg0: ecflow.PartExpression) -> ecflow.Node

Add a :term:`trigger` or :term:`complete expression`.Also see :py:class:`ecflow.Trigger`

This defines a dependency for a :term:`node`.
There can only be one :term:`trigger` or :term:`complete expression` dependency per node.
A :term:`node` with a trigger can only be activated when the trigger has expired.
A trigger holds a node as long as the expression returns false.

Exception:

- Will throw RuntimeError if multiple trigger or complete expression are added
- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression
  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set

Usage:

Note we cannot make multiple add_trigger(..) calls on the same :term:`task`!
to add a simple trigger:

.. code-block:: python

  task1.add_trigger( 't2 == active' )
  task2.add_trigger( 't1 == complete or t4 == complete' )
  task3.add_trigger( 't5 == active' )

Long expression can be broken up using add_part_trigger:

.. code-block:: python

  task2.add_part_trigger( 't1 == complete or t4 == complete')
  task2.add_part_trigger( 't5 == active',True)  # True means  AND
  task2.add_part_trigger( 't7 == active',False) # False means OR

The trigger for task2 is equivalent to:
't1 == complete or t4 == complete and t5 == active or t7 == active'

2. add_part_complete(self: ecflow.Node, arg0: str) -> ecflow.Node

3. add_part_complete(self: ecflow.Node, arg0: str, arg1: bool) -> ecflow.Node


.. py:method:: Node.add_part_trigger(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_part_trigger(self: ecflow.Node, arg0: ecflow.PartExpression) -> ecflow.Node

Add a :term:`trigger` or :term:`complete expression`.Also see :py:class:`ecflow.Trigger`

This defines a dependency for a :term:`node`.
There can only be one :term:`trigger` or :term:`complete expression` dependency per node.
A :term:`node` with a trigger can only be activated when the trigger has expired.
A trigger holds a node as long as the expression returns false.

Exception:

- Will throw RuntimeError if multiple trigger or complete expression are added
- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression
  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set

Usage:

Note we cannot make multiple add_trigger(..) calls on the same :term:`task`!
to add a simple trigger:

.. code-block:: python

  task1.add_trigger( 't2 == active' )
  task2.add_trigger( 't1 == complete or t4 == complete' )
  task3.add_trigger( 't5 == active' )

Long expression can be broken up using add_part_trigger:

.. code-block:: python

  task2.add_part_trigger( 't1 == complete or t4 == complete')
  task2.add_part_trigger( 't5 == active',True)  # True means  AND
  task2.add_part_trigger( 't7 == active',False) # False means OR

The trigger for task2 is equivalent to:
't1 == complete or t4 == complete and t5 == active or t7 == active'

2. add_part_trigger(self: ecflow.Node, arg0: str) -> ecflow.Node

3. add_part_trigger(self: ecflow.Node, arg0: str, arg1: bool) -> ecflow.Node


.. py:method:: Node.add_queue(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_queue(self: ecflow.Node, arg0: ecflow.Queue) -> ecflow.Node

2. add_queue(self: ecflow.Node, arg0: str, arg1: list) -> ecflow.Node


.. py:method:: Node.add_repeat(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatDate) -> ecflow.Node

Add a RepeatDate attribute. See :py:class:`ecflow.RepeatDate`

A node can only have one repeat
Reference to a RepeatDate in a trigger will use date arithmetic in a sub expression. i.e.
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer

   trigger /suite/family:YMD + 1 > 20190101

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatDate('YMD',20100111,20100115) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatDate('YMD',20100111,20100115))


2. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatDateTime) -> ecflow.Node

Add a RepeatDateTime attribute. See :py:class:`ecflow.RepeatDateTime`

A node can only have one repeat.
When a RepeatDateTime is used in a trigger expression, the arithmetic value of the Repeat decays to second.
For example, the expression `/suite/family:DATETIME + 1` is evaluated as the number of seconds represented by `/suite/family:DT` (since the reference epoch, i.e. 19700101T000000) plus 1.The result is an integer.

   trigger /suite/family:DT + 1 > 123456

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat(RepeatDateTime('DT', '20100111T120000', '20100115T000000', '12:00:00'))

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatDateTime('DT', '20100101T000000', '20100115T000000', '1:00:00'))


3. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatDateList) -> ecflow.Node

Add a RepeatDateList attribute. See :py:class:`ecflow.RepeatDateList`

A node can only have one repeat
Reference to a RepeatDateList in a trigger will use date arithmetic. i.e.
Here (/suite/family:YMD + 1) uses date arithmetic only, the result is still an integer:

.. code-block:: python

  trigger /suite/family:YMD + 1 > 20190101


Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatDateList('YMD',[20100111,20100115]) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatDateList('YMD',[20100111,20100115]))


4. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatDateTimeList) -> ecflow.Node

Add a RepeatDateTimeList attribute. See :py:class:`ecflow.RepeatDateTimeList`

A node can only have one repeat

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatDateTimeList('DT', ['20240101T000000', '20240102T120000']) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatDateTimeList('DT', ['20240101T000000', '20240102T120000']))


5. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatInteger) -> ecflow.Node

Add a RepeatInteger attribute. See :py:class:`ecflow.RepeatInteger`

A node can only have one :term:`repeat`

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatInteger('testInteger',0,100,2) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatInteger('testInteger',0,100,2))


6. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatString) -> ecflow.Node

Add a RepeatString attribute. See :py:class:`ecflow.RepeatString`

A node can only have one :term:`repeat`

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatString('test_string',['a', 'b', 'c' ] ) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatString('test_string',['a', 'b', 'c' ] ) )


7. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatEnumerated) -> ecflow.Node

Add a RepeatEnumerated attribute. See :py:class:`ecflow.RepeatEnumerated`

A node can only have one :term:`repeat`

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t1 = Task('t1')
  t1.add_repeat( RepeatEnumerated('test_string', ['red', 'green', 'blue' ] ) )

  # we can also create a repeat in Task constructor like any other attribute
  t2 = Task('t2',
            RepeatEnumerated('test_string', ['red', 'green', 'blue' ] ) )


8. add_repeat(self: ecflow.Node, arg0: ecflow.RepeatDay) -> ecflow.Node

Add a RepeatDay attribute. See :py:class:`ecflow.RepeatDay`

A node can only have one :term:`repeat`

Exception:

- Throws a RuntimeError if more than one repeat is added

Usage:

.. code-block:: python

  t2 = Task('t2',
            RepeatDay(1))


.. py:method:: Node.add_time(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_time(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

Add a :term:`time` dependency. See :py:class:`ecflow.Time`


Usage:

.. code-block:: python

  t1 = Task('t1', Time('+00:30 20:00 01:00')) # Create Time in Task constructor
  t1.add_time( '00:30' )
  t1.add_time( '+00:30' )
  t1.add_time( '+00:30 20:00 01:00' )
  t1.add_time( Time( 0,10 ))      # hour,min,relative =false
  t1.add_time( Time( 0,12,True )) # hour,min,relative
  t1.add_time( Time(TimeSlot(20,20),False))
  t1.add_time( 0,1 ))              # hour,min,relative=false
  t1.add_time( 0,3,False ))        # hour,min,relative=false
  start = TimeSlot(0,0)
  finish = TimeSlot(23,0)
  incr = TimeSlot(0,30)
  ts = TimeSeries( start, finish, incr, True)
  task2.add_time( Time(ts) )


2. add_time(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: bool) -> ecflow.Node

3. add_time(self: ecflow.Node, arg0: str) -> ecflow.Node

4. add_time(self: ecflow.Node, arg0: ecflow.Time) -> ecflow.Node


.. py:method:: Node.add_today(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_today(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

Add a :term:`today` time dependency. See :py:class:`ecflow.Today`


Usage:

.. code-block:: python

  t1 = Task('t1',
            Today('+00:30 20:00 01:00')) # Create Today in Task constructor

  t1.add_today( '00:30' )
  t1.add_today( '+00:30' )
  t1.add_today( '+00:30 20:00 01:00' )
  t1.add_today( Today( 0,10 ))      # hour,min,relative =false
  t1.add_today( Today( 0,12,True )) # hour,min,relative
  t1.add_today( Today(TimeSlot(20,20),False))
  t1.add_today( 0,1 ))              # hour,min,relative=false
  t1.add_today( 0,3,False ))        # hour,min,relative=false
  start = TimeSlot(0,0)
  finish = TimeSlot(23,0)
  incr = TimeSlot(0,30)
  ts = TimeSeries( start, finish, incr, True)
  task2.add_today( Today(ts) )


2. add_today(self: ecflow.Node, arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsInt | typing.SupportsIndex, arg2: bool) -> ecflow.Node

3. add_today(self: ecflow.Node, arg0: str) -> ecflow.Node

4. add_today(self: ecflow.Node, arg0: ecflow.Today) -> ecflow.Node


.. py:method:: Node.add_trigger(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_trigger(self: ecflow.Node, arg0: str) -> ecflow.Node

Add a :term:`trigger` or :term:`complete expression`.Also see :py:class:`ecflow.Trigger`

This defines a dependency for a :term:`node`.
There can only be one :term:`trigger` or :term:`complete expression` dependency per node.
A :term:`node` with a trigger can only be activated when the trigger has expired.
A trigger holds a node as long as the expression returns false.

Exception:

- Will throw RuntimeError if multiple trigger or complete expression are added
- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression
  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set

Usage:

Note we cannot make multiple add_trigger(..) calls on the same :term:`task`!
to add a simple trigger:

.. code-block:: python

  task1.add_trigger( 't2 == active' )
  task2.add_trigger( 't1 == complete or t4 == complete' )
  task3.add_trigger( 't5 == active' )

Long expression can be broken up using add_part_trigger:

.. code-block:: python

  task2.add_part_trigger( 't1 == complete or t4 == complete')
  task2.add_part_trigger( 't5 == active',True)  # True means  AND
  task2.add_part_trigger( 't7 == active',False) # False means OR

The trigger for task2 is equivalent to:
't1 == complete or t4 == complete and t5 == active or t7 == active'

2. add_trigger(self: ecflow.Node, arg0: ecflow.Expression) -> ecflow.Node


.. py:method:: Node.add_variable(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. add_variable(self: ecflow.Node, arg0: str, arg1: str) -> ecflow.Node

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


2. add_variable(self: ecflow.Node, arg0: str, arg1: typing.SupportsInt | typing.SupportsIndex) -> ecflow.Node

3. add_variable(self: ecflow.Node, arg0: ecflow.Variable) -> ecflow.Node

4. add_variable(self: ecflow.Node, arg0: ecflow.Edit) -> ecflow.Node

5. add_variable(self: ecflow.Node, arg0: object) -> ecflow.Node


.. py:method:: Node.add_verify(self: ecflow.Node, arg0: ecflow.Verify) -> None
   :module: ecflow

Add a Verify attribute.

Used in python simulation used to assert that a particular state was reached.  t2 = Task('t2',
             Verify(State.complete, 6)) # verify task completes 6 times during simulation


.. py:method:: Node.add_zombie(self: ecflow.Node, arg0: ecflow.ZombieAttr) -> ecflow.Node
   :module: ecflow

The :term:`zombie` attribute defines how a :term:`zombie` should be handled in an automated fashion

Very careful consideration should be taken before this attribute is added
as it may hide a genuine problem.
It can be added to any :term:`node`. But is best defined at the :term:`suite` or :term:`family` level.
If there is no zombie attribute the default behaviour is to block the init,complete,abort :term:`child command`.
and *fob* the event,label,and meter :term:`child command`
This attribute allows the server to make a automated response.
Please see: :py:class:`ecflow.ZombieType`, :py:class:`ecflow.ChildCmdType`, :py:class:`ecflow.ZombieUserActionType`

Constructor::

   ZombieAttr(ZombieType,ChildCmdTypes, ZombieUserActionType, lifetime)
      ZombieType            : Must be one of ZombieType.ecf, ZombieType.path, ZombieType.user
      ChildCmdType          : A list(ChildCmdType) of Child commands. Can be left empty in
                              which case the action affect all child commands
      ZombieUserActionType  : One of [ fob, fail, block, remove, adopt ]
      int lifetime<optional>: Defines the life time in seconds of the zombie in the server.
                              On expiration, zombie is removed automatically

Usage:

.. code-block:: python

   # Add a zombie attribute so that child commands(i.e ecflow_client --init)
   # will fail the job if it is a zombie process.
   s1 = Suite('s1')
   child_list = [ ChildCmdType.init, ChildCmdType.complete, ChildCmdType.abort ]
   s1.add_zombie( ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fob))

   # create the zombie as part of the node constructor
   s1 = Suite('s1',
              ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fail))


.. py:property:: Node.avisos
   :module: ecflow

Returns a list of :term:`aviso`\ s


.. py:method:: Node.change_complete(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.change_trigger(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:property:: Node.crons
   :module: ecflow

Returns a list of :term:`cron`\ s


.. py:property:: Node.dates
   :module: ecflow

Returns a list of :term:`date`\ s


.. py:property:: Node.days
   :module: ecflow

Returns a list of :term:`day`\ s


.. py:method:: Node.delete_complete(self: ecflow.Node) -> None
   :module: ecflow


.. py:method:: Node.delete_cron(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_cron(self: ecflow.Node, arg0: str) -> None

2. delete_cron(self: ecflow.Node, arg0: ecflow.Cron) -> None


.. py:method:: Node.delete_date(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_date(self: ecflow.Node, arg0: str) -> None

2. delete_date(self: ecflow.Node, arg0: ecflow.Date) -> None


.. py:method:: Node.delete_day(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_day(self: ecflow.Node, arg0: str) -> None

2. delete_day(self: ecflow.Node, arg0: ecflow.Day) -> None


.. py:method:: Node.delete_event(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_generic(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_inlimit(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_label(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_limit(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_meter(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_queue(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_repeat(self: ecflow.Node) -> None
   :module: ecflow


.. py:method:: Node.delete_time(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_time(self: ecflow.Node, arg0: str) -> None

2. delete_time(self: ecflow.Node, arg0: ecflow.Time) -> None


.. py:method:: Node.delete_today(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_today(self: ecflow.Node, arg0: str) -> None

2. delete_today(self: ecflow.Node, arg0: ecflow.Today) -> None


.. py:method:: Node.delete_trigger(self: ecflow.Node) -> None
   :module: ecflow


.. py:method:: Node.delete_variable(self: ecflow.Node, arg0: str) -> None
   :module: ecflow


.. py:method:: Node.delete_zombie(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. delete_zombie(self: ecflow.Node, arg0: str) -> None

2. delete_zombie(self: ecflow.Node, arg0: ecflow.ZombieType) -> None


.. py:method:: Node.evaluate_complete(self: ecflow.Node) -> bool
   :module: ecflow

evaluate complete expression


.. py:method:: Node.evaluate_trigger(self: ecflow.Node) -> bool
   :module: ecflow

evaluate trigger expression


.. py:property:: Node.events
   :module: ecflow

Returns a list of :term:`event`\ s


.. py:method:: Node.find_event(self: ecflow.Node, arg0: str) -> ecflow.Event
   :module: ecflow

Find the :term:`event` on the node only. Returns a object


.. py:method:: Node.find_gen_variable(self: ecflow.Node, arg0: str) -> ecflow.Variable
   :module: ecflow

Find generated variable on the node only. Returns an object


.. py:method:: Node.find_generic(self: ecflow.Node, arg0: str) -> ecflow.Generic
   :module: ecflow

Find the :term:`generic` on the node only. Returns a Generic object


.. py:method:: Node.find_label(self: ecflow.Node, arg0: str) -> ecflow.Label
   :module: ecflow

Find the :term:`label` on the node only. Returns a object


.. py:method:: Node.find_limit(self: ecflow.Node, arg0: str) -> ecflow.Limit
   :module: ecflow

Find the :term:`limit` on the node only. returns a limit ptr


.. py:method:: Node.find_meter(self: ecflow.Node, arg0: str) -> ecflow.Meter
   :module: ecflow

Find the :term:`meter` on the node only. Returns an object


.. py:method:: Node.find_node_up_the_tree(self: ecflow.Node, arg0: str) -> ecflow.Node
   :module: ecflow

Search immediate node, then up the node hierarchy


.. py:method:: Node.find_parent_variable(self: ecflow.Node, arg0: str) -> ecflow.Variable
   :module: ecflow

Find user variable variable up the parent hierarchy. Returns an object


.. py:method:: Node.find_parent_variable_sub_value(self: ecflow.Node, arg0: str) -> str
   :module: ecflow

Find user variable *up* node tree, then variable substitute the value, otherwise return empty string


.. py:method:: Node.find_queue(self: ecflow.Node, arg0: str) -> ecflow.Queue
   :module: ecflow

Find the queue on the node only. Returns a queue object


.. py:method:: Node.find_variable(self: ecflow.Node, arg0: str) -> ecflow.Variable
   :module: ecflow

Find user variable on the node only. Returns an object


.. py:property:: Node.generics
   :module: ecflow

Returns a list of :term:`generic`\ s


.. py:method:: Node.get_abs_node_path(self: ecflow.Node) -> str
   :module: ecflow

returns a string which holds the path to the node


.. py:method:: Node.get_all_nodes(self: ecflow.Node) -> ecflow.NodeVec
   :module: ecflow

Returns all the child nodes


.. py:method:: Node.get_autoarchive(self: ecflow.Node) -> ecflow.Autoarchive
   :module: ecflow


.. py:method:: Node.get_autocancel(self: ecflow.Node) -> ecflow.Autocancel
   :module: ecflow


.. py:method:: Node.get_autorestore(self: ecflow.Node) -> ecflow.Autorestore
   :module: ecflow


.. py:method:: Node.get_complete(self: ecflow.Node) -> ecflow.Expression
   :module: ecflow


.. py:method:: Node.get_defs(self: ecflow.Node) -> Defs
   :module: ecflow


.. py:method:: Node.get_defstatus(self: ecflow.Node) -> ecflow.DState
   :module: ecflow


.. py:method:: Node.get_dstate(self: ecflow.Node) -> ecflow.DState
   :module: ecflow

Returns the state of node. This will include suspended state


.. py:method:: Node.get_flag(self: ecflow.Node) -> ecflow.Flag
   :module: ecflow

Return additional state associated with a node.


.. py:method:: Node.get_generated_variables(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. get_generated_variables(self: ecflow.Node) -> list

Returns the list of generated variables.

2. get_generated_variables(self: ecflow.Node, arg0: ecflow.VariableList) -> None

Retrieves the list of generated variables. Pass in ecflow.VariableList as argument to hold variables.


.. py:method:: Node.get_late(self: ecflow.Node) -> ecflow.Late
   :module: ecflow


.. py:method:: Node.get_parent(self: ecflow.Node) -> ecflow.Node
   :module: ecflow


.. py:method:: Node.get_repeat(self: ecflow.Node) -> ecflow.Repeat
   :module: ecflow


.. py:method:: Node.get_state(self: ecflow.Node) -> ecflow.State
   :module: ecflow

Returns the state of the node. This excludes the suspended state


.. py:method:: Node.get_state_change_time(self: ecflow.Node, format: str = 'iso_extended') -> str
   :module: ecflow

Returns the time of the last state change as a string. Default format is iso_extended, (iso_extended, iso, simple)


.. py:method:: Node.get_trigger(self: ecflow.Node) -> ecflow.Expression
   :module: ecflow


.. py:method:: Node.has_time_dependencies(self: ecflow.Node) -> bool
   :module: ecflow


.. py:property:: Node.inlimits
   :module: ecflow

Returns a list of :term:`inlimit`\ s


.. py:method:: Node.is_suspended(self: ecflow.Node) -> bool
   :module: ecflow

Returns true if the :term:`node` is in a :term:`suspended` state


.. py:property:: Node.labels
   :module: ecflow

Returns a list of :term:`label`\ s


.. py:property:: Node.limits
   :module: ecflow

Returns a list of :term:`limit`\ s


.. py:property:: Node.meters
   :module: ecflow

Returns a list of :term:`meter`\ s


.. py:property:: Node.mirrors
   :module: ecflow

Returns a list of :term:`mirror`\ s


.. py:method:: Node.name(self: ecflow.Node) -> str
   :module: ecflow


.. py:property:: Node.queues
   :module: ecflow

Returns a list of :term:`queue`\ s


.. py:method:: Node.remove(self: ecflow.Node) -> ecflow.Node
   :module: ecflow

Remove the node from its parent. and returns it


.. py:method:: Node.replace_on_server(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. replace_on_server(self: ecflow.Node, suspend_node_first: bool = True, force: bool = True) -> None

replace node on the server.

2. replace_on_server(self: ecflow.Node, host: str, port: str, suspend_node_first: bool = True, force: bool = True) -> None

replace node on the server.

3. replace_on_server(self: ecflow.Node, host_port: str, suspend_node_first: bool = True, force: bool = True) -> None

replace node on the server.

4. replace_on_server(self: ecflow.Node, client: ClientInvoker, suspend_node_first: bool = True, force: bool = True) -> None

replace node on the server.


.. py:method:: Node.sort_attributes(*args, **kwargs)
   :module: ecflow

Overloaded function.

1. sort_attributes(self: ecflow.Node, arg0: ecflow.AttrType) -> None

2. sort_attributes(self: ecflow.Node, arg0: ecflow.AttrType, arg1: bool) -> None

3. sort_attributes(self: ecflow.Node, arg0: ecflow.AttrType, arg1: bool, arg2: list) -> None

4. sort_attributes(self: ecflow.Node, attribute_type: str, recursive: bool = True, no_sort: list = []) -> None

5. sort_attributes(self: ecflow.Node, attribute_type: ecflow.AttrType, recursive: bool = True, no_sort: collections.abc.Sequence[str] = []) -> None


.. py:property:: Node.times
   :module: ecflow

Returns a list of :term:`time`\ s


.. py:property:: Node.todays
   :module: ecflow

Returns a list of :term:`today`\ s


.. py:method:: Node.update_generated_variables(self: ecflow.Node) -> None
   :module: ecflow


.. py:property:: Node.variables
   :module: ecflow

Returns a list of user defined :term:`variable`\ s


.. py:property:: Node.verifies
   :module: ecflow

Returns a list of Verify's


.. py:property:: Node.zombies
   :module: ecflow

Returns a list of :term:`zombie`\ s

