.. index::
   single: Glossary
   
.. _glossary:
   
============
**Glossary**
============

.. glossary::
   :sorted:

   ecFlow
      Is the ECMWF work flow manager.
      
      A general purpose application designed to schedule a large number of
      computer process in a heterogeneous environment.
      
      Helps computer jobs design, submission and monitoring both in
      the research and operation departments.

   status
      Each :term:`node` in :term:`suite definition` has a status. 
      
      Status reflects the state of the :term:`node`. 
      In :term:`ecflowview` the background colour of the text reflects the status.  
      
      :term:`task` status are: :term:`unknown`, :term:`queued`, :term:`submitted`, :term:`active`, :term:`complete`, :term:`aborted` and :term:`suspended`
      
      :term:`ecflow_server` status are: :term:`shutdown`, :term:`halted`, :term:`running` 
      this is shown on the root node in :term:`ecflowview`
      
   unknown
      Is a :term:`node` :term:`status`. 
      
      This is the default :term:`node` :term:`status` when a :term:`suite definition` is loaded into the :term:`ecflow_server`
      
   queued
      Is a :term:`node` :term:`status`. 
      
      After the begin command, the task **without** a :term:`defstatus` are placed into the queued state
      
   submitted
      Is a :term:`node` :term:`status`. 
      
      When the :term:`task` :term:`dependencies` are resolved/free the :term:`ecflow_server` places the task into a submitted state.
      However if the ECF_JOB_CMD fails, the task is placed into the :term:`aborted` state
      
   active
      Is a :term:`node` :term:`status`. 
      
      If :term:`job creation` was successful, and :term:`job file` has started, then the :term:`ecflow_client` --init
      :term:`child command` is received by the :term:`ecflow_server` and the :term:`task` is placed into a active state
      
   complete
      Is a :term:`node` :term:`status`. 
      
      The node can be set to complete:
      
          By the :term:`complete expression`
          
          At job end when the :term:`task` receives the :term:`ecflow_client` --complete :term:`child command`
      
   check point 
      The check point file is like the :term:`suite definition`, but includes all the state information.
      
      It is periodically saved by the :term:`ecflow_server`. 
      
      It can be used to recover the state of the node tree should server die, or machine crash.
      
      By default when a :term:`ecflow_server` is started it will look to load the check point file. 
      
      The default check point file name is <host>.<port>.ecf.check. This can be overridden by the ECF_CHECK environment variable
      
   aborted
      Is a :term:`node` :term:`status`. 
      
      When the ECF_JOB_CMD fails or the :term:`job file` sends a :term:`ecflow_client` --abort :term:`child command`, then
      the task is placed into a aborted state.
      
   suspended
      Is a :term:`node` state. A :term:`node` can be placed into the suspended state via a :term:`defstatus` or via :term:`ecflowview`
      
      A suspended :term:`node` including any of its children can not take part in :term:`scheduling` until
      the node is resumed.

   shutdown
      Is a :term:`ecflow_server` state. See :term:`server states`
      
   halted
      Is a :term:`ecflow_server` state. See :term:`server states`
      
   running
      Is a :term:`ecflow_server` state. See :term:`server states`
   
   child command
      Child command's(or task requests) are called from within the :term:`ecf script` files. They include:
      
         ===================================  ======================================================
         Child Command                        Description
         ===================================  ======================================================
         :term:`ecflow_client`  --init        Sets the :term:`task` to the :term:`active` :term:`status`
         
         :term:`ecflow_client`  --event       Set an event 
      
         :term:`ecflow_client`  --meter       Change a meter 
      
         :term:`ecflow_client`  --label       Change a label 
            
         :term:`ecflow_client`  --wait        wait for a expression to evaluate.  
      
         :term:`ecflow_client`  --abort       Sets the :term:`task` to the :term:`aborted` :term:`status`
         
         :term:`ecflow_client`  --complete    Sets the :term:`task` to the :term:`complete` :term:`status`
         ===================================  ======================================================
         
      The following environment variables must be set for the child commands. ECF_HOST, ECF_NAME ,ECF_PASS  and
      ECF_RID. See :term:`ecflow_client`.                                         
       
      
   ecf script
      The ecFlow script refers to an ‘.ecf’ file.  
      
      The script file is transformed into the :term:`job file` by the :term:`job creation` process.
      
      The base name of the script file **must** match its corresponding :term:`task`. i.e t1.ecf , corresponds to the task of name 't1'.
      The script if placed in the ECF_FILES directory, may be re-used by multiple tasks belonging to different families,
      providing the :term:`task` name matches.
      
      The ecFlow script is similar to a UNIX shell script.  
      
      The differences, however, includes the addition of “C” like pre-processing :term:`directives` and ecFlow :term:`variable`'s.
      Also the script *must* include calls to the **init** and **complete** :term:`child command` s so that
      the :term:`ecflow_server` is aware when the job starts(i.e changes state to :term:`active`) and finishes ( i.e changes state to :term:`complete`)
       
      
   job file
      The job file is created by the :term:`ecflow_server` during :term:`job creation` using the :term:`ECF_TRYNO` :term:`variable`
      
      It is derived from the :term:`ecf script` after expanding the pre-processing :term:`directives`. 
      
      It has the form <task name>.job<:term:`ECF_TRYNO`>", i.e. t1.job1. 
      
      Note job creation checking will create a job file with an extension with zero. i.e '.job0'. See :py:class:`ecflow.Defs.check_job_creation`
      
      When the job is run the output file has the :term:`ECF_TRYNO` as the extension.
      i.e t1.1 where 't1' represents the task name and '1' the :term:`ECF_TRYNO`  
      
   manual page
      Manual pages are part of the :term:`ecf script`. 
      
      This is to ensure that the manual page is updated when the :term:`ecf script` is updated. 
      The manual page is a very important operational tool allowing you to view a description of a task,
      and possibly describing solutions to common problems.  
      The :term:`pre-processing` can be used to extract the manual page from the script file and is visible in :term:`ecflowview`.  
      The manual page is the text contained within the %manual and %end :term:`directives`.  
      They can be seen using the manual button on :term:`ecflowview`.
      
      The text in the manual page in **not** included in the :term:`job file`.
      
      There can be multiple manual sections in the same :term:`ecf script` file. When viewed they are
      simply concatenated. It is good practice to modify the manual pages when the script changes.
      
      The manual page may have the %include :term:`directives`.
            
   suite
      A suite is organisational entity. It is serves as the root :term:`node` in a :term:`suite definition`.
      It should be used to hold a set of jobs that achieve a common function. It can be used to hold
      user :term:`variable` s that are common to all of its children.
      
      Only a suite node can have a :term:`clock`.
      
      It is a collection of :term:`family`'s, :term:`variable`'s, :term:`repeat` and a single
      :term:`clock` definition. For a complete list of attributes look at BNF for :token:`suite`.
      For python see :py:class:`ecflow.Suite`.
      
   family
      A family is an organisational entity that is used to provide hierarchy and grouping. 
      It consists of a collection of :term:`task`'s and families.
      
      Typically you place tasks that are related to each other inside the same family, analogous to the way you 
      create directories to contain related files. 
      For python see :py:class:`ecflow.Family`. For BNF see :token:`family`
      
      It serves as an intermediate :term:`node` in a :term:`suite definition`.
      
   task
      A task represents a job that needs to be carried out. 
      It serves as a leaf :term:`node` in a :term:`suite definition`
       
      Only tasks can be submitted. 
      
      A job inside a task :term:`ecf script` should generally be re-entrant
      so that no harm is done by rerunning it, since a task may be automatically 
      submitted more than once if it aborts.
      
      For python see :py:class:`ecflow.Task`. For text BNF see :token:`task`
            
   node
      :term:`suite`, :term:`family` and :term:`task` form a hierarchy.
      Where a :term:`suite` serves as the root of the hierarchy. 
      The :term:`family` provides the intermediate nodes, and the :term:`task`
      provide the leaf's. 
      
      Collectively :term:`suite`, :term:`family` and :term:`task` can be referred
      to as nodes.     
      
      For python see :py:class:`ecflow.Node`.  
      
   event
      The purpose of an event is to signal partial completion of a :term:`task` and to be able to 
      trigger another job which is waiting for this partial completion. 
      
      Only tasks can have events and they can be considered as an attribute of a :term:`task`. 
      
      There can be many events and they are displayed as nodes. 
      
      The event is updated by placing the --event :term:`child command` in a :term:`ecf script`.
      
      An event has a number and possibly a name. If it is only defined as a number, 
      its name is the text representation of the number without leading zeroes.
      
      For python see: :py:class:`ecflow.Event` and :py:class:`ecflow.Node.add_event` For text BNF see :token:`event`
      
      Events can be referenced in :term:`trigger` and :term:`complete expression` s.
     
   meter
      The purpose of a meter is to signal proportional completion of a task and to 
      be able to trigger another job which is waiting on this proportional completion.
      
      The meter is updated by placing the --meter :term:`child command` in a :term:`ecf script`.
      
      For python see: :py:class:`ecflow.Meter` and :py:class:`ecflow.Node.add_meter`. For text BNF see :token:`meter`
      
      Meter's can be referenced in :term:`trigger` and :term:`complete expression` expressions.
      
   label
      A label has a name and a value and is a way of **displaying** information in :term:`ecflowview`
      
      By placing a label :term:`child command` s in the :term:`ecf script` the user can be informed about progress
      in :term:`ecflowview`.
      
      For python see :py:class:`ecflow.Label` and :py:class:`ecflow.Node.add_label`. For text BNF see :token:`label`
      
   limit
      Limits provide simple load management by limiting the number of tasks
      submitted by a specific :term:`ecflow_server`.
      Typically you either define limits on :term:`suite` level or define a separate suite to 
      hold limits so that they can be used by multiple suites. 
      
      The limit max value can be changed on the command line ::
      
         >ecflow_client --alter change limit_max <limit-name> <new-limit-value> <path-to-limit>
         >ecflow_client --alter change limit_max limit 2 /suite
         
      It can also be changed in python ::
      
         #!/usr/bin/env python2.7
         import ecflow 
         try:
            ci = ecflow.Client()
            ci.alter("/suite","change","limit_max","limit", "2")   
         except RuntimeError, e:
            print "Failed: " + str(e)
      
      For python see :py:class:`ecflow.Limit` and :py:class:`ecflow.Node.add_limit`. For BNF see :token:`limit` and :term:`inlimit`
      
   inlimit
      The inlimit works in conjunction with :term:`limit`/:py:class:`ecflow.Limit` for providing simple load management
      
      inlimit is added to the :term:`node` that needs to be limited.
      
      For python see :py:class:`ecflow.InLimit` and :py:class:`ecflow.Node.add_inlimit`. For text BNF see :term:`inlimit`
      
   dependencies
      Dependencies are attributes of node, that can suppress/hold a :term:`task` from taking part in :term:`job creation`.
      
      They include :term:`trigger`, :term:`date`, :term:`day`, :term:`time`, :term:`today`, :term:`cron`, :term:`complete expression`, :term:`inlimit` and  :term:`limit`. 
      
      A :term:`task` that is dependent can not be started as long as some dependency is holding it or any of its **parent** :term:`node` s.
      
      The :term:`ecflow_server` will check the dependencies every minute, during normal :term:`scheduling` **and** when any
      :term:`child command` causes a state change in the :term:`suite definition`.
      
   trigger
      Triggers defines a dependency for a :term:`task` or :term:`family`. 
      
      There can be only one trigger dependency per :term:`node`, 
      but that can be a complex boolean expression of the :term:`status` of several nodes. 
      Triggers should be avoided on suites.
      A node with a trigger can only be activated when its trigger has expired. 
      A trigger holds the node as long as the trigger's expression evaluation returns false. 
      
      Trigger evaluation occurs when ever the :term:`child command` communicates with the server. i.e whenever
      there is a state change in the suite definition.
      
      The keywords in trigger expressions are: :term:`unknown`, :term:`suspended`, :term:`complete`, :term:`queued`, :term:`submitted`, :term:`active`, :term:`aborted`
      and **clear** and **set** for :term:`event` status.
      
      Triggers can also reference Node attributes like :term:`event`, :term:`meter`, :term:`variable`, :term:`repeat` and generated variables.
      Trigger evaluation for node attributes uses integer arithmetic:
      
      - :term:`event` has the integer value of 0(clear) and set(1)
      - :term:`meter` values are integers hence they are used as is
      - :term:`variable` value is converted to an integer, otherwise 0 is used. See example below
      - :term:`repeat` *string* :     We use the index values as integers. See example below
      - :term:`repeat` *enumerated* : We use the index values as integers. See example below
      - :term:`repeat` *integer* :    Use the implicit integer values
      - :term:`repeat` *date* :       Use the date values as integers. Use of plus/minus on repeat date variable uses date arithmetic
      
      Here are some examples ::
     
         suite trigger_suite
            task a
               event EVENT
               meter METER 1 100 50
               edit  VAR_INT 12
               edit  VAR_STRING "captain scarlett"         # This is not convertible to an integer, if referenced will use '0'
            family f1
               edit SLEEP 2
               repeat string NAME a b c d e f              # This has values: a(0), b(1), c(3), d(4), e(5), f(6) i.e index
               family f2
                  repeat integer VALUE 5 10                # This has values: 5,6,7,8,9,10
                  family f3
                     repeat enumerated red green blue      # red(0), green(1), blue(2)
                     task t1
                        repeat date DATE 19991230 20000102 # This has values: 19991230,19991231,20000101,20000102
                  endfamily
               endfamily
            endfamily
            family f2
               task event_meter
                   trigger /suite/a:EVENT == set and /suite/a:METER >= 30
               task variable
                   trigger /suite/a:VAR_INT >= 12 and /suite/a:VAR_STRING == 0
               task repeat_string
                   trigger /suite/f1:NAME >= 4
               task repeat_integer
                   trigger /suite/f1/f2:VALUE >= 7
               task repeat_date
                   trigger /suite/f1/f2/f3/t1:DATE >= 19991231
               task repeat_date2
                   # Using plus/minus on a repeat DATE will use date arithmetic
                   # Since the starting value of DATE is 19991230, this task will run straight away
                   trigger /suite/f1/f2/f3/t1:DATE - 1 == 19991229
            endfamily
         endsuite
  
      What happens when we have multiple node attributes of the same name, referenced in trigger expressions ? ::
      
         task foo
            event blah
            meter blah 0 200 50
            edit  blah 10
         task bar
            trigger foo:blah >= 0
            
      In this case ecFlow will use the following precedence:
      
      - :term:`event` 
      - :term:`meter`
      - :term:`variable`
      - :term:`repeat`
      - generated variables
      
      Hence in the example above expression 'foo:blah >= 0' will reference the event.
      
      For python see :py:class:`ecflow.Expression` and :py:class:`ecflow.Node.add_trigger`
       
   complete expression
      Force a node to be complete **if** the expression evaluates, without running any of the nodes. 
      
      This allows you to have tasks in the suite which a run only if others fail. 
      In practice the node would need to have a :term:`trigger` also. 
      
      For python see :py:class:`ecflow.Expression` and :py:class:`ecflow.Node.add_complete`
      
   date
      This defines a date dependency for a node. 
      
      There can be multiple date dependencies. 
      The European format is used for dates, which is: dd.mm.yy as in 31.12.2007. 
      Any of the three number fields can be expressed with a wildcard `*` to mean any valid value. 
      Thus, 01.*.* means the first day of every month of every year.

      If a :term:`hybrid clock` is defined, any node held by a date dependency will be set to :term:`complete` at the beginning
      of the :term:`suite`, without running the corresponding job. Otherwise under a hybrid clock the :term:`suite` would
      never :term:`complete`.
      
      For python see: :py:class:`ecflow.Date` and :py:class:`ecflow.Node.add_date`. For text BNF see :token:`date`
       
   day
      This defines a day dependency for a  node.
      
      There can be multiple day dependencies.
      
      If a :term:`hybrid clock` is defined, any node held by a day dependency will be set to :term:`complete` at the beginning
      of the :term:`suite`, without running the corresponding job. Otherwise under a hybrid clock the :term:`suite` would
      never :term:`complete`.
      
      For python see: :py:class:`ecflow.Day` and :py:class:`ecflow.Node.add_day`. For text BNF see :token:`day`
       
   time
      This defines a time dependency for a node. 
      
      Time is expressed in the format [h]h:mm. 
      Only numeric values are allowed. There can be multiple time dependencies for a node, 
      but overlapping times may cause unexpected results. 
      To define a series of times, specify the start time, end time and a time increment. 
      If the start time begins with '+', times are relative to the beginning of the suite or, 
      in repeated families, relative to the beginning of the repeated family.
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g. ::
      
         time 10:00 20:00 01:00 
         
      if the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      For python see :py:class:`ecflow.Time` and :py:class:`ecflow.Node.add_time`. For BNF see :token:`time`
      
   today
      Like :term:`time`, but If the suites begin time is **past** the time given for the "today" command the :term:`node` is free
      to run (as far as the time dependency is concerned). 
      
      For example ::
      
         task x
            today 10:00
            
      If we begin or re-queue the :term:`suite` at 9.00 am, then the :term:`task` in held until 10.00 am.
      However if we begin or re-queue the suite at 11.00am, the :term:`task` is run immediately.
      
      No lets look at time ::
      
         task x
            time 10:00
            
      If we begin or re-queue the :term:`suite` at 9.00am, then the :term:`task` in held until 10.00 am.
      If we begin or re-queue the :term:`suite` at 11.00am, the :term:`task` is still held.      
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g. ::
      
         today 10:00 20:00 01:00 
         
      if the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      For python see :py:class:`ecflow.Today`. For text BNF see :token:`today`
            
   late
      Define a tag for a node to be late. 
      
      Suites cannot be late, but you can define a late tag 
      for submitted in a suite, to be inherited by the families and tasks. 
      When a node is classified as being late, the only action :term:`ecflow_server` takes is to set a flag. 
      :term:`ecflowview` will display these alongside the :term:`node` name as an icon (and optionally pop up a window).
      
      For python see :py:class:`ecflow.Late` and :py:class:`ecflow.Node.add_late`. For text BNF see :token:`late`
            
   cron
      Like :term:`time`, cron defines time dependency for a :term:`node`, but it will be repeated indefinitely ::
            
         cron 11:00
         cron 10:00 22:00 00:30   # <start> <finish> <increment>
      
      When the node becomes complete it will be :term:`queued` immediately. This means that the suite
      will never complete, and the output is not directly accessible through :term:`ecflowview`
      
      If tasks abort, the :term:`ecflow_server` will not schedule it again.
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g. ::
      
         cron 10:00 20:00 01:00 
         
      if the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      If the cron defines months, days of the month, or week days or a single time slot
      the it relies on a day change, hence if a :term:`hybrid clock` is defined, 
      then it will be set to :term:`complete` at  the beginning of the :term:`suite`, 
      without running  the corresponding job. 
      Otherwise under a hybrid clock the :term:`suite` would never :term:`complete`.
      
      For python see :py:class:`ecflow.Cron` and :py:class:`ecflow.Node.add_cron`. For text BNF see :token:`cron`
       
   repeat
      Repeats provide looping functionality. There can only be a single repeat on a :term:`node`.
      
         repeat day step [ENDDATE]                       # only for suites
         
         repeat integer VARIABLE start end [step]
         
         repeat enumerated VARIABLE first [second [third ...]]
         
         repeat string VARIABLE str1 [str2 ...]
         
         repeat file VARIABLE filename
         
         repeat date VARIABLE yyyymmdd yyyymmdd [delta]
      
      The repeat VARIABLE can be used in :term:`trigger` and :term:`complete expression` expressions.
      
      If a "repeat date" VARIABLE is used in a trigger expression then date arithmetic is used,
      when the expression uses addition and subtraction. i.e ::
      
         defs = ecflow.Defs()
         s1 = defs.add_suite("s1");
         t1 = s1.add_task("t1").add_repeat( ecflow.RepeatDate("YMD",20090101,20091231,1) );
         t2 = s1.add_task("t2").add_trigger("t1:YMD - 1 eq 20081231");
         assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101 - 1  == 20081231"
      
      For python see :py:class:`ecflow.Node.add_repeat`, :py:class:`ecflow.Repeat`, :py:class:`ecflow.RepeatDate`, :py:class:`ecflow.RepeatEnumerated`, :py:class:`ecflow.RepeatInteger`, :py:class:`ecflow.RepeatDay`
      For text BNF see :token:`repeat`
      
   autocancel
      autocancel is a way to automatically delete a :term:`node` which has completed.
      
      The delete may be delayed by an amount of time in hours and minutes or 
      expressed in days. Any node may have a single autocancel attribute. 
      If the auto cancelled node is referenced in the :term:`trigger` expression of other nodes
      it may leave the node waiting. This can be solved by making sure the :term:`trigger`
      expression also checks for the :term:`unknown` state. i.e.::
      
         trigger node_to_cancel == complete or node_to_cancel == unknown
      
      This guards against the 'node_to_cancel' being undefined or deleted
      
      For python see :py:class:`ecflow.Autocancel` and :py:class:`ecflow.Node.add_autocancel`. For text BNF see :token:`autocancel`
       
   clock
      A clock is an attribute of a :term:`suite`. 
      
      A gain can be specified to offset from the given date.
      
      The hybrid and real clock's always runs in phase with the system clock (UTC in UNIX) 
      but can have any offset from the system clock. 
      
      The clock can be :
      
         * :term:`hybrid clock`
          
         * :term:`real clock`
          
         * :term:`virtual clock`
         
      :term:`time`, :term:`day` and :term:`date` and :term:`cron` :term:`dependencies` 
      work a little differently under the clocks. 
      
      The default clock type is hybrid. 
      
      If the :term:`ecflow_server` is :term:`shutdown` or :term:`halted` the job :term:`scheduling` is suspended.
      If this suspension is left for period of time, then it can affect task submission under **hybrid** and **real** clocks.
      In particular it will affect :term:`task` s with :term:`time`, :term:`today` or :term:`cron` :term:`dependencies`.
          
         - :term:`dependencies` with time series, can result in missed time slots::
         
               time 10:00 20:00 00:15    # If server is suspended > 15 minutes, time slots can be missed            
               time +00:05 20:00 00:15   # start 5 minutes after the start of the suite, then every 15m until 20:00
         
         - When the server is placed back into :term:`running` state any time :term:`dependencies`
           with an expired time slot are submitted straight away. i.e if :term:`ecflow_server` is
           :term:`halted` at 10:59 and then placed back into :term:`running` state at 11:20
           
               time 11:00
           
           Then any :term:`task` with a expired single time slot dependency will be submitted straight away.

      For python see :py:class:`ecflow.Clock` and :py:class:`ecflow.Suite.add_clock`. For text BNF see :token:`clock`
       
       
   hybrid clock      
      A hybrid :term:`clock` is a complex notion: the date and time are not connected. 
      
      The date has a fixed value during the complete execution of the :term:`suite`. 
      This will be mainly used in cases where the suite does not :term:`complete` in less than 24 hours. 
      This guarantees that all tasks of this suite are using the same :term:`date`.
      On the other hand, the time follows the time of the machine.
                         
      Hence the :term:`date` never changes unless specifically altered or unless the suite restarts,
      either automatically or from a begin command. 
       
      Under a hybrid :term:`clock` any :term:`node` held by a :term:`date` or :term:`day` dependency 
      will be set to complete at the beginning of the suite. (i.e without its job ever running).
      Otherwise the :term:`suite` would never :term:`complete`.
      
   real clock
       A :term:`suite` using a real :term:`clock` will have its :term:`clock` matching the clock of the machine.
       Hence the :term:`date` advances by one day at midnight. 
   
   virtual clock
      Like :term:`real clock` until the :term:`ecflow_server` is suspended (i.e :term:`shutdown` or :term:`halted`), 
      the suites :term:`clock` is also suspended.
      
      Hence will honour relative times in :term:`cron`, :term:`today` and :term:`time` dependencies.
      It is possible to have a combination of hybrid/real and virtual.
      
      More useful when we want complete adherence to time related dependencies at the expense
      being out of sync with system time.
      
   defstatus
      Defines the default :term:`status` for a task/family to be assigned to the :term:`node` when the begin command is issued.
      
      By default :term:`node` gets queued when you use begin on a :term:`suite`. 
      defstatus is useful in preventing suites from running automatically once begun or in setting
      tasks complete so they can be run selectively. 

      For python see :py:class:`ecflow.DState` and :py:class:`ecflow.Node.add_defstatus`. For text BNF see :token:`defstatus`
       
   extern
      This allows an external :term:`node` to be used in a :term:`trigger` expression. 
      
      All :term:`node`'s in :term:`trigger`'s must be known to :term:`ecflow_server` by the end of the load command. 
      No cross-suite :term:`dependencies` are allowed unless the names of tasks outside the suite are declared as external. 
      An external :term:`trigger` reference is considered unknown if it is not defined when the :term:`trigger` is evaluated. 
      You are strongly advised to avoid cross-suite :term:`dependencies`. 
      
      Families and suites that depend on one another should be placed in a single :term:`suite`. 
      If you think you need cross-suite dependencies, you should consider merging the suites 
      together and have each as a top-level family in the merged suite.
      For BNF see :token:`extern`
      
   variable
      ecFlow makes heavy use of different kinds of variables.There are several kinds of variables:
      
         Environment variables: which are set in the UNIX shell before the :term:`ecFlow` starts.  
         These control :term:`ecflow_server`, and :term:`ecflow_client`  .
           
         suite definition variables: Also referred to as user variables. These control :term:`ecflow_server`, and :term:`ecflow_client` and
         are available for use in :term:`job file`.  
         
         Generated variables: These are generated within the :term:`suite definition` node tree during :term:`job creation` 
         and are available for use in the :term:`job file`.
         
      Variables can be referenced in :term:`trigger` and :term:`complete expression` s . The value part of the variable should
      be convertible to an integer otherwise a default value of 0 is used.
      
      For python see :py:class:`ecflow.Node.add_variable`. For BNF see :token:`variable`
      
   variable inheritance
      When a :term:`variable` is needed at :term:`job creation` time, it is first sought in the :term:`task` itself.
       
      If it is not found in the :term:`task`, it is sought from the task's parent and so on, up through 
      the :term:`node` levels until found.
      
      For any :term:`node`, there are two places to look for variables. 
      
      Suite definition variables are looked for first, and then any generated variables. 
      
   ecflowview
      ecflowview executable is the GUI based client, that is used to visualise and monitor
      the hierarchical structure of the :term:`suite definition`   
         
         state changes in the :term:`node`'s and the :term:`ecflow_server`, using colour coding
         
         Attributes of the nodes and any :term:`dependencies`
         
         :term:`ecf script` file and the corresponding :term:`job file`
      
   ecflow_client
      This executable is a command line program; it is used for all communication with the :term:`ecflow_server`.
      
      To see the full range of commands that can be sent to the :term:`ecflow_server` type the following in a UNIX shell:
      
         ecflow_client --help
         
      This functionality is also provided by the :ref:`client_server_python_api`.
      
      The following variables affect the execution of ecflow_client. 
      
      Since the :term:`ecf script` can call ecflow_client( i.e :term:`child command`) then typically
      some are set in an include header. i.e. :ref:`head_h`.
      
      ============== =========================================================== ========== ===================================
      Variable Name  Explanation                                                 Compulsory Example
      ============== =========================================================== ========== ===================================
      ECF_HOST       Name of the host running the :term:`ecflow_server`          Yes        pikachu
      ECF_NAME       Path to the task                                            Yes        /suite/family/task
      ECF_PASS       Jobs password.                                              Yes        (generated)
      ECF_RID        Remote id. Allow easier job kill, and disambiguate a zombie Yes        (generated)
                     from the real job.                                           
      ECF_PORT       Port number of the :term:`ecflow_server`                    No          3141.Must match :term:`ecflow_server`
      ECF_TRYNO      The number of times the job has run. This is allocated by   No         (generated)
                     the server and used in job/output file name generation.            
      ECF_HOSTFILE   File that lists alternate hosts to try, if connection to    No         /home/user/avi/.ecfhostfile
                     main host fails                                             
      ECF_TIMEOUT    Maximum time is seconds for the client to deliver message   No         24*3600 (default value)
      ECF_DENIED     If server denies client communication and this flag is set, No          1  
                     exit with an error. Avoids 24hr hour connection attempt to 
                     :term:`ecflow_server`.
      NO_ECF         If set exit's ecflow_client immediately with success. This  No          1
                     allows the scripts to be tested independent of the server
      ============== =========================================================== ========== ===================================
         
   ecflow_server
      This executable is the server. 
      
      It is responsible for :term:`scheduling` the jobs and responding to :term:`ecflow_client` requests
      
      Multiple servers can be run on the same machine/host providing they are assigned a unique port number.
      
      The server record's all request's in the log file.  
      
      The server will periodically(See ECF_CHECKINTERVAL) write out a :term:`check point` file. 
            
      The following environment variables control the execution of the server and may be set before the start of the server.
      ecflow_server will start happily with out any of these variables being set, since all of them have default values.
      
      =================  =========================================================  ======================================
      Variable Name      Explanation                                                Default value
      =================  =========================================================  ======================================
      ECF_HOME           Home for all the :term:`ecFlow` files                      Current working directory
      ECF_PORT           Server port number. Must be unique                         3141
      ECF_LOG            History or log file                                        <host>.<port>.ecf.log
      ECF_CHECK          Name of the checkpoint file                                <host>.<port>.ecf.check
      ECF_CHECKOLD       Name of the backup checkpoint file                         <host>.<port>.ecf.check.b
      ECF_CHECKINTERVAL  Interval in second to save :term:`check point` file        120                                                 
      ECF_LISTS          White list file. Controls read/write access to the server  <host>.<port>.ecf.lists
                         for each user
      =================  =========================================================  ======================================
      
      The server can be in several states. The default when first started is :term:`halted`, See :term:`server states`
      
   server states
      The following tables reflects the :term:`ecflow_server` capabilities in the different states

      ================  ============   ============  ==============  ===================
      State             User Request   Task Request  Job Scheduling  Auto-Check-pointing 
      ================  ============   ============  ==============  ===================
      :term:`running`      yes           yes            yes             yes               
      :term:`shutdown`     yes           yes            no              yes               
      :term:`halted`       yes           no             no              no                
      ================  ============   ============  ==============  ===================  
       
   scheduling
      The :term:`ecflow_server` is responsible for :term:`task` scheduling. 
      
      It will check :term:`dependencies` in the :term:`suite definition` every minute. 
      If these :term:`dependencies` are free, the :term:`ecflow_server` will submit the task. 
      See :term:`job creation`.
   
   suite definition
      The suite definition is the hierarchical :term:`node` tree. 
      
      It describes how your :term:`task`'s run and interact.
      
      It can built up using:
      
         * Ascii text file by following the rules defined in the ecFlow :ref:`grammer`.
         
           Hence any language can be used, to generate this format.
         
         * :ref:`suite_definition_python_api`
         
      Once the definition is built, it can be loaded into the :term:`ecflow_server`, and started. 
      It can be monitored by :term:`ecflowview`
   
   job creation
      Job creation or task invocation can be initiated manually via :term:`ecflowview` but also by 
      the :term:`ecflow_server` during :term:`scheduling` when a :term:`task` (and *all* of its parent :term:`node` s) 
      is free of its :term:`dependencies`.
             
      The process of job creation includes:
      
         o Generating a unique password ECF_PASS, which is placed in :term:`ecf script` during :term:`pre-processing`. See :ref:`head_h`
          
         o Locating :term:`ecf script` files , corresponding to the :term:`task` in the :term:`suite definition`, See :term:`ecf file location algorithm`
         
         o :term:`pre-processing` the contents of the :term:`ecf script` file       
                  
      The steps above transforms an :term:`ecf script` to a :term:`job file` that can be submitted by
      performing :term:`variable substitution` on the ECF_JOB_CMD :term:`variable` and invoking the command.
         
      The running jobs will communicate back to the :term:`ecflow_server` by calling :term:`child command`'s.
        
      This causes :term:`status` changes on the :term:`node`'s in the :term:`ecflow_server` and flags can be set 
      to indicate various events.  
      
      If a :term:`task` is to be treated as a dummy task( i.e. is used as a scheduling task) and is not meant to
      to be run, then a variable of name :term:`ECF_DUMMY_TASK` can be added. ::
      
         task.add_variable("ECF_DUMMY_TASK","")
      
      
   ecf file location algorithm
     :term:`ecflow_server` and job creation checking uses the following algorithm to locate the '.ecf' file corresponding to a :term:`task`:
      
     * ECF_SCRIPT
      
       First it uses the generated variable ECF_SCRIPT to locate the script. 
       This variable is generated from: ECF_HOME/<path to task>.ecf
        
       Hence if the task path is /suite/f1/f2/t1, then ECF_SCRIPT=ECF_HOME/suite/f1/f2/t1.ecf
        
     * ECF_FILES
      
       Second it checks for the user defined ECF_FILES variable. 
       If defined the value of this variable must correspond to a directory.
       This directory is searched in reverse order.
        
       i.e lets assume we have a task: /o/12/fc/model
       and ECF_FILES is defined as: /home/ecmwf/emos/def/o/ECFfiles
        
       The ecFlow will use the following search pattern.
        
           #. /home/ecmwf/emos/def/o/ECFfiles/o/12/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/12/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/model.ecf

     * ECF_HOME
      
       Thirdly it searchs for the script in reverse order using ECF_HOME (i.e like ECF_FILES) 
           
      If this fails, than the :term:`task` is placed into the :term:`aborted` state.  
      We can check that file can be located before loading the suites into the server.
      
         * :ref:`checking-job-creation`
         * :py:class:`ecflow.Defs.check_job_creation`    
   
   pre-processing
      Pre-processing takes place during :term:`job creation` and acts on :term:`directives` specified in :term:`ecf script` file.
      
      This involves:
      
         o expanding any include file :term:`directives`.  i.e similar to 'c' language pre-processing
         
         o removing comments and manual :term:`directives`
         
         o performing :term:`variable substitution`  
         
   variable substitution
      Takes place during :term:`pre-processing` *or* command invocation.(i.e ECF_JOB_CMD,ECF_KILL_CMD,etc)
      
      It involves searching each line of :term:`ecf script` file or command, for :term:`ECF_MICRO` character. typically '%'
      
      The text between two % character, defines a variable. i.e %VAR%
      
      This variable is searched for in the :term:`suite definition`. 
      
      First the suite definition variables( sometimes referred to as user variables) are searched
      and then Repeat variable name, and finally the generated variables.If no variable
      is found then the same search pattern is repeated up the node tree.
      
      The value of the :term:`variable` is replaced between the % characters. 
      
      If the micro character are not paired and an error message is written to the log file, 
      and the task is placed into the :term:`aborted` state.
      
      If the variable is not found in the :term:`suite definition` during pre-processing then :term:`job creation` fails, 
      and an error message is written to the log file, and the task is placed into the :term:`aborted` state.
      To avoid this, variables in the :term:`ecf script` can be defined as:
       
      ::
      
         %VAR:replacement% 
         
      This is similar to %VAR% but if VAR is not found in the :term:`suite definition` then 'replacement' is used.
      
   directives
      Directives start with a % charater. This is referred to as :term:`ECF_MICRO` character.
      
      The directives are used in two main context.
      
         - Preprocessing directives. In this case the directive starts as the **first** character on a line in a :term:`ecf script` file. 
           See the table below which shows the allowable values. Only one directive is allowed on the line.
           
         - Variable directives. We use two :term:`ECF_MICRO` characters ie %VAR%, in this case they can occur **anywhere** on 
           the line and in any number.  
           
           ::
           
            %CAR% %TYPE% %WISHLIST% 
            
           These directives take part in :term:`variable substitution`.
           
           If the micro characters are not paired (i.e uneven) then :term:`variable substitution` can not take place
           hence an error message is issued. 
      
           ::
      
            port=%ECF_PORT       # error issued since '%' micro character are not paired.
         
           However an uneven number of micro character are allowed, **If** the line begins with '#' comment charcter. 
      
           ::
      
            # This is a comment line with a single micro character % no error issued
            # port=%ECF_PORT        again no error issued    
      
      Directives are expanded during :term:`pre-processing`. Examples include:
      
      ====================== ============================================================================
      Symbol                                  Meaning
      ====================== ============================================================================
      %include <filename>    %ECF_INCLUDE% directory is searched for the :file:`filename` and the contents
                             included into the job file. If that variable is not defined ECF_HOME is used.
                             If the ECF_INCLUDE is defined but the file does not exist, then we look in
                             ECF_HOME. This allows specific files to be placed in ECF_INCLUDE and the 
                             more general/common include files to be placed in ECF_HOME.
                             This is the recommended format
      %include "filename"    Include the contents of the file:
                             %ECF_HOME%/%SUITE%/%FAMILY%/filename into the job.
      %include filename      Include the contents of the file :file:`filename` into the output. The only form
                             that can be used safely must start with a slash '/'
      %includenopp filename  Same as %include, but the file is not interpreted at all.
      %comment               Start's a comment, which is ended by %end directive.
                             The section enclosed by %comment - %end is removed during :term:`pre-processing`
      %manual                Start's a manual, which is ended by %end directive.
                             The section enclosed by %manual - %end is removed during :term:`pre-processing`.
                             The manual directive is used to create the :term:`manual page`
                             show in :term:`ecflowview`. 
      %nopp                  Stop pre-processing until a line starting with %end is found.
                             No interpretation of the text will be done( i.e. no variable substitutions)
      %end                   End processing of %comment or %manual or %nopp
      %ecfmicro CHAR         Change the directive character, to the character given. If set in an 
                             include file the effect is retained for the rest of the job( or until
                             set again). It should be noted that the ecfmicro directive specified in
                             the :term:`ecf script` file, does **not** effect the variable substitution
                             for ECF_JOB_CMD, ECF_KILL_CMD or ECF_STATUS_CMD variables. They still use
                             :term:`ECF_MICRO`. If no ecfmicro directive exists, we default to using
                             :term:`ECF_MICRO` from the :term:`suite definition`
      ====================== ============================================================================
              
   zombie
      Zombies are running jobs that fail authentication when communicating with the :term:`ecflow_server`
      
      :term:`child command` s like (init, event,meter, label, abort,complete) are placed in the :term:`ecf script`
      file and are used to communicate with the :term:`ecflow_server`. 
      
      The :term:`ecflow_server` authenticates each connection attempt made by the :term:`child command`. 
      Authentication can fail for a number of reasons:
      
         - password(ECF_PASS) supplied with the :term:`child command`, does not match the one in the :term:`ecflow_server`
         - path name(ECF_NAME) supplied with the :term:`child command`, does not locate a :term:`task` in the :term:`ecflow_server`
         - process id(ECF_RID) supplied with :term:`child command`, does not correspond with the one stored in the :term:`ecflow_server`
         - :term:`task` is already :term:`active`, but receives another init :term:`child command`
         - :term:`task` is already :term:`complete`, but receives another :term:`child command`
         
      When authentication fails the job is considered to be a zombie.
      The :term:`ecflow_server` will keep a note of the zombie for a period of time, before it is automatically removed.
      However the removed zombie, may well re-appear. ( this is because each :term:`child command` will continue
      attempting to contact the :term:`ecflow_server` for 24 hours. This is configurable 
      see ECF_TIMEOUT on :term:`ecflow_client`)
      
      For python see :py:class:`ecflow.ZombieAttr`, :py:class:`ecflow.ZombieUserActionType`
            
      There are several types of zombies see :term:`zombie type` and :py:class:`ecflow.ZombieType`  
     
   zombie attribute
      The zombie attribute defines how a :term:`zombie` should be handled in an automated fashion.
      Very careful consideration should be taken before this attribute is added as it may hide a genuine problem.
      It can be added to any :term:`node`. But is best defined at the :term:`suite` or :term:`family` level. 
      If there is no zombie attribute the default behaviour is to block the :term:`child command`. 
      
      To add a zombie attribute in python, please see: :py:class:`ecflow.ZombieAttr`
      
   zombie type
      See :term:`zombie` and class :py:class:`ecflow.ZombieAttr` for further information.
      There are several types of zombies:
      
         * path 
            - The task path can not be found in the server, because node tree was deleted, 
              replaced,reload, server crashed or backup server does not have node tree. 
            - Jobs could have been created, via server :term:`scheduling` or by :term:`user commands` 
            
         * user
            Job is created by :term:`user commands` like, rerun, re-queue. User zombies are
            differentiated from server(scheduled) since they are automatically created when
            the force option is used and we have tasks in an :term:`active` or :term:`submitted` states.
            
         * ecf
            Jobs are created as part of the normal :term:`scheduling`
            
            - Server crashed ( or terminated and restarted) and the recovered :term:`check point` file is out of date.
            - A :term:`task` is repeatedly re-run, earlier copies will not be remembered.
            - Job sent by another :term:`ecflow_server`, but which can not talk to the original :term:`ecflow_server`
            - Network glitches/network down
      
      The type of the zombie is not fixed and may change.
     
   user commands
      User commands are any client to server requests that are **not** :term:`child command` s.
      
   ECF_TRYNO
      This is a generated :term:`variable` that is used in file name generation.
      It represents the current try number for the :term:`task`.
      
      After **begin** it is set to 1. The number is advanced if the job is re-run.
      It is re-set back to 1 after a re-queue.
      It is used in output and :term:`job file` numbering. 
      (i.e It avoids overwriting the :term:`job file` output during multiple re-runs)
      
   ECF_MICRO
      This is a generated :term:`variable`. The default value is %.
      This variable is used in :term:`variable substitution` during command invocation and 
      default directive character during :term:`pre-processing`. 
      It can be overriden, but must be replaced by a single character.
      
   ECF_NAME
      This is a generated :term:`variable`. It defines the path name of the task.
      
   ECF_SCRIPT
       This is a generated :term:`variable`. If defines the path name for the :term:`ecf script`
       
   ECF_JOB
       This is a generated :term:`variable`. If defines the path name location of the job file.
       
       The variable is composed as: ECF_HOME/ECF_NAME.job<ECF_TRYNO>
       
   ECF_JOBOUT
       This is a generated :term:`variable`. This variable defines the path name
       for the job output file. The variable is composed as:
      
         ECF_HOME/ECF_NAME.ECF_TRYNO
       
   ECF_DUMMY_TASK
      This is a user variable that can be added to :term:`task` to indicate that there is no
      associated :term:`ecf script` file. 
      
      If this variable is added to :term:`suite` or :term:`family` then all child tasks are treated as dummy.
      
      This stops the server from reporting an error during :term:`job creation`.
      