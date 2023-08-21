
.. index::
   single: Glossary
   
.. _glossary:
   
============
**Glossary**
============

.. glossary::
   :sorted:

   aborted
      Is a :term:`node` :term:`status`. 
      
      When the :term:`ECF_JOB_CMD` fails or the :term:`job file` sends a :term:`ecflow_client` --abort :term:`child command`, then
      the task is placed into a aborted state.
      
   active
      Is a :term:`node` :term:`status`. 
      
      If :term:`job creation` was successful, and :term:`job file` has started, then the :term:`ecflow_client` --init
      :term:`child command` is received by the :term:`ecflow_server` and the :term:`task` is placed into a active state
      
   autocancel
      autocancel is a way to automatically delete a :term:`node` which has completed.
      
      The delete may be delayed by an amount of time in hours and minutes or 
      expressed in days. Any node may have a single autocancel attribute. 
      If the auto cancelled node is referenced in the :term:`trigger` expression of other nodes
      it may leave the node waiting. This can be solved by making sure the :term:`trigger`
      expression also checks for the :term:`unknown` state. i.e.:

      .. code-block:: shell
      
         trigger node_to_cancel == complete or node_to_cancel == unknown
      
      This guards against the 'node_to_cancel' being undefined or deleted
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Autocancel`, :py:class:`ecflow.Node.add_autocancel`
         * - :ref:`grammar`
           - :token:`autocancel`

   check point 
      The check point file is like the :term:`suite definition`, but includes all the state information.
      
      It is periodically saved by the :term:`ecflow_server`. 
      
      It can be used to recover the state of the node tree should server die, or machine crash.
      
      By default when a :term:`ecflow_server` is started it will look to load the check point file. 
      
      The default check point file name is <host>.<port>.ecf.check. This can be overridden by the ECF_CHECK environment variable

      The check point file format is the same as the defs file format (from release 4.7.0 onwards). However, the indentation has been removed to preserve space. To view with indentation use:

      .. code-block:: shell

         ecflow_client --load=<check_point_file> print check_only
      
   child command
      Child commands (or task requests) are called from within the :term:`ecf script` files. The table also includes the default action (from version 4.0.4) if the child command is part of a zombie. 'block' means the job will be held by the :term:`ecflow_client` command. Until time out, or manual/automatic intervention.

      .. list-table:: 
         :header-rows: 1
         
         * - Child Command 
           - Description
           - Zombie (default action)
         * - :ref:`ecflow_client --init <init_cli>`
           - Sets the :term:`task` to the :term:`active` :term:`status`
           - block
         * - :ref:`ecflow_client --wait <wait_cli>`
           - Wait for a expression to evaluate
           - block
         * - :ref:`ecflow_client --queue <queue_cli>`
           - Update :term:`queue` step in server
           - block
         * - :ref:`ecflow_client --abort <abort_cli>`
           - Sets the :term:`task` to the :term:`aborted` :term:`status`
           - block
         * - :ref:`ecflow_client --complete <complete_cli>`
           - Sets the :term:`task` to the :term:`complete` :term:`status`
           - block 
         * - :ref:`ecflow_client --event <event_cli>`
           - Set an :term:`event`
           - fob
         * - :ref:`ecflow_client --meter <meter_cli>`
           - Change a :term:`meter`
           - fob
         * - :ref:`ecflow_client --label <label_cli>`
           - Change a :term:`label`
           - fob

      The following environment variables must be set for the child commands. ECF_HOST, :term:`ECF_NAME` ,:term:`ECF_PASS` and ECF_RID. See :term:`ecflow_client`.                                         
       
      
   clock
      A clock is an attribute of a :term:`suite`. 
      
      A gain can be specified to offset from the given date.
      
      The hybrid and real clocks always runs in phase with the system clock (UTC in UNIX) 
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
      In particular it will affect :term:`task`\ s with :term:`time`, :term:`today` or :term:`cron` :term:`dependencies`.
          
         - :term:`dependencies` with time series, can result in missed time slots:

           .. code-block:: shell
         
               time 10:00 20:00 00:15    # If server is suspended > 15 minutes, time slots can be missed            
               time +00:05 20:00 00:15   # start 5 minutes after the start of the suite, then every 15m until 20:00
         
         - When the server is placed back into :term:`running` state any time :term:`dependencies`
           with an expired time slot are submitted straight away. i.e if :term:`ecflow_server` is
           :term:`halted` at 10:59 and then placed back into :term:`running` state at 11:20:
           
           .. code-block:: shell
         
               time 11:00
           
           Then any :term:`task` with a expired single time slot dependency will be submitted straight away.

      See also:

      .. list-table::
         :widths: 40 60

         * - :ref:`python_api`
           - :py:class:`ecflow.Clock`, :py:class:`ecflow.Suite.add_clock`
         * - :ref:`grammar`
           - :token:`clock`

   complete 
      Is a :term:`node` :term:`status`.
      
      The node can be set to complete:

      - By the :term:`complete expression`
      - At job end when the :term:`task` receives the :ref:`ecflow_client –complete <complete_cli>` :term:`child command`
      - Manually via the command line or GUI. When this happens any time attributes are expired in order.

  
   complete expression
      Force a node to be complete **if** the expression evaluates, without running any of the nodes. 
      
      This allows you to have tasks in the suite which a run only if others fail. 
      In practice the node would need to have a :term:`trigger` also. 
      
      .. list-table::
         :widths: 40 60

         * - :ref:`ecflow_cli`
           - :ref:`--complete <complete_cli>` 
         * - :ref:`python_api`
           - :py:class:`ecflow.Expression`, :py:class:`ecflow.Node.add_complete`
         * - :ref:`grammar`
           - :token:`complete`
      
   cron
      Like :term:`time`, cron defines time dependency for a :term:`node`, but it will be repeated indefinitely:

      .. code-block:: shell

         cron -w <weekdays> -d <days> -m <months> <start_time> <end_time> <increment>
         # weekdays:   range [0...6], Sunday=0, Monday=1, etc    e.g. -w, 0,3,6
         # days:       range [1..31]                             e.g. -d 1,2,20,30    if the month does not have a day, i.e. February 21st it is ignored
         # months:     range [1..12]                             e.g. -m 5,6,7,8
         # start_time: The starting time. format hh:mm           e.g. 15:21
         # end_time:   The end time, if multiple times used
         # increment:  The increment in time if multiple times are given
         
         -w day of the week   valid values are , 0 → 6 where 0 is Sunday , 1 is Monday etc AND
                              0L→6L, where 0L means last Sunday of the month, and 1L means the last Monday of the month, etc
                              It is an error to overlay, i.e. cron -w 0,1,2,1L,2L,3L   23:00  will throw an exception
         -d day of the month   valid values are in range 0-31,L   Extended so that we now use 'L' to mean the last day of the month
         -m month              valid values are in range 0-12
         
         cron 11:00                           # single time
         cron 10:00 22:00 00:30               # <start> <finish> <increment>
         cron +00:20 23:59 00:30              # relative to suite start time, or when re-queued  as part of a repeat loop. Note: maximum relative time is 24 hours
         cron -w 0,1 10:00 11:00 01:00        # run every Sunday & Monday at 10 and 11 am
         cron -d 15,16 -m 1 10:00 11:00 01:00 # run 15,16 January at 10 and 11 am
         cron -w 5L 23:00                     # run on *last* Friday(5L) of each month at 23pm,
                                             # Python: cron = Cron("23:00",last_week_days_of_the_month=[5])
         cron -w 0,1L 23:00                   # run every Sunday(0) and *last* Monday(1L) of the month at 23pm
                                             # Python: cron = Cron("23:00",days_of_week=[0],last_week_days_of_the_month=[1])
         cron -w 0L,1L,2L,3L,4L,5L,6L 10:00   # run on the last Monday,Tuesday..Saturday,Sunday of the month at 10 am
                                             # Python: cron = Cron("10:00",last_week_days_of_the_month=[0,1,2,3,4,5,6])
         cron -d 1,L  23:00                   # Run on the first and last of the month at 23pm
                                             # Python: cron = Cron("23:00",days_of_week=[1],last_day_of_the_month=True)

   
      When the node becomes complete it will be :term:`queued` immediately. This means that the suite will never complete, and the output is not directly accessible through :term:`ecflow_ui`
      
      If tasks abort, the :term:`ecflow_server` will not schedule it again.
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g.:

      .. code-block:: shell
      
         cron 10:00 20:00 01:00 
         
      if the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      If the cron defines months, days of the month, or week days or a single time slot
      the it relies on a day change, hence if a :term:`hybrid clock` is defined, 
      then it will be set to :term:`complete` at  the beginning of the :term:`suite`, 
      without running  the corresponding job. 
      Otherwise under a hybrid clock the :term:`suite` would never :term:`complete`.
      
      See also:

      .. list-table::
         :widths: 40 60

         * - :ref:`python_api`
           - :py:class:`ecflow.Cron`, :py:class:`ecflow.Node.add_cron` 
         * - :ref:`grammar`
           - :token:`cron`

   date
      This defines a date dependency for a node. 
      
      There can be multiple date dependencies. 
      The European format is used for dates, which is: dd.mm.yy as in 31.12.2007. 
      Any of the three number fields can be expressed with a wildcard `*` to mean any valid value. 
      Thus, 01.*.* means the first day of every month of every year.

      If a :term:`hybrid clock` is defined, any node held by a date dependency will be set to :term:`complete` at the beginning
      of the :term:`suite`, without running the corresponding job. Otherwise under a hybrid clock the :term:`suite` would
      never :term:`complete`.
      
      .. list-table::
         :widths: 40 60

         * - :ref:`python_api`
           - :py:class:`ecflow.Date`, :py:class:`ecflow.Node.add_date`
         * - :ref:`grammar`
           - :token:`date`

   day
      This defines a day dependency for a  node.
      
      There can be multiple day dependencies.
      
      If a :term:`hybrid clock` is defined, any node held by a day dependency will be set to :term:`complete` at the beginning
      of the :term:`suite`, without running the corresponding job. Otherwise under a hybrid clock the :term:`suite` would
      never :term:`complete`.
      
      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Day`, :py:class:`ecflow.Node.add_day`
         * - :ref:`grammar`
           - :token:`day`
       
   defstatus
      Defines the default :term:`status` for a task/family to be assigned to the :term:`node` when the begin command is issued.
      
      By default :term:`node` gets queued when you use begin on a :term:`suite`. 
      defstatus is useful in preventing suites from running automatically once begun or in setting
      tasks complete so they can be run selectively. 

      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.DState`, :py:class:`ecflow.Node.add_defstatus`
         * - :ref:`grammar`
           - :token:`defstatus`
       
   dependencies
      Dependencies are attributes of node, that can suppress/hold a :term:`task` from taking part in :term:`job creation`.
      
      They include :term:`trigger`, :term:`date`, :term:`day`, :term:`time`, :term:`today`, :term:`cron`, :term:`complete expression`, :term:`inlimit` and  :term:`limit`. 
      
      A :term:`task` that is dependent cannot be started as long as some dependency is holding it or any of its **parent** :term:`node` s.
      
      The :term:`ecflow_server` will check the dependencies every minute, during normal :term:`scheduling` **and** when any
      :term:`child command` causes a state change in the :term:`suite definition`.
      
   directives
      Directives appear in a ecf script. (i.e. typically .ecf file, but could be .py file).Directives start with a % character. This is referred to as :term:`ECF_MICRO` character.
      
      The directives are used in two main context.
      
         - Preprocessing directives. In this case the directive starts as the **first** character on a line in a :term:`ecf script` file. 
           See the table below which shows the allowable values. Only one directive is allowed on the line.
           
         - Variable directives. We use two :term:`ECF_MICRO` characters ie %VAR%, in this case they can occur **anywhere** on 
           the line and in any number.  
           
           .. code-block:: shell
           
               %CAR% %TYPE% %WISHLIST% 
            
           These directives take part in :term:`variable substitution`.
           
           If the micro characters are not paired (i.e uneven) then :term:`variable substitution` cannot take place
           hence an error message is issued. 
      
           .. code-block:: shell
      
               port=%ECF_PORT       # error issued since '%' micro character are not paired.
         
           However an uneven number of micro character are allowed, **If** the line begins with '#' comment character.
      
           .. code-block:: shell
      
               # This is a comment line with a single micro character % no error issued
               # port=%ECF_PORT        again no error issued    
      
      Directives are expanded during :term:`pre-processing`. Examples include:
      
      ====================== ============================================================================
      Symbol                                  Meaning
      ====================== ============================================================================
      %include <filename>    %ECF_INCLUDE% directory is searched for the :file:`filename` and the contents
                             included into the job file. If that variable is not defined :term:`ECF_HOME` is used.
                             If the :term:`ECF_INCLUDE` is defined but the file does not exist, then we look in
                             :term:`ECF_HOME`. This allows specific files to be placed in ECF_INCLUDE and the 
                             more general/common include files to be placed in ECF_HOME.
                             This is the recommended format
      %include "filename"    Include the contents of the file:
                             %ECF_HOME%/%SUITE%/%FAMILY%/filename into the job.
      %include filename      Include the contents of the file :file:`filename` into the output. The only form
                             that can be used safely must start with a slash '/'
      %includenopp filename  Same as %include, but the file is not interpreted at all.
      %comment               Starts a comment, which is ended by %end directive.
                             The section enclosed by %comment - %end is removed during :term:`pre-processing`
      %manual                Starts a manual, which is ended by %end directive.
                             The section enclosed by %manual - %end is removed during :term:`pre-processing`.
                             The manual directive is used to create the :term:`manual page`
                             show in :term:`ecflow_ui`. 
      %nopp                  Stop pre-processing until a line starting with %end is found.
                             No interpretation of the text will be done (i.e. no variable substitutions)
      %end                   End processing of %comment or %manual or %nopp
      %ecfmicro CHAR         Change the directive character, to the character given. If set in an 
                             include file the effect is retained for the rest of the job (or until
                             set again). It should be noted that the ecfmicro directive specified in
                             the :term:`ecf script` file, does **not** effect the variable substitution
                             for :term:`ECF_JOB_CMD`, ECF_KILL_CMD or :term:`ECF_STATUS_CMD` variables. They still use
                             :term:`ECF_MICRO`. If no ecfmicro directive exists, we default to using
                             :term:`ECF_MICRO` from the :term:`suite definition`
      ====================== ============================================================================

      From ecFlow release 4.4.0, use of %VAR% (variable substitution) can be a part of the include filename. i.e.:

      .. code-block:: shell

         # %file% must be defined, on the task, or on the parent hierarchy
         %include <%file%.h>

         # use %INCLUDEFILE% if defined (on the task, or on the parent hierarchy,
         # and MUST follow one of formats above: ".filename", "../filename", "filename",
         # filename>)  otherwise use <file>
         %include %INCLUDEFILE:<file>%  

      Care should be taken to avoid spaces in the variable values.       

   ecf file location algorithm
     :term:`ecflow_server` and job creation checking uses the following algorithm to locate the '.ecf' file corresponding to a :term:`task`.

     .. note::

         To search for files with a different extension, i.e. to look for python file '.py'. Override the :term:`ECF_EXTN` variable. Its default value is '.ecf'
     
     * :term:`ECF_SCRIPT`: First it uses the generated variable ECF_SCRIPT to locate the script. 
       This variable is generated from: ECF_HOME/<path to task>.ecf
       Hence if the task path is /suite/f1/f2/t1, then ECF_SCRIPT=ECF_HOME/suite/f1/f2/t1.ecf
        
     * :term:`ECF_FETCH` (user variable): File is obtained from running the command after some postfix arguments are added. (Output of popen)

     * :term:`ECF_SCRIPT_CMD` (user variable): File is obtained from running the command. (Output of popen)
     
     * **ECF_FILES**: Second it checks for the user defined ECF_FILES variable. If defined the value of this variable must correspond to a directory. This directory is searched in reverse order.
      
      I.e. lets assume we have a :term:`task` /o/12/fc/model and ECF_FILES is defined as /home/ecmwf/emos/def/o/ECFfiles
        
      The ecFlow will use the following search pattern.
        
           #. /home/ecmwf/emos/def/o/ECFfiles/o/12/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/12/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/fc/model.ecf
           #. /home/ecmwf/emos/def/o/ECFfiles/model.ecf

      If the directory does not exist, the server will try variable substitution.  This allows additional configuration:

      .. code-block:: shell

            edit ECF_FILES /home/ecmwf/emos/def/o/%FILE_DIR:ECFfiles%
       
      The search can be reversed, by adding a variable **ECF_FILES_LOOKUP**, with a value of "prune_leaf" (from ecFlow 4.12.0). Then ecFlow will use the following search pattern.

         #. /home/ecmwf/emos/def/o/ECFfiles/o/12/fc/model.ecf
         #. /home/ecmwf/emos/def/o/ECFfiles/o/12/model.ecf
         #. /home/ecmwf/emos/def/o/ECFfiles/o/model.ecf
         #. /home/ecmwf/emos/def/o/ECFfiles/model.ecf
        
      However please be aware this will also affect the search in :term:`ECF_HOME`

     * :term:`ECF_HOME`: Thirdly it searches for the script in reverse order using :term:`ECF_HOME` (i.e like ECF_FILES). If this fails, than the :term:`task` is placed into the :term:`aborted` state. We can check that file can be located before loading the suites into the server.
      
     Note: The addition of variable with a name **ECF_FILES_LOOKUP** and value 'prune_leaf', affects the search in BOTH **ECF_FILES** and :term:`ECF_HOME`

     See also:

         * :ref:`tutorial-checking-job-creation`
         * :py:class:`ecflow.Defs.check_job_creation`    
   
   ecf script
      The ecFlow script refers to an ‘.ecf’ file.  
      
      The script file is transformed into the :term:`job file` by the :term:`job creation` process.
      
      The base name of the script file **must** match its corresponding :term:`task`. i.e t1.ecf , corresponds to the task of name 't1'.
      The script if placed in the ECF_FILES directory, may be re-used by multiple tasks belonging to different families,
      providing the :term:`task` name matches.
      
      The ecFlow script is similar to a UNIX shell script.  
      
      The differences, however, includes the addition of “C” like pre-processing :term:`directives` and ecFlow :term:`variable`\ s.
      Also the script *must* include calls to the **init** and **complete** :term:`child command`\ s so that
      the :term:`ecflow_server` is aware when the job starts (i.e changes state to :term:`active`) and finishes (i.e changes state to :term:`complete`)
       
   ECF_DUMMY_TASK
      This is a user variable that can be added to :term:`task` to indicate that there is no
      associated :term:`ecf script` file. 
      
      If this variable is added to :term:`suite` or :term:`family` then all child tasks are treated as dummy.
      
      This stops the server from reporting an error during :term:`job creation`.
      
   ECF_EXTN
      Defines the extension for the script that will be turned into a job file. This has a default value of '.ecf'. But could be any extension.This is used by the server as part of 'ecf file location algorithm'

   ECF_FETCH
      *Experimental*
      This is used to specify a command, whose output can be used as a job script. The ecFlow server will run the command with popen. Hence great care needs to be taken not to doom the server, with command that can hang. As this could severely affect servers ability to schedule jobs.

      .. code-block:: shell

         edit ECF_FETCH my_custom_cmd.sh
      
      After variable substitution, the server will add the following.

      .. code-block:: shell
      
         my_custom_cmd.sh -s <task_name>.<ECF_EXTN>   # to extract the script and create the job
         my_custom_cmd.sh -i                          # to extract the includes
         my_custom_cmd.sh -m <task_name>.<ECF_EXTN>   # to extract the manual, i.e. for display in the info tab
         my_custom_cmd.sh -c <task_name>.<ECF_EXTN>   # to extract the comments

      The output of running these commands (-s) is used to create the job.

   ECF_HOME
      This is user defined :term:`variable`; it has four functions:
      
      - it is used as a prefix portion of the path of the job files created by ecFlow server; see the description of the :term:`ECF_JOB` generated variable.
      - it is a default directory where ecFlow server looks for scripts (with file extension defined by :term:`ECF_EXTN`,default is .ecf); overridden by ECF_FILES user defined variable. See the "ecf file location algorithm" entry for more detail.
      - it is a default directory where ecFlow server looks for include files; overridden by :term:`ECF_INCLUDE` user defined variable. See the "directives" entry for more detail.
      - it is used as a default prefix portion of the job output path (the :term:`ECF_JOBOUT` generated variable); overridden by **ECF_OUT** user defined variable. See descriptions of :term:`ECF_JOBOUT` and :term:`ECF_OUT` variables for more detail.

   ECF_INCLUDE
      This is a user defined variable. It is used to specify directory locations, that are used to search for include files.

      .. code-block:: shell
                  
         edit ECF_INCLUDE /home/fred/course/include           # a single directory
         edit ECF_INCLUDE /home/fred/course/include:/home/fred/course/include2:/home/fred/course/include_me  # set of directories to search
   
   ECF_JOB
       This is a generated :term:`variable`. If defines the path name location of the job file.
       
       The variable is composed as::
         
         ECF_HOME/ECF_NAME.job<ECF_TRYNO>
       
   ECF_JOB_CMD
      This :term:`variable` should point to a script that can submit the job. (i.e. to the queuing system, via, SLURM,PBS). 
      
      The ecFlow server will detect abnormal termination of this command. Hence for errors in the job file, should call 'ecflow_client --abort", then exits cleanly.
      Otherwise server detects abnormal job termination, and abort flag is set. Which will prevent job re-queue(due to ECF_TRIES). 
      
      If the job also sends an abort, zombies can be created. If ECF_JOB_CMD command fails, and the task is in a submitted state, then the task is set to the aborted state. However if the task was active or complete, then we do NOT abort the task. Instead the zombie flag is set. (since ecFlow 4.17.1)

   ECF_JOBOUT
      This is a generated :term:`variable`. This variable defines the path name for the job output file. The variable is composed as following. 

      If :term:`ECF_OUT` is specified::

         ECF_OUT/ECF_NAME.ECF_TRYNO
      
      otherwise::

         ECF_HOME/ECF_NAME.ECF_TRYNO
       
   ECF_LISTS
      This is the server variable. The variable specifies the path to the White list file. This file controls who has read/write access to the server via the :term:`user command`\ s.

      The user name can be found using linux, id command and is typically the login name. The file has a very simple format.

      The file path specified by ECF_LISTS environment, is read by the server on start up. The contents of the white list can be modified, and reloaded by the server. (However the path to the white-list file can NOT be modified after the server has started).

      If ECF_LISTS is not set, the server will look for a file named <host>.<port>.ecf.lists (i.e.my_host.3141.ecf.lists) in same directory where the server was started.

      If the file specified by ECF_LISTS or <host>.<port>.ecf.lists, does not exist or exists but is empty, then all users will have read/write access to suites on the server. Special care must be taken, so that user reloading the white list file does not remove write access for the administrator.

      .. code-block:: shell 
         :caption: Re-load white list file

          ecflow_client --help=reloadwsfile
          ecflow_client --reloadwsfile


      .. code-block:: shell 
         :caption: Read write access for specific users

          4.4.14   # this is a comment, the first non-comment line must include a version.

          # These users have read and write access to the server
          uid1  # user uid1,uid2,cog have read and write access to the server
          uid2  
          cog  
          
          # Read only users
          -fred  # users fred,bill and jake have read only access
          -bill  
          -jake


      .. code-block:: shell 
         :caption: Example where all users have read access

          4.4.14   # this is a comment, the first non-comment line must include a version.
          
          # These users have read and write access to the server
          uid1  # user uid1,uid2,cog have read and write access to the server
          uid2  
          cog  
          
          # User with read access
          -*    # all users have read access

      .. code-block:: shell 
         :caption: From ecFlow release 4.1.0, users can be restricted via node paths

          4.4.5
          fred             # has read /write access to all suites
          -joe             # has read access to all suites
          
          *  /x /y    # all users have read/write access to suites /x /y
          -* /w /z    # all users have read access to suites /w /z
          
          user1 /a,/b,/c  # user1 has read/write access to suite /a /b /c
          user2 /a
          user2 /b
          user2 /c       # user2 has read write access to suite /a /b /c
          user3 /a /b /c # user3 has read write access to suite /a /b /c
          
          -user4 /a,/b,/c  # user4 has read access to suite /a /b /c
          -user5 /a
          -user5 /b
          -user5 /c    # user5 has read access to suite /a /b /c
          -user6 /a /b /c   # user6 has read access to suite /a /b /c

   ECF_MICRO
      This is a generated :term:`variable`. The default value is %.
      This variable is used in :term:`variable substitution` during command invocation and 
      default directive character during :term:`pre-processing`. 
      It can be overriden, but must be replaced by a single character.
      
   ECF_NAME
      This is a generated :term:`variable`. It defines the path name of the :term:`task`. It will typically be used inside script file, referring to the corresponding task. 

      .. code-block:: shell
         :caption: t1.ecf

          %include <head.h>
          ....
          ecflow_client --alter change variable "fred" "bill" %ECF_NAME% # change variable on corresponding task
          ...
          %include <tail.h>
      
      
   ECF_NO_SCRIPT
      This is a user :term:`variable`, that can be added to a :term:`node` (introduced with ecFlow release 4.3.0). It is used to inform the ecflow_server that there is **no SCRIPT** associated with a task. However unlike ECF_DUMMY_TASK, the task can still be submitted provided the :term:`ECF_JOB_CMD` is set up.

      This is suitable for very **lightweight** tasks that want to minimize latency. The output can still be seen, if it is redirected  to :term:`ECF_JOBOUT`. Care must be taken to ensure the path  to ecflow_client is accessible.

      .. code-block:: shell
         :caption: ECF_NO_SCRIPT examples

         family no_script
         edit ECF_NO_SCRIPT "1"  # the server will not look for .ecf files
         edit ECFLOW_CLIENT ecflow_client
         edit DIROUT %VERBOSE%
         edit SILENT ""
         edit VERBOSE " > %ECF_JOBOUT 2>&1"
         
         task non_script_task
            edit ECF_JOB_CMD "export ECF_PASS=%ECF_PASS%;export ECF_PORT=%ECF_PORT%;export ECF_HOST=%ECF_HOST%;export ECF_NAME=%ECF_NAME%;export ECF_TRYNO=%ECF_TRYNO%; %ECF_CLIENT% --init=$$; echo 'test test_ecf_no_script' %DIROUT% && %ECF_CLIENT% --complete"
            # this command is not expected to fail. hence no error handling.(i.e.. will stay active)
         
         task ecf_no_script
         edit ECF_JOB_CMD "ecf_no_script --pass %ECF_PASS% --host %ECF_HOST% --port %ECF_PORT% " # %DIROUT%
         # ecf_no_script contains init, complete, call to ecflow_client and trapping to raise abort
         # use this approach for robust error handling
         
         task ymd2jul
         edit ECF_JOB_CMD "ECF_PASS=%ECF_PASS% ECF_NAME=%ECF_NAME% /usr/local/bin/ymd2jul.sh -p %ECF_PORT% -n %ECF_HOST% -r /%SUITE%/%FAMILY% -y %YMD% > %ECF_JOBOUT% 2>&1 &"
         # /usr/local/bin/ymd2jul.sh can be called on command line or as ecflow_client
         endfamily

   ECF_OUT
      This is user/suite variable that specifies a directory PATH. It controls the location of job output (stdout and stderr of the process) on a remote file system. It provides an alternate location for the job and cmd output files. If it exists, it is used as a base for :term:`ECF_JOBOUT`, but it is also used to search for the output by ecFlow, when asked by :term:`ecflow_ui`/:term:`ecflow_client`. If the output is in ECF_OUT/ECF_NAME.ECF_TRYNO  it is returned, otherwise ECF_HOME/ECF_NAME.ECF_TRYNO is used.

      The user must ensure that all the directories exists, including :term:`suite`/:term:`family`. If this is not done, you may well find task remains stuck in a submitted state. At ECMWF our submission scripts will ensure that directories exists.

   ECF_PASS
      This is a generated :term:`variable`. During job generation process in the server, a unique password is generated and stored in the task. It then replaces %ECF_PASS% in the scripts(.ecf), with the actual value. When the job runs, ecflow_client reads this, as an environment variable, and passes it to the server. The server then compares this password with the one held on the task. This is used as a part of the authentication for child commands, and is used to detect zombies.

      The authentication process can be  bypassed, and allow the job to proceed (i.e.. when the user is sure that there is only a single process, trying to communicate with the server), by adding it as a user variable. i.e.:

      .. code-block:: shell

         ecflow_client --alter add variable ECF_PASS FREE  <path to task>

      This functionality is also available in the GUI. Select a task,  RMB > Special >Free password. However it is important not leave this in place, as it will always bypass the authentication. Just delete the variable.

   ECF_PASSWD
      This is an environment variable, which points to a password file for both client and server. This enables password based authentication for ecFlow :term:`user command`\ s. The password file is required for the client and server.

      .. code-block:: shell
         :caption: Example client password file. The same file can be used for multiple servers

         4.5.0
         # <user> <host> <port> <passwd>
         user1 machine1 3141 xxxty
         user1 machine2 3142 shhert

      .. code-block:: shell
         :caption: Example server password file for machine1 and port 3141

         4.5.0
         user1 machine1 3141 xxxty
         user2 machine1 3141 bbsdd7

      The server administrator needs to set Unix file permissions, so that this file is only readable by ecFlow server and the administrator.

   ECF_SCRIPT
       This is a generated :term:`variable`. If defines the path name for the :term:`ecf script`
       
   ECF_SCRIPT_CMD
      *Experimental*

      This allows the output of running a command to be treated as a script. The command is run after variable substitution. The output is obtained from running the system function popen in the server. Great care should be taken when running this command, to ensure errors in the command do not crash the server. This approach could be used for short lived tasks, where extremely low latency is required. Commands that take more than 20s can interfere with job scheduling and should be avoided. Could possibly be used to checkout a script from a version control system.
   
      If the output contains %include,%manual,%noop they are treated in the same manner as a normal '.ecf' script. 

      .. code-block:: shell
         :caption: Here the output of the 'cat' command is treated as a script

         suite test
            family family
               task check
                  edit ECF_SCRIPT_CMD "cat /tmp/ECF_SCRIPT_CMD/family/check.ecf"
               task t1
                  trigger check == complete
                  edit ECF_SCRIPT_CMD "cat /tmp/ECF_SCRIPT_CMD/family/t1.ecf"
            endfamily
         endsuite
   
   ECF_STATUS_CMD
      User defined :term:`variable` defining the :ref:`ecflow_client --status <status_cli>` command. It invokes a user-supplied (shell) command that queries the status of the job.
      
      The command should be written in such a way that the output is written to %ECF_JOB%.stat, and if the script determines that the job is not active, it should abort the task in ecflow. This command can be particularly useful when nodes on the supercomputer go down, and we don't know the true state of the jobs.

      The status command can be invoked from the :ref:`ecflow_cli` and :ref:`ecflow_ui`. If applied to a :term:`family` or :term:`suite`, the command will be run hierarchically. In :ref:`ecflow_ui` use the Status tab in the Info panel or use Special > Status from the node context menu to run it and see the output.

      The code below allows the output of the status command to be shown by the ``--file`` command on the command line, and automatically via the Status tab in :ref:`ecflow_ui`:

      .. code-block:: shell

         suite s1
            edit ECF_STATUS_CMD /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1
         ....
         endsuite      

      .. code-block:: shell
         :caption: Invoking status cmd, from the command line

         ecflow_client --status=/s1/f1/t1     # ECF_STATUS_CMD should output to %ECF_JOB%.stat
         ecflow_client --file=/s1/f1/t1 stat  # Return contents of %ECF_JOB%.stat file" 

   ECF_TRIES
      This is generated variable added at the server level with a default value of 2.  It can be overridden by the user and controls the number of times job should re-run should it abort. Provided:

      - the task/job has NOT been killed(user action) 
      - the job process (created from .ecf or .py) exited cleanly and not with exit 1 || sys.exit(1) as process death is captured by the server. Always ensure your script exits cleanly. i.e. exit(0)
      - the task has NOT been set to abort by the user(user action)
      - job creation has not failed . i.e. task pre-processing(include file expansion,variable - substitution, change of file permission for job file)
      - the value of the variable ECF_TRIES must be convertible to an integer.
      
      Please note this allows your scripts to be self-aware of the number times it is being run. i.e.:

       .. code-block:: shell
         :caption: task.ecf
         
          %include <head.h>
          "echo do some work\n";
          if [ %ECF_TRYNO% -eq 1 ] ; then
             echo "first attempt"
             .....
          fi
          if [ %ECF_TRYNO% -eq 2 ] ; then
             echo "first attempt failed, trying a different approach, clean data, etc"
             .....
          fi
          %include <tail.h>
   
   ECF_TRYNO
      This is a generated :term:`variable` that is used in file name generation.
      It represents the current try number for the :term:`task`.
      
      After **begin** it is set to 1. The number is advanced if the job is re-run.
      It is re-set back to 1 after a re-queue.
      It is used in output and :term:`job file` numbering. 
      (i.e It avoids overwriting the :term:`job file` output during multiple re-runs)
      
   ecFlow
      Is the ECMWF work flow manager.
      
      A general purpose application designed to schedule a large number of
      computer process in a heterogeneous environment.
      
      Helps computer jobs design, submission and monitoring both in
      the research and operation departments.

   ecflow_client
      This executable provides the ecFlow :ref:`ecflow_cli`; it is used for all communication with the :term:`ecflow_server`.
      
      To see the full range of commands that can be sent to the :term:`ecflow_server` type the following in a UNIX shell:
      
      .. code-block:: shell

         ecflow_client --help
         
      This functionality is also provided by the :ref:`python_api`.
      
      The following variables affect the execution of ecflow_client. 
      
      Since the :term:`ecf script` can call ecflow_client(i.e :term:`child command`) then typically
      some are set in an include header. i.e. :ref:`tutorial-head_h`.
      
      .. list-table:: Environment variables common for user and child commands
         :header-rows: 1
         :widths: 10 50 10 30

         * - Variable Name
           - Explanation
           - Compulsory
           - Example
         * - ECF_PORT
           - Port number of the :term:`ecflow_server`. Must match :term:`ecflow_server`
           - Yes/No
           - We can use:
            
             .. code-block:: shell
            
                ecflow_client --port 3141

             as an alternative to specifying the ECF_PORT.
         * - ECF_HOST
           - Name of the host running the :term:`ecflow_server` 
           - Yes/No
           - We can use:
            
             .. code-block:: shell 
            
               ecflow --host machine1

             as an alternative to specifying ECF_HOST
         * - NO_ECF
           - If set exits ecflow_client immediately with success. This allows the scripts to be tested independent of the server
           - No
           - .. code-block:: shell
            
               export NO_ECF=1

         * - ECF_DENIED
           - If server denies client communication and this flag is set, exit with an error. Avoids 24hr hour connection attempt to :term:`ecflow_server`.
           - No
           - .. code-block:: shell
             
               export ECF_DENIED=1

         * - ECF_SSL
           - For secure socket communication with server. Requires client/server built with openssl libs.
           - No
           - .. code-block:: shell
              
               # Use same certificate for multiple server
               export ECF_SSL=1
               # Use server specific certificates
               export ECF_SSL=<host>.<port>

             Alternatively to avoid setting environmental variables we can use :code:`ecflow_client --ssl ...`.
             
             The client will first look for:  $HOME/.ecflowrc/ssl/server.crt then $HOME/.ecflowrc/ssl/<host>.<port>.crt 
      

      .. list-table:: Environment variables for child commands
         :header-rows: 1
         :widths: 10 50 10 30

         * - Variable Name
           - Explanation
           - Compulsory
           - Example
         * - :term:`ECF_NAME`
           - Path to the task
           - Yes
           - /suite/family/task
         * - :term:`ECF_PASS`
           - Jobs password. Generated by the server, will replace %ECF_PASS% in the scripts,during job generation.Used for authenticating child commands.
           - Yes
           - (generated)
         * - ECF_RID
           - Remote id. Allow easier job kill, and disambiguate a zombie
           - Yes
           - (generated)
         * - :term:`ECF_TRYNO`
           - The number of times the job has run. This is allocated by the server and used in job/output file name generation.
           - No 
           - (generated)
         * - ECF_HOSTFILE
           - File that lists alternate hosts to try, if connection to main host fails
           - No
           - $HOME/.echostfile
         * - ECF_TIMEOUT
           - Maximum time is seconds for the client to deliver message
           - No
           - 24*3600 (default value):

             .. code-block:: shell

               export ECF_TIMEOUT=36024*3600   

         * - ECF_ZOMBIE_TIMEOUT
           - Maximum time in seconds for the child(init, abort, complete, etc) zombie client to get a reply from the server. 
           - No
           - 12*3600 (default value):

             .. code-block:: shell
             
               export ECF_ZOMBIE_TIMEOUT=36024*3600
         
      .. list-table:: Variables specific to user commands
         :header-rows: 1
         :widths: 10 50 10 30

         * - Variable Name
           - Explanation
           - Compulsory
           - Example
         * - :term:`ECF_PASSWD`
           - path to the client password file, used for password based authentication
           - No
           - .. code-block:: shell
  
               export ECF_PASSWD=mymachine.3141.ecf.passwd

         * - ECF_USER
           - When user need to pose as another user, i.e. when users id on the client machine, doesn't  match his id on the remote server. Requires password file.
           - No
           - .. code-block:: shell
              
               export ECF_USER=my_user_name

             To avoid setting environment variable we can use:

             .. code-block:: shell
                  
                ecflow_client --user my_user_name ......

   ecflow_server
      This executable is the server. 
      
      It is responsible for :term:`scheduling` the jobs and responding to :term:`ecflow_client` requests
      
      Multiple servers can be run on the same machine/host providing they are assigned a unique port number.
      
      The server records all requests in the log file.  
      
      The server will periodically (see ECF_CHECKINTERVAL) write out a :term:`check point` file. 
            
      The following environment variables control the execution of the server and may be set before the start of the server.
      ecflow_server will start happily with out any of these variables being set, since all of them have default values.
      
      .. list-table:: 
         :header-rows: 1
         
         * - Variable Name
           - Explanation
           - Default value
         * - :term:`ECF_HOME`
           - Home for all the :term:`ecFlow` files
           - Current working directory
         * - ECF_PORT
           - Server port number. Must be unique
           - 3141
         * - ECF_LOG
           - History or log file
           - <host>.<port>.ecf.log
         * - ECF_CHECK
           - Name of the checkpoint file
           - <host>.<port>.ecf.check
         * - ECF_CHECKOLD
           - Name of the backup checkpoint file
           - <host>.<port>.ecf.check.b
         * - ECF_CHECKINTERVAL
           - Interval in second to save :term:`check point` file
           - 120
         * - ECF_LISTS
           - White list file. Controls read/write access to the server for each user
           - <host>.<port>.ecf.lists
         * - ECF_TASK_THRESHOLD
           - Report in log file all task/job that take longer than given threshold. Used to debug/instrument, those scripts that are very large.
           - 4000 (milliseconds). Before release 4.0.6 default was 2000 ms.
         * - :term:`ECF_PASSWD`
           - path to server password file, used to authenticate :term:`user command`\ s. Use when ALL should be password authenticated
           - <host>.<port>.ecf.passwd
         * - ECF_CUSTOM_PASSWD
           - path to server password file, used to authenticate :term:`user command`\ s. Use when a small number of users need to be password authenticated. Typically client would use:ecflow_client --user=fred ....export ECF_USER=fred; ecflow_client ...
           - <host>.<port>.ecf.custom_passwd
         * - ECF_PRUNE_NODE_LOG
           - When the checkpoint point file is loaded, node log history older than 30 days is automatically pruned. The variable allows this value to be changed.Setting the variable to zero, means there will be no pruning. All history is preserved at the cost increasing server memory, and time taken to write checkpoint file.
           - .. code-block:: shell
            
               export ECF_PRUNE_NODE_LOG=40
               
             Prune node log history older than 40 days, upon reload of :term:`check point` file.
         * - ECF_SSL
           - For secure socket communication with client.Requires client/server built with openssl libs
           - .. code-block:: shell
              
               #Use same certificate for multiple servers
               export ECF_SSL=1
               # Use server specific certificates
               export ECF_SSL=<host>.<port> 
               
             Alternatively to avoid setting environmental variables we can use:

             .. code-block:: shell
               
               ecflow_server --ssl ... || ecflow_start.sh -s
              
             The server will then first look for $HOME/.ecflowrc/ssl/server.crt then $HOME/.ecflowrc/ssl/<host>.<port>.crt


      The server can be in several states. The default when first started is :term:`halted`, See :term:`server states`
      
   ecflow_ui
      ecflow_ui executable in the new GUI based client. It is used to visualise and monitor the hierarchical structure of the :term:`suite definition`.

   event
      The purpose of an event is to signal partial completion of a :term:`task` and to be able to 
      trigger another job which is waiting for this partial completion. 
      
      Only tasks can have events and they can be considered as an attribute of a :term:`task`. 
      
      There can be many events and they are displayed as nodes. 
      
      The event is updated by placing the ``--event`` :term:`child command` in a :term:`ecf script`.
      
      An event has a number and possibly a name. If it is only defined as a number, 
      its name is the text representation of the number without leading zeroes.
      
      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`event_cli`
         * - :ref:`python_api`
           - :py:class:`ecflow.Event`, :py:class:`ecflow.Node.add_event`
         * - :ref:`grammar`
           - :token:`event`
              
      Events can be referenced in :term:`trigger` and :term:`complete expression` s.
     
   extern
      This allows an external :term:`node` to be used in a :term:`trigger` expression. 
      
      All :term:`node`\ s in :term:`trigger`\ s must be known to :term:`ecflow_server` by the end of the load command. 
      No cross-suite :term:`dependencies` are allowed unless the names of tasks outside the suite are declared as external. 
      An external :term:`trigger` reference is considered unknown if it is not defined when the :term:`trigger` is evaluated. 
      You are strongly advised to avoid cross-suite :term:`dependencies`. 
      
      Families and suites that depend on one another should be placed in a single :term:`suite`. 
      If you think you need cross-suite dependencies, you should consider merging the suites 
      together and have each as a top-level family in the merged suite.
      
      For grammar see :token:`extern`.
      
          
   family
      A family is an organisational entity that is used to provide hierarchy and grouping. 
      It consists of a collection of :term:`task`\ s and families.
      
      Typically you place tasks that are related to each other inside the same family, analogous to the way you 
      create directories to contain related files. 
      For python see :py:class:`ecflow.Family`. For BNF see :token:`family`
      
      It serves as an intermediate :term:`node` in a :term:`suite definition`.
      
   halted
      Is a :term:`ecflow_server` state. See :term:`server states`.
      
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
      
   inlimit
      The inlimit works in conjunction with :term:`limit`/:py:class:`ecflow.Limit` for providing simple load management. inlimit is added to the :term:`node` that needs to be limited.
      
      .. code-block:: shell
         :caption: Limiting tasks, only allow 5 tasks to run in parallel

         suite suite
            limit disk 100
            family anon   
               inlimit /suite:disk 5
               task t1
               ...
               task t100
            endfamily
         endsuite         

      .. code-block:: shell
         :caption: Limiting Families, only two families can run in parallel. The tasks are unconstrained

            suite test
               limit fam 2
               family f1
                  inlimit -n fam
                  task t1
                  ....
               endfamily
               family f2
                  inlimit -n fam
                  task t1
                  ....
               endfamily
               family f3
                  inlimit -n fam
                  task t1
                  ....
               endfamily
            endsuite

      .. code-block:: shell
         :caption: Limit submission

            # Hence we could have more than 2 active jobs, since we are only control the number in the submitted state.
            # If we removed the -s then we can only have two active jobs running at one time
            suite test_limit_on_submission
               limit disk 2
               family anon  
                  inlimit -s disk   # Inlimit submission
                  task t1
                  task t2
                  ....
               endfamily
            endsuite

      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.InLimit`, :py:class:`ecflow.Node.add_inlimit`
         * - :ref:`grammar`
           - :token:`inlimit`
      
   job creation
      Job creation or task invocation can be initiated manually via :term:`ecflow_ui` but also by 
      the :term:`ecflow_server` during :term:`scheduling` when a :term:`task` (and *all* of its parent :term:`node` s) is free of its :term:`dependencies`.
             
      The process of job creation includes:
      
      - Generating a unique password :term:`ECF_PASS`, which is placed in :term:`ecf script` during :term:`pre-processing`. See :ref:`tutorial-head_h`
          
      - Locating :term:`ecf script` files , corresponding to the :term:`task` in the :term:`suite definition`, See :term:`ecf file location algorithm`
         
      - :term:`pre-processing` the contents of the :term:`ecf script` file       
                  
      The steps above transforms an :term:`ecf script` to a :term:`job file` that can be submitted by
      performing :term:`variable substitution` on the :term:`ECF_JOB_CMD` :term:`variable` and invoking the command.
         
      The running jobs will communicate back to the :term:`ecflow_server` by calling :term:`child command`\ s.
        
      This causes :term:`status` changes on the :term:`node`\ s in the :term:`ecflow_server` and flags can be set 
      to indicate various events.  
      
      If a :term:`task` is to be treated as a dummy task (i.e. is used as a scheduling task) and is not meant to
      to be run, then a variable of name :term:`ECF_DUMMY_TASK` can be added:

      .. code-block:: shell
      
            task.add_variable("ECF_DUMMY_TASK","")
      
      
   job file
      The job file is created by the :term:`ecflow_server` during :term:`job creation` using the :term:`ECF_TRYNO` :term:`variable`
      
      It is derived from the :term:`ecf script` after expanding the pre-processing :term:`directives`. 
      
      It has the form <task name>.job<:term:`ECF_TRYNO`>", i.e. t1.job1. 
      
      Note job creation checking will create a job file with an extension with zero. i.e '.job0'. See :py:class:`ecflow.Defs.check_job_creation`
      
      When the job is run the output file has the :term:`ECF_TRYNO` as the extension.
      i.e t1.1 where 't1' represents the task name and '1' the :term:`ECF_TRYNO`  
      
   label
      A label has a name and a value and is a way of **displaying** information in :term:`ecflow_ui`
      
      By placing a label :term:`child command`\ s in the :term:`ecf script` the user can be informed about progress
      in :term:`ecflow_ui`.

      Labels can be added to family nodes. To change the labels, scripts should use:
      
      .. code-block:: shell
         
         ecflow_client --alter change label <label_name> <new_value> /path/to/family_node/with/label

      If the label :term:`child command`\ s results in a :term:`zombie` then the default action if for the server to **fob**, this allows the ecflow_client command to exit normally. (i.e. without any errors). This default can be overridden by using a :term:`zombie` attribute.

      .. list-table::
 
         * - :ref:`ecflow_cli`
           - :ref:`label_cli`, :ref:`add_cli`, :ref:`alter_cli` 
         * - :ref:`python_api`
           - :py:class:`ecflow.Label`, :py:class:`ecflow.Node.add_label`
         * - :ref:`grammar`
           - :token:`label`

   late
      Define a tag for a node to be late. A node can only have **one** late attribute. The late attribute only applies to a task. You can define it on a Suite/Family in which case it will be inherited. Any late defined lower down the hierarchy will override the aspect(submitted,active, complete) defined higher up.

      Command options:

      * -s submitted: The time node can stay submitted (format ``[+]hh:mm``). submitted is always relative, so + is simple ignored, if present. If the node stays submitted longer than the time specified, the late flag is set
      * -a active: The time of day the node must have become active (format ``hh:mm``). If the node is still queued or submitted, the late flag is set
      * -c complete: The time node must become complete (format ``{+}hh:mm``). If relative, time is taken from the time the node became active, otherwise node must be complete by the time given.

      .. code-block:: shell
                  
         suite late
            family familyName
               task t1
                     late -s +00:15 -a 20:00 -c +02:00
               task t2
                     late -a 20:00 -c +02:00 -s +00:15
               task t3
                     late -c +02:00 -a 20:00  -s +00:15
               task t4
                     late  -s 00:02 -c +00:05
               task t5
                     late  -s 00:01 -a 14:30 -c +00:01
            endfamily
         endsuite

      Suites cannot be late, but you can define a late tag 
      for submitted in a suite, to be inherited by the families and tasks. 
      When a node is classified as being late, the only action :term:`ecflow_server` takes is to set a flag. 
      :term:`ecflow_ui` will display these alongside the :term:`node` name as an icon (and optionally pop up a window).
      
      .. code-block:: shell
                  
         suite late
            late -s +00:15    # report late for all task taking longer than 15 minutes in submitted state
            family familyName
               late -c +02:00 # all child task that take longer than 2 hours to complete should raise a late flag
               task t1
                     # effective late -s +00:05 -c +02:00
                     late -s +00:05  
               task t2
                     # effective late  -s +00:15 -c +02:00
               task t5
                     # effective late  -c +03:00 -a 18:00 -s +00:15
                     late -c +03:00 -a 18:00 
            endfamily
         endsuite  

      The late attribute can be added/deleted to any suite/family/task.

      .. code-block:: shell

         ecflow_client --alter add    late "-s 00:15" <path-to-node>
         ecflow_client --alter change late "-s 00:01 -a 14:30 -c +00:01" <path-to-node>
         ecflow_client --alter delete late  

      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`add_cli`, :ref:`alter_cli` 
         * - :ref:`python_api`
           - :py:class:`ecflow.Late`, :py:class:`ecflow.Node.add_late`
         * - :ref:`grammar`
           - :token:`late`
         
   limit
      Limits provide simple load management by limiting the number of tasks
      submitted by a specific :term:`ecflow_server`.
      Typically you either define limits on :term:`suite` level or define a separate suite to 
      hold limits so that they can be used by multiple suites. 
      
      Setting limits on a separate suite,  has the benefit that by setting the limit value to zero, you can control task submission over a number of suites.

      .. code-block:: shell
         :caption: Limits

         suite suiteName
            limit sg1  10
            limit mars 10
         endsuite

      The limits are used in conjunction with :term:`inlimit`.

      The limit max value can be changed on the command line:
      
      .. code-block:: shell

         ecflow_client --alter change limit_max <limit-name> <new-limit-value> <path-to-limit>
         ecflow_client --alter change limit_max limit 2 /suite
         
      It can also be changed in python:
      
      .. code-block:: python
         
         import ecflow

         try:
            ci = ecflow.Client()
            ci.alter("/suite","change","limit_max","limit", "2")   
         except RuntimeError, e:
            print("Failed: " + str(e))
      
      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`add_cli`, :ref:`alter_cli` 
         * - :ref:`python_api`
           - :py:class:`ecflow.Limit`, :py:class:`ecflow.Node.add_limit`
         * - :ref:`grammar`
           - :token:`limit`

   manual page
      Manual pages are part of the :term:`ecf script`. 
      
      This is to ensure that the manual page is updated when the :term:`ecf script` is updated. 
      The manual page is a very important operational tool allowing you to view a description of a task,
      and possibly describing solutions to common problems.  
      The :term:`pre-processing` can be used to extract the manual page from the script file and is visible in :term:`ecflow_ui`.  
      The manual page is the text contained within the %manual and %end :term:`directives`.  
      They can be seen using the Manual tab in the Info panel in :term:`ecflow_ui`.
      
      The text in the manual page in **not** included in the :term:`job file`.
      
      There can be multiple manual sections in the same :term:`ecf script` file. When viewed they are
      simply concatenated. It is good practice to modify the manual pages when the script changes.
      
      The manual page may have the %include :term:`directives`.

      Suite and families may also have a manual page. These will also be available in the GUI. Ecflow will look for a file ``<node_name>.man`` (where node_name is the name of suite or family) using a backwards search algorithm first in ECF_FILES directory, then ECF_HOME directory. Note that errors in variable pre-processing are ignored inside of a manual section. It should also be noted that for family and suite manuals, the %manual and %end directives are not strictly necessary, as the whole file is treated as a manual. 
 
      If we have family ``/suite/big/f1``, ecFlow will search for "f1.man" in:

      .. code-block:: shell

         <ECF_FILES>/suite/big/f1.man
         <ECF_FILES>/suite/f1.man
         <ECF_FILES>/f1.man
         <ECF_HOME>/suite/big/f1.man
         <ECF_HOME>/suite/f1.man
         <ECF_HOME>/f1.man
            
   meter
      The purpose of a meter is to signal proportional completion of a task and to 
      be able to trigger another job which is waiting on this proportional completion.
      
      The meter is updated by placing the --meter :term:`child command` in a :term:`ecf script`. Meters can be added to family nodes. To change the meters, in the scripts should use:
      
      .. code-block:: shell
         
          ecflow_client --alter change meter <meter_name> <new_value> /path/to/family_node/with/meter 
          
      If the meter :term:`child command` results in a zombie, then the default action if for the server to **fob**, this allows the ecflow_client command to exit normally (i.e. without any errors). This default can be overridden by using a zombie attribute.

      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`meter_cli`, :ref:`add_cli`, :ref:`alter_cli` 
         * - :ref:`python_api`
           - :py:class:`ecflow.Meter`, :py:class:`ecflow.Node.add_meter`
         * - :ref:`grammar`
           - :token:`meter`

      Meters can be referenced in :term:`trigger` and :term:`complete expression` expressions.
      
   node
      :term:`suite`, :term:`family` and :term:`task` form a hierarchy.
      Where a :term:`suite` serves as the root of the hierarchy. 
      The :term:`family` provides the intermediate nodes, and the :term:`task`
      provide the leafs. 
      
      Collectively :term:`suite`, :term:`family` and :term:`task` can be referred
      to as nodes.     
      
      For python see :py:class:`ecflow.Node`.  
      
   pre-processing
      Pre-processing takes place during :term:`job creation` and acts on :term:`directives` specified in :term:`ecf script` file.
      
      This involves:
      
         - expanding any include file :term:`directives`.  i.e similar to 'c' language pre-processing
         
         - removing comments and manual :term:`directives`
         
         - performing :term:`variable substitution`  
         
   queue
      Queues allows efficiently running jobs that are identical but vary only in the step.

      This attribute makes it possible to follow a producer(server)/consumer(tasks) pattern. Note additional task consumers can be added for load balancing.

      .. code-block:: shell

         suite test_queue
         family f1
            queue q1 001 002 003 004 005 006 007
            task t
         endfamily
         family f2
            queue q2 1 2 3 4 5 6 8 9 10
            task a
            task b
               # notice that queue name is accessible to the trigger
               trigger /test_queue/f1:q1 > 5     
            task c
               trigger ../f2/a:q2 > 9
         endfamily
         endsuite        

      The  :ref:`queue_cli` :term:`child command` will signal when a step is active, complete, or has aborted:
               
      .. code-block:: shell

         # Note: because --queue is treated like a child command(init,complete,event,label,meter,abort,wait), the task path ECF_NAME is read from the environment
         
         # The --queue command will search up the node hierarchy for the queue name. If not found it fails.
         
         step=$(ecflow_client --queue queue_name  active)                # returns first queued/aborted step from the server and makes it active, Return "NULL" for the last step.
         ecflow_client --queue queue_name complete $step                 # Tell the server that step has completed for the given queue
         ecflow_client --queue queue_name aborted  $step                 # Tell the server that step has aborted for the given queue
         no_of_aborted=$(ecflow_client --queue queue_name no_of_aborted) # returns as a string the number of aborted steps
         ecflow_client --queue queue_name reset        

      The queue values can be strings, however, if they are to be used in :term:`trigger` expressions, they must be convertible to integers:

      .. code-block:: shell

         suite test_queue
            family f1
               queue q1 red orange yellow green blue indigo violet
               task t
            endfamily
         endsuite

      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`queue_cli`
         * - :ref:`python_api`
           - :py:class:`ecflow.Queue`, :py:class:`ecflow.Node.add_queue`
         * - :ref:`grammar`
           - :token:`queue`


   queued
      Is a :term:`node` :term:`status`. 
      
      After the begin command, the task **without** a :term:`defstatus` are placed into the queued state
      
   real clock
       A :term:`suite` using a real :term:`clock` will have its :term:`clock` matching the clock of the machine.
       Hence the :term:`date` advances by one day at midnight. 
   
   repeat
      Repeats provide looping functionality. There can only be a single repeat on a :term:`node`.

      .. code-block:: shell
      
         repeat day step [ENDDATE]   # only for suites
         repeat integer VARIABLE start end [step]
         repeat enumerated VARIABLE first [second [third ...]]        
         repeat string VARIABLE str1 [str2 ...]        
         repeat file VARIABLE filename       
         repeat date VARIABLE yyyymmdd yyyymmdd [delta]
         repeat datelist VARIABLE yyyymmdd(1) yyyymmdd(2) ...

      
      The repeat variable name is available as a generated variable.

      The **repeat date** defines additional generated variables (from ecFlow 4.7.0), which are scoped with prefix of the variable name i.e.:
            
      .. code-block:: shell

         <variable>           # the default, the value is the current date
         <variable>_YYYY      # The year
         <variable>_MM        # the month
         <variable>_DD        # The day of the month
         <variable>_DOW       # day of the week  
         <variable>_JULIAN    # the julian value for the date 

      For example:

      .. code-block:: shell
         :caption: Repeat date generated variables, accessible for trigger expressions

         repeat date YMD 20090101 20220101
         # The following generated variables, are accessible for trigger expressions
         # YMD, YMD_YYYY, YMD_MM, YMD_DD, YMD_DOW,YMD_JULIAN 

      The repeat VARIABLE can be used in :term:`trigger` and :term:`complete expression` expressions.
      
      As the repeat variable changes so do the generated variables. (See the tutorial for an example. Repeat)
      
      .. warning::

         If a repeat is added to a family/suite, then the repeat will ONLY loop(and automatically re-queue its children) if all the children are complete. Hence additional care needs to be taken. i.e. if the parent node has a repeat and the child  has a cron attribute then the cron will always force a re-queue on the node once it has run, and hence will stop the parent from looping.

      If we use relative time attribute. i.e. time +02:00, under a repeat, then the time is relative to the repeat re-queue.

      The repeat VARIABLE can be used in :term:`trigger` and :term:`complete expression` expressions. Depending on the kind of repeat the value can vary:

      .. code-block:: shell

         RepeatDate       -> value
         RepeatDateList   -> value
         RepeatString     -> index  (will always return a index)
         RepeatInteger    -> value
         RepeatEnumerated -> value | index  ( return value at index if cast-able to integer, otherwise return index )
         RepeatDay        -> value


      If a "repeat date" VARIABLE is used in a trigger expression then date arithmetic is used,
      when the expression uses addition and subtraction. i.e.:

      .. code-block:: python
      
         defs = ecflow.Defs()
         s1 = defs.add_suite("s1");
         t1 = s1.add_task("t1").add_repeat( ecflow.RepeatDate("YMD",20090101,20091231,1) );
         t2 = s1.add_task("t2").add_trigger("t1:YMD - 1 eq 20081231");
         assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101 - 1  == 20081231"
      
      When we use relative time attributes under a Repeat. They are automatically reset when the repeat loops. Take for example:

       .. code-block:: shell

          suite s1
             family hc00
                repeat integer HYEAR 1993 2017
                time +00:01                     # when the repeat loops delay starting task a, for 1 minute
                task a
                task b
                   trigger a  == complete
             endfamily
          endsuite

      Now when task 'a' and Task 'b' complete, the repeat is incremented, and any relative time attributes are reset. In this case effectively delaying the starting of task 'a' for 1 minute.

      See also:

      .. list-table::

         * - :ref:`ecflow_cli`
           - :ref:`add_cli`, :ref:`alter_cli`    
         * - :ref:`python_api`
           - :py:class:`ecflow.Node.add_repeat`, :py:class:`ecflow.Repeat`, :py:class:`ecflow.RepeatDate`, :py:class:`ecflow.RepeatEnumerated`, :py:class:`ecflow.RepeatInteger`, :py:class:`ecflow.RepeatDay`
         * - :ref:`grammar`
           - :token:`repeat`
      
   running
      Is a :term:`ecflow_server` state. See :term:`server states`
   
   scheduling
      The :term:`ecflow_server` is responsible for :term:`task` scheduling. 
      
      It will check :term:`dependencies` in the :term:`suite definition` every minute. 
      If these :term:`dependencies` are free, the :term:`ecflow_server` will submit the task. 
      See :term:`job creation`.
   
   server states
      The following tables reflects the :term:`ecflow_server` capabilities in the different states

      ================  ============   ============  ==============  ===================
      State             User Request   Task Request  Job Scheduling  Auto-Check-pointing 
      ================  ============   ============  ==============  ===================
      :term:`running`      yes           yes            yes             yes               
      :term:`shutdown`     yes           yes            no              yes               
      :term:`halted`       yes           no             no              no                
      ================  ============   ============  ==============  ===================  
       
   shutdown
      Is a :term:`ecflow_server` state. See :term:`server states`
      
   status
      Each :term:`node` in :term:`suite definition` has a status. 
      
      Status reflects the state of the :term:`node`. 
      In :term:`ecflow_ui` the background colour of the text reflects the status.  
      
      :term:`task` status are: :term:`unknown`, :term:`queued`, :term:`submitted`, :term:`active`, :term:`complete`, :term:`aborted` and :term:`suspended`
      
      :term:`ecflow_server` status are: :term:`shutdown`, :term:`halted`, :term:`running` 
      this is shown on the root node in :term:`ecflow_ui`
      
   submitted
      Is a :term:`node` :term:`status`. 
      
      When the :term:`task` :term:`dependencies` are resolved/free the :term:`ecflow_server` places the task into a submitted state.
      However if the :term:`ECF_JOB_CMD` fails, the task is placed into the :term:`aborted` state
      
   suite
      A suite is an organisational entity. It is serves as the root :term:`node` in a :term:`suite definition`.
      It should be used to hold a set of jobs that achieve a common function. It can be used to hold
      user :term:`variable`\ s that are common to all of its children.
      
      Only a suite node can have a :term:`clock`.
      
      Suite generated variables:

      .. list-table::
         :widths: 20 80

         * - SUITE
           - The name of the suite
         * - ECF_TIME
           - 23:30 the current suite time
         * - TIME
           - 2330 time as integer, Can be used in a trigger expression, ideally using <=, <, >=, >
         * - YYYY
           - The year as an integer
         * - DOW
           - Day of the week, as an integer. Sunday=0,Monday=1,etc
         * - DOY
           - Day of the year, as an integer
         * - DAY
           - The days as a string, i.e. monday
         * - DD
           - Day of the month as an integer
         * - MM
           - The month as an integer
         * - MONTH
           - as a string
         * - ECF_DATE
           - YYYMMDD   year,month,day of the month as 8 digit integer
         * - ECF_JULIAN
           - The julian value of the current date (added in ecFlow 4.7.0)
         * - ECF_CLOCK
           - <day>:<month>:<day of week>:<day of year>. i.e.  Tuesday:December:2:348

      It is a collection of :term:`family`\ s, :term:`variable`\ s, :term:`repeat` and a single
      :term:`clock` definition. 
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Suite`
         * - :ref:`grammar`
           - :token:`suite`
      
   suite definition
      The suite definition is the hierarchical :term:`node` tree. It describes how your :term:`task`\ s run and interact. It can be built up using:
      
      * Ascii text file by following the rules defined in the ecFlow :ref:`grammar`. Hence any language can be used, to generate this format.
         
      * :ref:`python_based_suite_definition`
         
      Once the definition is built, it can be loaded into the :term:`ecflow_server`, and started. 
      It can be monitored by :term:`ecflow_ui`
   
   suspended
      Is a :term:`node` state. A :term:`node` can be placed into the suspended state via a :term:`defstatus` or via :term:`ecflow_ui`
      
      A suspended :term:`node` including any of its children cannot take part in :term:`scheduling` until
      the node is resumed.

   task
      A task represents a job that needs to be carried out. 
      It serves as a leaf :term:`node` in a :term:`suite definition`
       
      Only tasks can be submitted. 
      
      A job inside a task :term:`ecf script` should generally be re-entrant
      so that no harm is done by rerunning it, since a task may be automatically 
      submitted more than once if it aborts.
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Task`
         * - :ref:`grammar`
           - :token:`task`
            
   time
      This defines a time dependency for a node. 
      
      Time is expressed in the format ``[h]h:mm``. Only numeric values are allowed.

      There can be multiple time dependencies for a node, 
      but overlapping times may cause unexpected results. 

      .. code-block:: shell
         :caption: The task is free to run when the time is 10:00 or 11:00

         task t          
            time 10:00  
            time 11:00         

      To define a series of times, specify the start time, end time and a time increment. 
      If the start time begins with '+', times are relative to the beginning of the suite or, 
      in repeated families, relative to the beginning of the repeated family.
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g.:

      .. code-block:: shell
      
         time 10:00 20:00 01:00 
         
      If the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Time`, :py:class:`ecflow.Node.add_time`
         * - :ref:`grammar`
           - :token:`time`

   time dependencies
      This includes :term:`time`, :term:`today`, :term:`day`, :term:`date` and :term:`cron`.
      
      When we have multiple time dependencies on the same task, then time dependency of the same type are **or'ed** together, and **and'ed** with the different types.
      
      .. code-block:: shell
         :caption: This task will run on the 17th of February 2017 at 10am
         
         task xx        
            time 10:00
            date 17.2.2017

      .. code-block:: shell
         :caption: Run task xx. at 10am and 8pm, on the 17th and 19th of February 2017, that is four times in all. Notice the task is queued in between and completes only after the last run

         task xx         
            time 10:00
            time 20:00
            date 17.2.2017
            date 19.2.2017

   today
      Like :term:`time`, but If the suites begin time is **past** the time given for the "today" command the :term:`node` is free
      to run (as far as the time dependency is concerned). 
      
      For example:

      .. code-block:: shell

         task x
            today 10:00
            
      If we begin or re-queue the :term:`suite` at 9.00 am, then the :term:`task` in held until 10.00 am.
      However if we begin or re-queue the suite at 11.00am, the :term:`task` is run immediately.
      
      No lets look at time:
      
      .. code-block:: shell

         task x
            time 10:00
            
      If we begin or re-queue the :term:`suite` at 9.00am, then the :term:`task` in held until 10.00 am.
      If we begin or re-queue the :term:`suite` at 11.00am, the :term:`task` is still held.      
      
      If the time the job takes to complete is longer than the interval a “slot” is missed, 
      e.g.:
      
      .. code-block:: shell

         today 10:00 20:00 01:00 
         
      If the 10:00 run takes more than an hour, the 11:00 run will never occur.
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Today`
         * - :ref:`grammar`
           - :token:`today`
            
   trigger
      Triggers defines a dependency for a :term:`task` or :term:`family`. 
      
      There can be only one trigger dependency per :term:`node`, 
      but that can be a complex boolean expression of the :term:`status` of several nodes. 
      Triggers should be avoided on suites.
      A node with a trigger can only be activated when its trigger has expired. 
      A trigger holds the node as long as the trigger expression evaluation returns false. 
      
      Trigger evaluation occurs when ever the :term:`child command` communicates with the server. i.e whenever
      there is a state change in the suite definition.
      
      The keywords in trigger expressions are: :term:`unknown`, :term:`suspended`, :term:`complete`, :term:`queued`, :term:`submitted`, :term:`active`, :term:`aborted`
      and **clear** and **set** for :term:`event` status.
      
      Triggers can also reference Node attributes like :term:`event`, :term:`meter`, :term:`variable`, :term:`repeat` and generated variables.
      Trigger evaluation for node attributes uses integer arithmetic:
      
      - :term:`event`: has the integer value of 0(clear) and set(1)
      - :term:`meter`: values are integers hence they are used as is
      - :term:`variable`: value is converted to an integer, otherwise 0 is used. See example below
      - :term:`repeat` *string*: use the index values as integers. See example below
      - :term:`repeat` *enumerated*: use the index values as integers. See example below
      - :term:`repeat` *integer*: use the implicit integer values
      - :term:`repeat` *date*: use the date values as integers. Use of plus/minus on repeat date variable uses date arithmetic
      - :term:`limit`: the limit value is used as an integer. This allows a degree of prioritisation amongst tasks under a limit
      - :term:`late`: the value is stored in a flag, and is a simple boolean. Used to signify when a task is late.
       
      Here are some examples:

      .. code-block:: shell
         :caption: Trigger examples
     
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
  
      What happens when we have multiple node attributes of the same name, referenced in trigger expressions?
      
      .. code-block:: shell
         :caption: Trigger priority when name clashes

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
      - :term:`limit`
      
      Hence in the example above expression ``foo:blah >= 0`` will reference the event.
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Expression`, :py:class:`ecflow.Node.add_trigger`
         * - :ref:`grammar`
           - :token:`trigger`
       
   unknown
      Is a :term:`node` :term:`status`. 
      
      This is the default :term:`node` :term:`status` when a :term:`suite definition` is loaded into the :term:`ecflow_server`
      
   user command
      User commands are any client to server requests that are **not** :term:`child command`\ s.
      
   variable
      ecFlow makes heavy use of different kinds of variables.There are several kinds of variables:
      
      - Environment variables: which are set in the UNIX shell before the :term:`ecFlow` starts. These control :term:`ecflow_server`, and :term:`ecflow_client`  .
           
      - suite definition variables: Also referred to as user variables. These control :term:`ecflow_server`, and :term:`ecflow_client` and are available for use in :term:`job file`.  
         
      - Generated variables: These are generated within the :term:`suite definition` node tree during :term:`job creation` and are available for use in the :term:`job file`.
         
      Variables can be referenced in :term:`trigger` and :term:`complete expression`\ s . The value part of the variable should be convertible to an integer otherwise a default value of 0 is used.
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.Node.add_variable`
         * - :ref:`grammar`
           - :token:`variable`
      
   variable inheritance
      When a :term:`variable` is needed at :term:`job creation` time, it is first sought in the :term:`task` itself.
       
      If it is not found in the :term:`task`, it is sought from the task's parent and so on, up through 
      the :term:`node` levels until found.
      
      For any :term:`node`, there are two places to look for variables. 
      
      Suite definition variables are looked for first, and then any generated variables. 
      
   variable substitution
      Takes place during :term:`pre-processing` *or* command invocation.(i.e :term:`ECF_JOB_CMD`,ECF_KILL_CMD,etc)
      
      It involves searching each line of :term:`ecf script` file or command, for :term:`ECF_MICRO` character. typically '%'
      
      The text between two % character, defines a variable. i.e %VAR%
      
      This variable is searched for in the :term:`suite definition`. 
      
      First the suite definition variables (sometimes referred to as user variables) are searched
      and then Repeat variable name, and finally the generated variables. If no variable
      is found then the same search pattern is repeated up the node tree.
      
      The value of the :term:`variable` is replaced between the % characters. 
      
      If the micro character are not paired and an error message is written to the log file, 
      and the task is placed into the :term:`aborted` state.
      
      If the variable is not found in the :term:`suite definition` during pre-processing then :term:`job creation` fails, 
      and an error message is written to the log file, and the task is placed into the :term:`aborted` state.
      To avoid this, variables in the :term:`ecf script` can be defined as:
       
      .. code-block:: shell
      
         %VAR:replacement% 
         
      This is similar to %VAR% but if VAR is not found in the :term:`suite definition` then 'replacement' is used.
      
   virtual clock
      Like :term:`real clock` until the :term:`ecflow_server` is suspended (i.e :term:`shutdown` or :term:`halted`), 
      the suites :term:`clock` is also suspended.
      
      Hence will honour relative times in :term:`cron`, :term:`today` and :term:`time` dependencies.
      It is possible to have a combination of hybrid/real and virtual.
      
      More useful when we want complete adherence to time related dependencies at the expense
      being out of sync with system time.
      
   zombie
      Zombies are running jobs that fail authentication when communicating with the :term:`ecflow_server`
      
      :term:`child command`\ s like (init, event,meter, label, abort,complete) are placed in the :term:`ecf script`
      file and are used to communicate with the :term:`ecflow_server`. 
      
      The :term:`ecflow_server` authenticates each connection attempt made by the :term:`child command`. 
      Authentication can fail for a number of reasons:
      
         - password(ECF_PASS) supplied with the :term:`child command`, does not match the one in the :term:`ecflow_server`
         - path name(ECF_NAME) supplied with the :term:`child command`, does not locate a :term:`task` in the :term:`ecflow_server`
         - process id(ECF_RID) supplied with :term:`child command`, does not correspond with the one stored in the :term:`ecflow_server`
         - :term:`task` is already :term:`active`, but receives another init :term:`child command`
         - :term:`task` is already :term:`complete`, but receives another :term:`child command`
         - :term:`task` is already :term:`aborted`, but receives another :term:`child command`
         
      When authentication fails the job is considered to be a zombie.
      The :term:`ecflow_server` will keep a note of the zombie for a period of time, before it is automatically removed.
      However the removed zombie, may well re-appear. (this is because each :term:`child command` will continue
      attempting to contact the :term:`ecflow_server` for 24 hours. This is configurable 
      see ECF_TIMEOUT on :term:`ecflow_client`)
      
      See also:

      .. list-table::

         * - :ref:`python_api`
           - :py:class:`ecflow.ZombieAttr`, :py:class:`ecflow.ZombieUserActionType`
         * - :ref:`grammar`
           - :token:`zombie`
      
      There are several types of zombies see :term:`zombie type` and :py:class:`ecflow.ZombieType`  
     
   zombie attribute
      The zombie attribute defines how a :term:`zombie` should be handled in an automated fashion.
      Very careful consideration should be taken before this attribute is added as it may hide a genuine problem.
      It can be added to any :term:`node`. But is best defined at the :term:`suite` or :term:`family` level. 
      If there is no zombie attribute the default behaviour is to block the :term:`child command`. 
      
      To add a zombie attribute in python, please see: :py:class:`ecflow.ZombieAttr`
      
   zombie type
      See :term:`zombie` and class :py:class:`ecflow.ZombieAttr` for further information.

      How do zombies arise.

      - Server crashed (or terminated and restarted) and the recovered check point file is out of date.
      - A task is repeatedly re-run, earlier copies will not be remembered.
      - Job sent by another ecflow_server , but which cannot talk to the original ecflow_server
      - Network glitches/network down
      - errors in script, i.e. multiple calls to init, complete
      - errors in job submission i.e. job submitted twice.
       
      There are several types of zombies:

      * **path**:
  
         - The task path cannot be found in the server, because node tree was deleted, replaced,reload, server crashed or backup server does not have node tree. 
         - Jobs could have been created, via server :term:`scheduling` or by :term:`user command`\ s
         
      * **user**: Job is created by :term:`user commands<user command>`\ s like, rerun, re-queue. User zombies are differentiated from server(scheduled) since they are automatically created when the force option is used and we have tasks in an :term:`active` or :term:`submitted` states.
         
      * **ecf**: Jobs are created as part of the normal :term:`scheduling`
         
         - Two init commands or task complete or aborted but receives another :term:`child command`
         - Server crashed (or terminated and restarted) and the recovered :term:`check point` file is out of date.
         - A :term:`task` is repeatedly re-run, earlier copies will not be remembered.
         - Job sent by another :term:`ecflow_server`, but which cannot talk to the original :term:`ecflow_server`
         - Network glitches/network down

      * **ecf_pid**: pid mismatched, Job scheduled twice. Check submitter.
      * **ecf_passwd**: Password mismatch, PID matches, system has re-cycled PID or hacked job file?
      * **ecf_pid_passwd**: Both PID and password mismatch. Re-queue & submit of active job?

      The type of the zombie is not fixed and may change.
