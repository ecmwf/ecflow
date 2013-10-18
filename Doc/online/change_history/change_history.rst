
.. index::
   single: change_history
   
.. _change_history:
   
==================
**Change History**
==================

4.0.0 (production, October 2013)
   - ecflowview    : Limited support to allow new clients to interact with old servers
   - ecflowview    : ECFLOW-71 error in ecflow on delete using GUID
   - ecflow_server : Pre-processor: %%%% should be replaced with %%, and not %
   - ecflow_server : Fixed cron(with time series) on a family, once free should stay free, until re-queued
   - ecflow_client : Updated suite definition parser, to allow suites to have labels
   - ecflow_client : Fix bug in the display of zombies. Action shown in log file did not match, command line display, when zombie attributes used.
   - ecflow_client : Updated zombie display, to prepend manual or automatic in front of action, i.e manual-fob.
                     This distinguishes between action taken by the user, from automatic action, especially when zombie attributes are used.
   - ecflow.so     : Added new python function to Node(find_node_up_the_tree()), to make it easier to add triggers.
   - *             : Update ecflow_migrate to take into account label's that have new lines
   - *             : Migrated source control management from perforce to git, and updated build scripts
   - *             : Updated build scripts to allow compilation and regression tests on cray XC30
   - *             : switch to boost 1.53
   
3.1.9: (production, September 2013)
   - ecflowview    : fix crash in the preference dialog
   - ecflow_client : Change parsing for aliases variables. No longer check variable names.
   - ecflow_client : Fixed parsing bug associated with trigger node references, relative paths not correct
   - ecflow_server : Changed Pre-process to ignore generated variables SUITE,FAMILY,TASK in %comment %end(user variables)
 
3.1.8: (production, September 2013)
   - ecflow_server : Fix --migrate bugs(reading cron state, and reading message history)
   - ecflow_server : Fix --migrate bugs: task, aborted reason, should not have \n. Need to use migrate.py, as fix.
   - ecflow_server : Changed --version to print date and time of compilation
   - ecflow_client : Update Repeat attribute to be able to return index_or_value(), Needed by ecflowview
   - ecflow.so     : Fixed python api bug, that allowed duplicate events.
   - ecflowview    : different colour possible beyond meter threshold
   - ecflowview    : Edit-Preferences-Colors window updated to accept meter-threshold-event color setting
   - ecflowview    : scrollbars added to TimePanel-Detail window
   - *             : Added support for the clang compiler (3.2) on linux
   - *             : Updated support for cmake 
  
3.1.7: (production, August 2013)
   - ecflow_server : Fix issue with combination of day/date and time attribute(time,today,cron)
                     Time/Today/cron should only be set free if day/date matches
   - ecflow_server : Removed reset of relative time, at midnight
   - ecflow_server : SUP-571 Time dependency after halt/checkpoint
   - ecflow_server : Changed RepeatEnumerated. Use value if cast-able as an integer, otherwise use index as value
                     Previously we only used the index.
   - tools         : ECFLOW-80 ecflow_status issue
   
3.1.6: (production, July 2013)
   - ecflow_server : ECFLOW-74 Improvements in the way the version number is reported, including boost version 
   - ecflow_server : ECFLOW-75 cron task not looping when run/executed manually 
   - ecflow_server : Allow use of PORTABLE_BINARY_ARCHIVE on AIX
   - ecflow_server : On re-queue clear child nodes that are suspended.
   - ecflow_server : Changed Day/Date attributes behaviour at midnight to be same as SMS.
                     i.e  once complete, stay complete, unless a parent cron/repeat cause a re-queue
   - tools         : Change to ecflow_start.sh. Now use ecf_{kill,status,submit}
   - ecflowview    : use stringstream for info and why windows.
   - ecflowview    : make menu-full to be applied as special-kill menu was not anymore

3.1.5: (production, June 2013)
   - ecflowview   : reduce blinking
   - ecflowview   : add keyword COMP accepting two string parameter for ecflowview.menu 
                   (facilitate node to node comparison for edit,output, job)
   - ecflow_server: Fix bug in variable substitution "printf %%02d %HOUR:00%"; expected = "printf %02d 00"
   - ecflow_server: ECFLOW-71 %include adds empty line in job creation
   - ecflow_server: Optimise Suite calendar updates, to minimise updates in viewer
   - ecflow_server: SUP 521 Job requeue after midnight instead of complete 
   - ecflow_server: Bug with single slot cron. Once free it should stay free until re-queued
   - ecflow_server: On ecgate invalid LANG environment variable, does not give proper error message 
   - ecflow_server: Change single slot time,today,cron attribute behaviour at midnight to be same as SMS
                     i.e  once complete, stay complete, unless a parent cron/repeat cause a re-queue
   - ecflow_client: Changed notification, to allow clients, to detect change in meter,event,label only
   - build        : minor change to test.sh script, used for testing
   - tools        : Change to ecflow_start.sh. Update LANG and sleep interval
     
