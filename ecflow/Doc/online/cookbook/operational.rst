
.. index::
   single: cookbook
   
.. _operational:
   
Toward an operational server.
---------------------------------------------------

At the beginning of the course, the ecflow server is started in a
directory, and tasks/header scripts are created, below the same
directory. it will rapidly lead to a situation with many files, in one
place (administrative server files, checkpoints files, log-file, white
list, tasks wrappers, but also all jobs/output/aliases files).

This may be difficult to **maintain** or update after some time.

==============
Files Location
==============

For a maintenable operational suite, we recommand to:

* start the server in a local /tmp directory (**/:ECF_HOME**)

* define **ECF_FILES** and **ECF_INCLUDE** variables, at suites
  and families level: scripts wrappers will be accessible for
  creation, and update under these directories.

* define **/suite:ECF_HOME** as another directory location, where
  jobs and related outputs will be found. These dynamic files may
  then be tar'ed as snapshot of the effective work associated to a
  suite/family, for later analysis or rerun.

* when server and remote jobs destination do not share a common
  directory for output-files, **ECF_OUT** variable needs to be present
  in the suite definition: it indicates the **remote output path**. In
  this situation, the suite designer is responsible to create the
  directory structure where the output file will be found. Most
  queueing system won't start the job, if this directory is absent,
  and the task may remain visible as submitted, from the ecFlow server
  side.

* after seding the job complete command, the job may copy its output
  to **ECF_NODE**, to enable direct access from ecFlow server.

  When a file is requested from the ecflow-server, it is limited to
  15k lines, to avoid the server spending too much time delivering
  very large output files.

  ecFlowview can be configured (**globally**,
  Edit-Preferences-Server-Option or **locally** top-node-menu->Options
  "Read Output an other files from disk when possible") to get the
  best expected behaviour.

==========
Log-Server
==========

It is possible to setup a **log-server**, to access 'live'
output from the jobs. ecFlow is provided with the perl script
logsvr.pl.

* it is configured to deliver files under specific directories, 

* configration variables are

  * LOGPORT # 9316 
  * LOGPATH # <path1>:<path2>:<path3>
  * LOGMAP  # mapping betwen requested path and real actual location

  As an example, with two possible storage destination::

    export LOGPATH=/s2o1/logs:/s2o2/logs # two possible 
    export LOGMAP=/s2o1/logs:/s2o1/logs:/s2o2/logs:/s2o2/logs # maps itself
    export LOGMAP=$LOGMAP:/tmp:/s2o1/logs:/tmp:/s2o2/logs     # map from /tmp

* It is started on the remote machine and ecFlowview GUI will
  contact it when the variables ECF_LOGHOST and ECF_LOGPORT are
  defined in the suite::

    edit ECF_LOGHOST c2a
    edit ECF_LOGPORT 9316

* it can be tested from the command line with telnet::

    telnet c2a 9316 # get <file> # list <directory>

=============
Backup Server
=============

it can be useful to have a backup ecFlow server in case of network,
disk or host machine crash. The backup server shall be activated on
another workstation, with the most recent check-point-file.

The 'ecf_hostfile' files can be created with the list of hostnames to
contact, when the link with the original ecFlow server is broken.

common task header head.h may be updated with::

  export ECF_HOSTFILE=$HOME/.ecf_hostfile

======
System
======

As soon as the basic principles of ecflow are understood and mastered,
setting up a project with operational constaints may face serveral
challenges:

* I/O and disk access is critical for the server:

* use a local file system (/:ECF_HOME /tmp),

* use a mounted file system, capable of handling demanding I/O,

* use a snapshot capable file system.

==============
Error Handling
==============

A task should abort as close to the problem as possible:

* trap is used to intercept external signals received by the jobs. For
  Linux, trapped signals are 1 2 3 4 5 6 7 8 13 15 24 31. A signal
  may be sent, when the job exceeds a cpu-timeout, a memory
  consumption threshold, a kill request from the server (kill -2),
  or a 'command line' kill from the (root) user.

* loosing the traping capability is easy: 

  * traping inheritance between the main ksh script and ksh function
    is system dependent. To maintain deterministic behaviour, do not
    hesitate to repeat trap setting::
  
      # ... a function in a task wrapper ...
      function make
      {
        %include <trap_func.h>
        # body of the function
        set -e; trap 0; return 0 ##### reset trap
      }      

      # trap_func.h example:
      for sgn in $SIGNAL_LIST 0 ; do
      trap "{ echo \"Error in function with signal $sgn\"; exit 1; }" $sgn
      done

  * calling rsh or ssh within a task will not propagate a remote
    error locally. 

    In most cases, a suite may run "as requested", with
    jobs completing. It is only possible to identify the problem
    through job output analysis, or when a task aborts later, in the
    absence of the expected products, or when a product user is
    reporting.

    Splitting the job into simple units (tasks), submitted directly
    to the expected destination is part of the suite design. It will
    lead to clear identification of submission problems, followed by
    red tasks, that can be rerun later when the problem has been
    solved.

* Early exits must be a choice of the task designer, calling 'trap 0;
  ecflow_client --complete; exit 0'. Using 'trap ERROR 0', early exit
  will call the ERROR function, and then 'ecflow_client --abort'

