.. index::
   single: task_wrappers
   
.. _task_wrappers:
   
Task wrappers migration
-----------------------

The task wrapper file does not normally need many changes, 
if the task designer sticks to the KISS principle, focusing on
the functional aspect of the task.

* The file name is changed, ending with **.ecf** instead of **.sms**. 
* simply copy or link the original file from .sms into .ecf

* alternatively, define a variable ECF_EXTN in the definition 
  file:: edit ECF_EXTN .sms

  This requests that the ecFlow server uses .sms wrappers as the task template.
  In some cases, no files will need translation (no SMS variables, no
  CDP calls)

* smsmicro is replaced with **ecf_micro**, when needed

  ========= ==========  ===============
  SMS       ecFlow      location
  ========= ==========  ===============
  SMSMICRO  ECF_MICRO   definition file
  %smsmicro %ecf_micro  script .ecf .h
  ========= ==========  ===============

.. # find . -type f -name "*sms" -exec grep "%SMS" {} \; -print | grep "^./"  | wc
.. # find . -type f -name "*sms" -print | grep "^./"  | wc
.. # find . -type f -name "*sms" -exec grep "%SMS" {} \; -print | grep "%SMS"  | cut -d% -f2 | sort | uniq
.. # find . -type f -name "*sms" -exec grep "%SMS" {} \; -print | grep "^./"
 
* In ECMWF Operations, in the main branch, amongst 1394 files, only 43 use SMS
  system variables, i.e. variables whose name starts with SMS. Among all the
  suites MetApps is in charge of, amongst 3738 files, 216 are affected.
  Extracting these variables, we have::

    ============
    %SMS in .sms
    ============
    SMSCHECK
    SMSCHECKOLD
    SMSDATE
    SMSFILES
    SMSHOME
    SMSHOST
    SMSINCLUDE
    SMSJOBOUT
    SMSLOG
    SMSNAME
    SMSNODE
    SMSTRYNO
    SMSURLBASE
    SMS_PROG
    ============

  You must then decide whether to:
    * tranlate SMS variables to the matching ecFlow variables
    * or replace SMS variable with a shell variable $SMS, or $ECF_

  Similarly, we can identify all scripts that call the CDP text client.

  It is a good design principle to create tasks that are independent of SMS
  system variables. Only the tasks in charge of "advanced use" are
  concerned: SMSTRYNO was used to make a job aware of its instance
  number, enabling verbose output in case of rerun.

  One step translation consists of running the scripts through a
  filter that can be used for both expanded SMS definition files or for
  task wrappers::

  > sed -f sms2ecf-min.sed X.sms > X.ecf

  .. literalinclude:: src/sms2ecf-min.sed

  Applying such a filter to all sms tasks can be simplfied::

    #!/bin/ksh
    files=`find -type f -name "*.sms"  ## all sms wrappers
    for f in $files ; do 
    ecf=$(basename $f .sms).ecf        ## ecf task name
    sed -f sms2ecf-min.sed $f > $ecf   ## translate
    diff $f $ecf > /dev/null && rm $ecf && ln -sf $f $g ## or link
    done


  SMS wrappers links can be preserved::

    #!/bin/ksh
    files=`find -type l -name "*.sms" `
    for f in $files ; do     
    ecf=$(basename $f .sms).ecf        ## ecf task name
    link=$(readlink $f)    
    dir=$(dirname $f); cd $dir
    ln -sf $link $ecf
    cd -
    done

  Special attention is needed for the variables renaming:

  ============= ==============  
  SMS           ecFlow
  ============= ==============  
  SMSCMD        ECF_JOB_CMD
  SMSKILL       ECF_KILL_CMD
  SMS_STATUSCMD ECF_STATUS_CMD
  SMS_URLCMD    ECF_URL_CMD
  ============= ==============

  It is not a good idea to systematically replace SMS with ECF\_, for
  example, we use the variables NO_SMS and LSMSSIG which are not related to SMS.

* If we want to run the the same job using both SMS and ecFlow,
  %SMSXXX% may be replaced with shell variables ECF_XXX. Then in a
  header file, we will define ECF_XXX=%SMSXXX:0% for sms mode and
  ECF_XXX=%ECF_XXX:0% for ecFlow mode.

* All tasks calling CDP directly must be treated carefully and text 
  client commands replaced with their ecFlow counterpart. They may 
  force complete a family or a task, requeue a job or change a variable value:

  .. literalinclude:: src/call-cdp.cdp

  The ECF_PORT variable gives us the ability to discriminate between jobs under
  ecFlow control or not:

  .. literalinclude:: src/call_both.ksh

* sms child commands may also be called in few sms task wrappers.  These should again
  be replaced with their ecFlow equivalents.