3.1.4: (production, May 2013)
   - ecflowview:    Minor change to warning message dialog. 
   - ecflowview:    SUP-457 Repeat node update
   - ecflowview:    Add integer values used/total for Limit attribute
   - ecflowview:    Add statistics display on top node info tab window (empty servers 
                    have no variables displayed until the loading of a suite)
   - ecflowview:    Variables substitution for ECF_LOGHOST and ECF_JOBOUT
   - ecflow_client: Added Aspect for node attribute addition/deletion for ecflowview
   - ecflow_server: ECFLOW-70  Get back time after free-dep on ecFlow   
   - ecflow_server: cron should always re-queue, regardless of time types(single slot or series)
   - doc:           Updated after feedback from external training
   - tools:         Change to log server. Crash in log server sends mail to user
   - tools:         ECFLOW-69 SUPPORT: ecflow_start.sh overwrites ECF_LISTS / documentation 
   - ecflow.so:     test: Ignore tests which use 'with' statement if python < 2.7

3.1.3: (production, March 2013)
   - ecflow_client: Fixed '--alter change defstatus suspended'  
   - ecflowview:    Fix change fonts
   - ecflowview:    Change in server list is now saved
   - doc:           Updated after feedback from internal training

3.1.2: (production, March 2013)
   - ecflowview:    SUP-423 tool tip related crash 
   - ecflowview:    activate ECF_CHECK_CMD  
   - ecflowview:    maintain selection when aspect node replace happens
   - ecflowview:    delete/replace node tested OK with ecjobs family

3.1.1: (production, March 2013)
   
   - tools:         Addition of logsvr.pl and logsvr.sh scripts
   - ecflowview:    ECFLOW-59 incorrect "Why?" information for cron tasks in families with repeat dates
   - ecflowview:    SUP-398 does not register new suites all the time            
   - ecflowview:    SUP-391 why button only works if higher level is suspended 
   - ecflowview:    SUP-421 ecflow view crashes - client loggin added on extra-menu
   - ecflowview:    add find button in variables panel window
   - ecflow_client: changed "--get /state" will now show the externs for the suite only.
   - ecflow_client: test: added 'wait_for_server_death()' to make client/server test more deterministic
   - ecflow_server: SUP-408 what does submitted mean in log? 
   - ecflow_server: Trigger expression always evaluated, unless explicitly free'd by the user
                    Previously once a trigger expression was clear, during tree traversal,
                    it stayed clear until requeued. This allows the force queued to work as expected
   - ecflow.so:     Added Node::get_dstate() to python interface. DState includes 'suspended' as one of the node states
   - ecflow.so:     test: Fix bug with generate scripts, ECF_DUMMY_TASK should not generate scripts

3.1.0: (production, January 2013)

   - ecflowview: Added more lines for edit
   - ecflowview: Fix bug with rendering a repeat at the suite level
   - ecflowview: Show zombie attributes in the 'info' dialog
   - ecflowview: Trigger 'info' now shows the state of each expression.
   - ecflowview: ECFLOW-62 ecflowview crash on ecflow_client delete 
   - ecflowview: ECFLOW-53 crash upon ecflow_client --replace 
   - ecflowview: ECFLOW-63 ecflowview crash when why tab selected for job 
   - ecflowview: ECFLOW-64 SUPPORT: node search 
   - ecflowview: SUP 317 crash when why tab selected for job 
   - ecflowview: Performance: Removed debug XSynchronize, which caused display to hang.  
                 (SUP-349) ecflowview performance slow with ecflow 3_1_rc1
   - ecflow_client: Optimisation of client side functions, and parser
   - ecflow_client: Fix bug with parsing of ecf host file. Should default to use job port
   - ecflow_server: Performance: If request successful, no reply back to server, socket closed.
   - ecflow_server: Performance: Improved incremental sync for flags
   - ecflow_server: Performance: Avoid unnecessary stat() system calls during job generation
   - ecflow_server: Performance: Remove redundant call to block SIGCHLD, during job generation
   - ecflow_server: Enhancement: Changed signal installation for terminated child process
   - ecflow_server: Enhancement: excessive check pt save times now, raise late flag on server.
                    Changed user command's --stats and --check_pt
   - ecflow_server: Improved time to check point.
   - ecflow_server: Fixed bug with incremental sync of time,today and cron.
   - ecflow_server: Fixed unnecessary synchronisation, when all suites registered in a handle
                    This could affect ecflowView performance
   - ecflow_server: Fixed bug where server variable were not always synchronised
   - ecflow_server: make sure manual files are pre-processed
   - ecflow.so:     Update Python interface to allow zombies to be killed via zombie attribute.
 