* unset variables can be detected thanks to 'set -u'

* time stamps may be added on a per line bases with variable PS4

* ECF_TRIES variables can be increased to allow multiple submission
  attemps (some jobs may become more verbose on second submission, or
  it can be a 'network glitch'

=====================
Server Administration
=====================

an 'admin' suite will be required: 

* to ensure that ecflow logfile is not filling up the disk, nor
  touching a quota limit, issuing regularly the command::

    ecflow_client --port %ECF_PORT% --host %ECF_NODE% --log=new

* to duplicate the checkpoint file, on a remote, backup server, or a
  slower long term archive system. (to handle the case when disk
  failure, hosting workstation problem, or network issue that does
  require backup server start).

* a white list file to control access for read-write users or read-only users

=============
CMD variables
=============

CMD variables shall be set and capable to submit/kill/query a job
locally and remotely. They are:

* on the server side:

  * ECF_JOB_CMD::

      edit ECF_JOB_CMD '%ECF_JOB% > %ECF_JOBOUT% 2>&1'
      edit ECF_JOB_CMD 'rsh %ECF_JOB% > %ECF_JOBOUT% 2>&1'

  * ECF_KILL_CMD::

      edit ECF_KILL_CMD '%kill -2 %ECF_RID% && kill -15 %ECF_RID%'

  * ECF_STATUS_CMD::

      edit ECF_STATUS_CMD '%ps --sid %ECF_RID% -f'

* on the client side:

  * ECF_CHECK_CMD:: 

     edit ECF_CHECK_CMD '%ps --sid %ECF_RID% -f'

  * ECF_URL_CMD (for html man pages for tasks, plots display, products
    arrival html page)::
  
     edit URLBASE https://software.ecmwf.int/wiki/display/
     edit URL     ECFLOW/Home
     edit ECF_CHECK_CMD '${BROWSER:=firefox} -remote "openURL(%URLBASE%/%URL%)"'

* alternatively, a script may be responsible for jobs
  submission/kill/query. At ECMWF, we use a submit script that tunes
  the generated job file to the remote destination.  It does:
    
  * translate queuing system directives to the expected syntax,

  * tune submission timeout according to submit user and remote destination,

  * use a submition utility according to the remote system, or even
    the way we want the job to be submitted there: nohup,
    standalone, rsh, ssh, ecrcmd

  * keep memory of the **remote queuing id** given to the job, stores it in a 
    ".sub" file, that may be used later by kill and query commands

  * handle frequent or specific errors with the submission: job may
    have been accepted, even if the submission command is reporting
    an error and shall not be reported as such to the server.

  * example::

      edit ECF_JOB_CMD    '$HOME/bin/ecf_submit %USER% %HOST% %ECF_JOB% %ECF_JOBOUT%
      edit ECF_KILL_CMD   '$HOME/bin/ecf_kill %USER% %HOST% %ECF_RID% %ECF_JOB%
      edit ECF_STATUS_CMD '$HOME/bin/ecf_status %USER% %HOST% %ECF_RID% %ECF_JOB% 

* remote jobs submission needs the server administrator, or the suite
  designer, to communicate with the system administration team, in
  order to decide:

  * shared, mounted, or local file systems according to best choice or
    topology, in the local network.

  * main submission schemes (rsh, ssh), 

  * alternative submission scheme (we may use nicknames to distinguish
    direct job submission from submission through a queueing system on
    the same host)

  * fall-back schemes (when c2a node is not available, c2a-batch is to
    be used, as alternative)

  * the best way to handle cluster switch (from c2a to c2b, as a
    variable on the top node, or multiple variables among the suites,  
    a shell variable, or even a one-line-switch in the submit script)

  * to handle remote storage switch (from /s2o1 to /s22o, as a server
    variable or a shell variable in the jobs)

  * submission time-outs,

  * notification before killing a job, (sending kill -2 signal), to
    give a chance to send the abort command.

===========
Task Design
===========

most tasks should be re-runnable and they should have an up to date
'manual section'.

=====
micro
=====

micro character (%) is used as variable delimiter, or to start
preprocessing directives (include, manual, end, nopp) in task wrappers.

* It can be changed in the definition file, as ECF_MICRO variable::

    edit ECF_MICRO @ # we shall find @include in the affected wrappers

* micro may change trough the job thanks to the directive
  %ecf_micro::

    %ecf_micro ^ # change micro to exponant character
    ^include "standalone_script.pl"
    ^ecf_micro % # revert back to original character

* %nopp can be used to avoid duplicating the '%' in some sections of
   the task wrapper where it can be frequently used (date, perl)

* %includenopp <file> is also a simple way to import a script that do
  not contain ecFlow prepprocessing directive, and that may contain
  the micro '%' character

================
Python Debugging
================

Python suite definition files sometimes lead to 'Memory fault'
message. Error can be understood running it with pdb or gdb::

  python -m pdb  <script.py>

  gdb python
  > set args suite.def
  > run
  > bt

.. header files examples: head.h trap.h endt.h rcp.h

.. Demo, Remote suites examples, 