3.0.1: (production, October 2012)

   - ecflowview, use of new icons to convey additional information
   - ecflowview, fixed static initialisation order bugs.(invocation crash on ecgate)
   - ecflowview, fixed change order
   - ecflowview, fixed modify server variables
   - ecflowview, fixed Z icon, and  BadDrawable (invalid Pixmap or Window parameter) 
   - ecflowview  will only connect to server if version number matches 
   - ecflowview  fixed crash when using repeat day
   - ECFLOW-50   ecflowview doesn't show output of task 
   - ECFLOW-49   ecflowview doesn't show server if no suite is running 
   - Added support for python 2.7 on HPUX
   - AIX rs6000,power6,power7 now built with v12 c++ compiler
   - Changed suites in handles so that they are always in same order as def suites
   - Added edit history functionality for the server/defs node. 
   - Reduced memory usage, when nodes don't have trigger/complete expressions
   - Downloads from server to client improved by ~25-40% for very large definitions(>60MB)
   - Fixed bug with trigger expression that have leading integers
   - Allowed defs file in the server to be migrated to future versions.
   - Periodic check pt only saved if there was a state change
   - Trigger expression use simple date arithmetic if referenced variable is a repeat DATE
   - Tested builds with boost 1.51, fixed issues with HPUX
   - Added support for use of eos portable binary archive
   - Fix crash when registering suites with an empty server
   - Change search algorithm for include files, when using angled brackets
   - Re-queue now correctly resets any missed time dependencies.

2.0.30: (production)

   - Removed code duplication in class EcfFile
   - Modified test.sh for autotools integration
   - Fix bug with alter, change variable, where value is a path
   - Fixed ecflowview duplicate symbol warning on ecgate.
   - Fixed RepeatDate variable, so that its in range of start/end, at expiration
   - Change replace node to check expressions and limits
   - ECFLOW-44 variable add/edit with ecflowview variable panel
   - ECFLOW-43 script external viewer window (ecflowview)
   
2.0.29: (Beta)

   - Changed Child wait command to error if expression references paths that don't exist
   - Added functionality to allow zombie process to be killed
   - Changed server polling to avoid syncronous wait
   - Change child commands so that job generation is deferred to the server
   - Improved defs file parser performance
   - Begin command changed so that it forces a full sync in the client
   - Automatic checkpoint by server is now logged.
   - ecflowview changed, will now prompt for suite name, on first open
 
2.0.28:

   - Changed Free dependencies command so that it misses next time slot
   - Change Python Api to allow with statement use on tasks
   - Changed AlterCmd to show errors on the command line, when illegal paths specified
   - Changed AlterCmd for suite clocks.Clock attribute added if it does not exist, requires re-queue of suite to take effect
   - Changed default ECF_KILL_CMD to "kill -15 %ECF_RID%"
   - Changed default ECF_STATUS_CMD to "ps --sid %ECF_RID% -f"
   - Server load command(--server_load) will now graphically display top 5 suites contributing to server load
   - Improved parser performance
   - ecflowview: various bug fixes
   - Changed ecflow_start.sh to use use correct kill and status command on ecgate
   
2.0.27:

   - Improved parsing time for definition file.
   - Changed server startup, so that if check pt exist but can't be loaded, then server exits
   - Added new command to print the list of handles and referenced suites
   - Alias creation changed , so that variable addition by passes checks
   - ecflowview: Fix for variable exception on startup, when RepeatDay used, ECFLOW-38
   - ecflowview: various bug fixes
   
2.0.26:

   - Changed node suspend/resume so they no longer check the suite begun status
   - Changed test Test/src/TestEvents to remove dependence on log file verification.
   - Updated online tutorial
   - Added support ECF_VERSION server environment variable
   - Minor performance tweaks, added Variable constructor that does not check for valid names
   - Change defaults for job submission interval, to avoid assert
   - Changed interface for Variables on Defs to be same as Node
   - Removed Defs suspend/resume to use server states instead
   - Updated the command line zombie commands to succeed whenever possible
   - Updated Task commands, to flag a zombie when task set to complete
   
2.0.25:

   - Updated online tutorial
   - Updated python api, to allow use of a dictionary when adding variables
   - Updated python api, to support with statement, allowing indentation
   - Updated python api, to allow functional programming
   - Updated python api, to host/port to be set directly on the Client
   - Update why for limits to include first 4 consumed node paths
   
2.0.24:

   - Change force and run command, so that no requeue if single time dependency flag is set up node hierarchy
   - Increased the timeout out for the client to server communication
   - Allow suites to be registered before they are loaded into the server
   - Update sync commands to reset local caches when no definition in the server
   - Update server to support SIGTERM for emergency check pointing & added regression test
   - Allow setting of new log file path using the existing ECF_LOG variable
   - Improved accuracy of statistics recording the number of requests per second
   - Client errors are now sent to standard error instead of standard output
   - Added support for boost 1.48
   - License changes. We now use Apache license 2.0
   
2.0.23:

   - Added ECF_HOME,ECF_CHECK,ECF_LOG to the output of --stats(statistics) output
   - Improved handling of errors in server, due to file system full testing.
   - Changed --suites to not throw error if no suites in the server
   - Updated server statistics to include reloading from a check point file
   - Remove automatic generation of .man files (left over from testing).
   - Changed replace, to act like add when there is no definition in the server.
   - Changed python interface for set_host_port, allow integer for port, and single string <host>:<port>
   - Changed handle commands so that deleted suites stay registered,until explicitly removed
   - ECFLOW-34 Running ecflow_server with wrong options results in obscure message and core dump.
   - ECFLOW-35 Documentation gets installed in ${PREFIX}/doc which is not good when PREFIX is /usr or /usr/local
   - ecflowview bug fixes &  Cleaned up some compilation warnings
   - Improved zombie logging message to include type of zombie.

2.0.22:

   - Fixed: Bug with white list file, where read only user could terminate server
   - Support for python 2.7 on AIX
   - Fixed: Release mode now works for AIX compiler v11.1 and v12
   - Changed: Defs::find_extern()  performance enhancement.
   - Changed: File::create(.)/LogImpl::do_log() added better error checking
   - Fixed: ECFLOW-32 start_server.sh doesn't seem to work on Ubuntu 11.04. Output attached
   - Migration to boost 1.47
  
2.0.21:

   - Fixed: ECFLOW-29 Compilation fails: boost filesystem doesn't seem to build properly
   - Fixed: ECFLOW-30 64-bit Linux platforms expect libraries to go to the $PREFIX/lib64 directory, not $PREFIX/lib
   - Fixed: ECFLOW-31 Allow ECF_JOB to be overridden
   - Changed Online tutorial to extract ecFlow version from installed directory, if extraction from the source code fails
   - Changes to support boost 1.47 
   - Fixed: ecflowview does not render RepeatDate end date correctly, (displays end date + 1)
  
2.0.20:

   - Changed install of ecflow python extension after feedback from Daniel.
   - Fixed: Bug in replace/add where sibling node states were not preserved
   - Fixed: ECFLOW-27 : ecflow_client --log=path, returns the log file name rather than the log file path as advertised.
   - Update FAQ for online tutorial. if ECF_OUT defined make sure directories are defined
   - Update Python API, added Suite::begun() to query if suite has begun
   - Fixed: ECFLOW-28 compilation fails in RHEL 6.0 : Changed script build_boost.sh. replaced $CPU with 'uname -m'. This will choose the right site-config.jam. i.e will include flags -fPIC for all compilations
   - Added support for boost file system 3, this should allow ecflow to built with the latest boost version

2.0.19:

   - Licensing: All files should have ': Version     : Beta version' for test use only. 
   - Allow Meter command to accept any valid value that is in the meter range(Asked by John)
   - Updated ecflow_client --get_state, so that suite will show begun status
   - Fixed: ECFLOW-21 Remove /bin/ksh dependency for ecflow_start.sh and stop scripts. 
     Try to use /bin/sh if possible 
   - Fixed: ECFLOW-23 When replacing a node the order is changed. 
   - Fixed: ECFLOW-22 Zombie icons not showing 
   - When ECF_CHECK is set, the check point file could be at any 
     directory and any name. Check for absolute paths

2.0.18:

   - Changed help structure, added summary 
   - Changed Jamroot.jam to conditionally build ecflowview on 
     Linux and rs6000 platforms only. 
     Uses ARCH environment variable
   - Doc: Changed installation to include PDF version of user manual
     Fixed: ECFLOW-20 The only installed documentation in 
     ${prefix}/doc is a .docx file
     
2.0.17:

   - Beta Feedback: Updated server to allow specification of TCP/IP protocol as command line argument
   - Beta Feedback: CFileCmd:       removed check for begun, when requesting the script,manual,job,jobout
   - Beta Feedback: EditScriptCmd:  removed check for begun, when requesting the pre-processed file
   - RequeueNodeCmd: *Added* check for begin before requeue.
   - Beta Feedback: NodeTreeTraverser: Change server poll to align with minute boundary
   - Change installation of python. Environment variable for install not defined in site-config.jam
   - Added Python class to demonstrate traversal of node tree
   - Updated online tutorial to show example of traversal of node tree
   - Changed ClientInvoker so that we can configure the number of attempts to connect to the server, and the period between attempts.
